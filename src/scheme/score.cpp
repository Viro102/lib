#include <qfileinfo.h>
#include <qimagereader.h>

#include <m3colours/quantise.hpp>
#include <m3colours/scheme.hpp>
#include <m3colours/score.hpp>

#include <vector>

#include "caelestia/scheme/scheme.hpp"
#include "scorecache.hpp"

namespace caelestia::scheme {

namespace {

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 1;
constexpr char k_delimiter = '|';
constexpr int k_maxColours = 128;

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

QList<m3colours::Argb> scoreImage(const QString& path) {
    QImageReader reader(path);
    if (!reader.canRead())
        return {};

    const auto image = reader.read();
    std::vector<m3colours::Argb> pixels;
    pixels.reserve(static_cast<std::size_t>(image.width()) * image.height());
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            pixels.emplace_back(image.pixel(x, y));

    const auto quant = m3colours::quantise::celebi(pixels, k_maxColours);
    const auto scored = m3colours::score::scoreByCutoff(quant.colourToCount);
    return { scored.begin(), scored.end() };
}

} // namespace

ScoreCache::ScoreCache()
    : BlobCache("schemes/scores", k_maxSize, k_cacheVersion) {}

ScoreCache& ScoreCache::instance() {
    static ScoreCache s_instance;
    return s_instance;
}

QFuture<QList<m3colours::Argb>> coloursFor(const QString& path) {
    return ScoreCache::instance()
        .lookup(ScoreCache::keyForPath(path))
        .then(QtFuture::Launch::Async, [path](const QByteArray& data) -> QList<m3colours::Argb> {
            if (!data.isEmpty())
                return bytesToColours(data);

            const auto colours = scoreImage(path);
            if (!colours.isEmpty())
                ScoreCache::instance().store(ScoreCache::keyForPath(path), coloursToBytes(colours));
            return colours;
        });
}

} // namespace caelestia::scheme
