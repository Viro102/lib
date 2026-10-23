#include "caelestia/media/analysis.hpp"

#include <qdatastream.h>
#include <qfileinfo.h>
#include <qimagereader.h>
#include <qmimedatabase.h>

#include <m3colours/hct.hpp>
#include <m3colours/quantise.hpp>
#include <m3colours/score.hpp>

#include <algorithm>
#include <cmath>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "analysiscache.hpp"
#include "videopixels.hpp"

namespace caelestia::media {

using Qt::StringLiterals::operator""_s;

namespace {

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 1;
constexpr int k_maxColours = 128;
// Just above the pixel count at which m3colours switches from exact to binned quantisation
constexpr std::size_t k_targetPixels = (std::size_t{ 1 } << 20u) + 1;
constexpr int k_maxFrames = 300;
constexpr int k_videoSamples = 16;

static_assert(sizeof(m3colours::Argb) == sizeof(QRgb) && std::is_standard_layout_v<m3colours::Argb>);

QByteArray serialise(const Analysis& analysis) {
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << static_cast<quint32>(analysis.colours.size());
    for (const auto& colour : analysis.colours)
        stream << colour.rgba();
    stream << analysis.dominant.rgba() << analysis.colourfulness << analysis.luminance << analysis.tone;
    return data;
}

Analysis deserialise(const QByteArray& data) {
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);

    Analysis analysis;
    quint32 count = 0;
    stream >> count;
    for (quint32 i = 0; i < count && stream.status() == QDataStream::Ok; ++i) {
        quint32 value = 0;
        stream >> value;
        analysis.colours << QColor::fromRgba(value);
    }
    quint32 dominant = 0;
    stream >> dominant >> analysis.colourfulness >> analysis.luminance >> analysis.tone;
    analysis.dominant = QColor::fromRgba(dominant);

    return stream.status() == QDataStream::Ok && stream.atEnd() ? analysis : Analysis{};
}

// Hasler & Süsstrunk colourfulness, perceived luminance and HCT tone of the mean colour,
// all computed over the quantiser's unique/binned colours rather than every pixel
Analysis analysePixels(std::span<const m3colours::Argb> pixels) {
    const auto unique = m3colours::quantise::uniquePixels(pixels, true);
    if (unique.pixels.empty())
        return {};

    double sumRg = 0;
    double sumYb = 0;
    double sumRg2 = 0;
    double sumYb2 = 0;
    double sumLuminance = 0;
    double sumR = 0;
    double sumG = 0;
    double sumB = 0;
    double n = 0;
    std::size_t best = 0;

    for (std::size_t i = 0; i < unique.pixels.size(); ++i) {
        const auto pixel = unique.pixels[i];
        const auto count = static_cast<double>(unique.counts[i]);
        if (unique.counts[i] > unique.counts[best])
            best = i;

        const double r = pixel.red();
        const double g = pixel.green();
        const double b = pixel.blue();
        const auto rg = r - g;
        const auto yb = 0.5 * (r + g) - b;
        sumRg += count * rg;
        sumYb += count * yb;
        sumRg2 += count * rg * rg;
        sumYb2 += count * yb * yb;
        sumLuminance += count * std::sqrt(0.299 * r * r + 0.587 * g * g + 0.114 * b * b) / 255.0;
        sumR += count * r;
        sumG += count * g;
        sumB += count * b;
        n += count;
    }

    const auto meanRg = sumRg / n;
    const auto meanYb = sumYb / n;
    const auto varRg = std::max(0.0, sumRg2 / n - meanRg * meanRg);
    const auto varYb = std::max(0.0, sumYb2 / n - meanYb * meanYb);
    const auto mean = m3colours::Argb(static_cast<std::uint8_t>(std::lround(sumR / n)),
        static_cast<std::uint8_t>(std::lround(sumG / n)), static_cast<std::uint8_t>(std::lround(sumB / n)));

    const auto quant = m3colours::quantise::celebi(unique, k_maxColours);
    const auto scored = m3colours::score::scoreByCutoff(quant.colourToCount);
    QList<QColor> colours;
    colours.reserve(static_cast<qsizetype>(scored.size()));
    for (const auto colour : scored)
        colours.append(QColor::fromRgba(colour.value()));

    return {
        .colours = std::move(colours),
        .dominant = QColor::fromRgba(unique.pixels[best].value()),
        .colourfulness = std::sqrt(varRg + varYb) + 0.3 * std::sqrt(meanRg * meanRg + meanYb * meanYb),
        .luminance = sumLuminance / n,
        .tone = m3colours::Hct(mean).tone(),
    };
}

std::size_t pixelsPerFrame(std::size_t frames) {
    return (k_targetPixels + frames - 1) / frames;
}

std::size_t pixelCount(const QImage& image) {
    return static_cast<std::size_t>(image.width()) * static_cast<std::size_t>(image.height());
}

void appendFrame(std::vector<m3colours::Argb>& pixels, const QImage& frame, std::size_t targetPixels) {
    auto formatted = frame.convertToFormat(QImage::Format_ARGB32);

    if (const auto size = sampleSize(formatted.width(), formatted.height(), targetPixels); size != formatted.size())
        formatted = formatted.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    const auto* bits = reinterpret_cast<const m3colours::Argb*>(formatted.constBits());
    pixels.insert(pixels.end(), bits, bits + pixelCount(formatted));
}

std::vector<m3colours::Argb> imagePixels(const QImage& image) {
    const auto formatted = image.convertToFormat(QImage::Format_ARGB32);
    const auto* bits = reinterpret_cast<const m3colours::Argb*>(formatted.constBits());
    return { bits, bits + pixelCount(formatted) };
}

std::vector<m3colours::Argb> animationPixels(QImageReader& reader) {
    const auto frames = reader.imageCount() > 0 ? std::min(reader.imageCount(), k_maxFrames) : k_maxFrames;
    const auto target = pixelsPerFrame(static_cast<std::size_t>(frames));

    std::vector<m3colours::Argb> pixels;
    for (int i = 0; i < frames; ++i) {
        const auto frame = reader.read();
        if (frame.isNull())
            break;

        appendFrame(pixels, frame, target);
    }
    return pixels;
}

bool isVideo(const QString& path) {
    return QMimeDatabase().mimeTypeForFile(path).name().startsWith(u"video/"_s);
}

std::vector<m3colours::Argb> pixelsForFile(const QString& path) {
    if (isVideo(path))
        return videoPixels(path, k_videoSamples, pixelsPerFrame(k_videoSamples));

    QImageReader reader(path);
    if (!reader.canRead())
        return {};

    if (reader.supportsAnimation())
        return animationPixels(reader);

    return imagePixels(reader.read());
}

} // namespace

AnalysisCache::AnalysisCache()
    : BlobCache(u"media/analysis"_s, k_maxSize, k_cacheVersion) {}

AnalysisCache& AnalysisCache::instance() {
    static AnalysisCache s_instance;
    return s_instance;
}

bool isSupported(const QString& path) {
    if (!QFileInfo(path).isFile())
        return false;
    return isVideo(path) || QImageReader(path).canRead();
}

Analysis analyseImage(const QImage& image) {
    if (image.isNull())
        return {};

    return analysePixels(imagePixels(image));
}

QFuture<Analysis> analyseForPath(const QString& path) {
    return AnalysisCache::instance()
        .lookup(AnalysisCache::keyForPath(path))
        .then(QtFuture::Launch::Async, [path](const QByteArray& data) -> Analysis {
            if (const auto cached = deserialise(data); !cached.colours.isEmpty())
                return cached;

            const auto analysis = analysePixels(pixelsForFile(path));
            if (!analysis.colours.isEmpty())
                AnalysisCache::instance().store(AnalysisCache::keyForPath(path), serialise(analysis));
            return analysis;
        });
}

} // namespace caelestia::media
