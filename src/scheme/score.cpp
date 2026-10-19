#include <qfileinfo.h>
#include <qimagereader.h>
#include <qmimedatabase.h>

#include <m3colours/quantise.hpp>
#include <m3colours/scheme.hpp>
#include <m3colours/score.hpp>

#include <algorithm>
#include <cmath>
#include <span>
#include <type_traits>
#include <vector>

#include "caelestia/scheme/scheme.hpp"
#include "scorecache.hpp"
#include "videopixels.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 2;
constexpr char k_delimiter = '|';
constexpr int k_maxColours = 128;
// Just above the pixel count at which m3colours switches from exact to binned quantisation
constexpr std::size_t k_targetPixels = (std::size_t{ 1 } << 20) + 1;
constexpr int k_maxFrames = 300;
constexpr int k_videoSamples = 16;

static_assert(sizeof(m3colours::Argb) == sizeof(QRgb) && std::is_standard_layout_v<m3colours::Argb>);

QList<m3colours::Argb> bytesToColours(const QByteArray& data) {
    const auto list = data.split(k_delimiter);
    QList<m3colours::Argb> colours;
    colours.reserve(list.size());
    for (const auto& item : list)
        colours << m3colours::Argb{ item.toUInt(nullptr, 16) };
    return colours;
}

QByteArray coloursToBytes(const QList<m3colours::Argb>& colours) {
    QByteArray data;
    for (const auto& colour : colours)
        data.append(QString::number(colour.value(), 16).toUtf8() + k_delimiter);
    return data.left(data.size() - 1);
}

QList<m3colours::Argb> coloursForPixels(std::span<const m3colours::Argb> pixels) {
    if (pixels.empty())
        return {};

    const auto quant = m3colours::quantise::celebi(pixels, k_maxColours);
    const auto scored = m3colours::score::scoreByCutoff(quant.colourToCount);
    return { scored.begin(), scored.end() };
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

QList<m3colours::Argb> coloursForFile(const QString& path) {
    if (QMimeDatabase().mimeTypeForFile(path).name().startsWith(u"video/"_s))
        return coloursForPixels(videoPixels(path, k_videoSamples, pixelsPerFrame(k_videoSamples)));

    QImageReader reader(path);
    if (!reader.canRead())
        return {};

    if (reader.supportsAnimation())
        return coloursForPixels(animationPixels(reader));

    return coloursForImage(reader.read());
}

} // namespace

ScoreCache::ScoreCache()
    : BlobCache(u"schemes/scores"_s, k_maxSize, k_cacheVersion) {}

ScoreCache& ScoreCache::instance() {
    static ScoreCache s_instance;
    return s_instance;
}

QList<m3colours::Argb> coloursForImage(const QImage& image) {
    const auto formatted = image.convertToFormat(QImage::Format_ARGB32);
    const std::span pixels(reinterpret_cast<const m3colours::Argb*>(formatted.constBits()),
        static_cast<std::size_t>(formatted.sizeInBytes()) / sizeof(QRgb));
    return coloursForPixels(pixels);
}

QFuture<QList<m3colours::Argb>> coloursForPath(const QString& path) {
    return ScoreCache::instance()
        .lookup(ScoreCache::keyForPath(path))
        .then(QtFuture::Launch::Async, [path](const QByteArray& data) -> QList<m3colours::Argb> {
            if (!data.isEmpty())
                return bytesToColours(data);

            const auto colours = coloursForFile(path);
            if (!colours.isEmpty())
                ScoreCache::instance().store(ScoreCache::keyForPath(path), coloursToBytes(colours));
            return colours;
        });
}

} // namespace caelestia::scheme
