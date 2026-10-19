#include <qeventloop.h>
#include <qfileinfo.h>
#include <qimagereader.h>
#include <qloggingcategory.h>
#include <qmediaplayer.h>
#include <qmimedatabase.h>
#include <qtimer.h>
#include <qurl.h>
#include <qvideoframe.h>
#include <qvideosink.h>

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

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

Q_LOGGING_CATEGORY(lcScore, "caelestia.scheme.score", QtInfoMsg)

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 2;
constexpr char k_delimiter = '|';
constexpr int k_maxColours = 128;
// Just above the pixel count at which m3colours switches from exact to binned quantisation
constexpr std::size_t k_targetPixels = (std::size_t{ 1 } << 20) + 1;
constexpr int k_maxFrames = 300;
constexpr int k_videoSamples = 16;
constexpr int k_videoTimeoutMs = 5000;

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

    if (const auto area = pixelCount(formatted); area > targetPixels) {
        const auto factor = std::sqrt(static_cast<double>(targetPixels) / static_cast<double>(area));
        formatted = formatted.scaled(static_cast<int>(std::ceil(formatted.width() * factor)),
            static_cast<int>(std::ceil(formatted.height() * factor)), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

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

std::vector<m3colours::Argb> videoPixels(const QString& path) {
    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&player, &QMediaPlayer::mediaStatusChanged, &loop, &QEventLoop::quit);
    QObject::connect(&player, &QMediaPlayer::errorOccurred, &loop, &QEventLoop::quit);

    QVideoFrame frame;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &loop, [&](const QVideoFrame& f) {
        frame = f;
        loop.quit();
    });

    const auto waitUntil = [&](auto&& done) {
        timeout.start(k_videoTimeoutMs);
        while (!done() && timeout.isActive())
            loop.exec();
        return done();
    };

    player.setSource(QUrl::fromLocalFile(path));
    waitUntil([&] {
        const auto status = player.mediaStatus();
        return status != QMediaPlayer::NoMedia && status != QMediaPlayer::LoadingMedia;
    });

    if (player.error() != QMediaPlayer::NoError || !player.hasVideo()) {
        qCWarning(lcScore) << "Failed to load video" << path << player.errorString();
        return {};
    }

    std::vector<m3colours::Argb> pixels;
    const auto target = pixelsPerFrame(k_videoSamples);
    const auto sample = [&] {
        const auto frameValid = waitUntil([&] {
            return frame.isValid();
        });
        if (!frameValid)
            return false;

        appendFrame(pixels, frame.toImage(), target);
        frame = {};
        return true;
    };

    player.pause();
    if (!sample())
        return pixels;

    const auto duration = player.duration();
    if (duration <= 0 || !player.isSeekable())
        return pixels;

    for (int i = 1; i < k_videoSamples; ++i) {
        player.setPosition(duration * i / k_videoSamples);
        if (!sample())
            break;
    }

    player.stop();
    return pixels;
}

QList<m3colours::Argb> coloursForFile(const QString& path) {
    if (QMimeDatabase().mimeTypeForFile(path).name().startsWith(u"video/"_s))
        return coloursForPixels(videoPixels(path));

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
