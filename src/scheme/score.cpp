#include <qfileinfo.h>
#include <qimagereader.h>

#include <m3colours/quantise.hpp>
#include <m3colours/scheme.hpp>
#include <m3colours/score.hpp>

#include <span>
#include <type_traits>

#include "caelestia/scheme/scheme.hpp"
#include "scorecache.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

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

    const auto image = reader.read().convertToFormat(QImage::Format_ARGB32);
    static_assert(sizeof(m3colours::Argb) == sizeof(QRgb) && std::is_standard_layout_v<m3colours::Argb>);
    const std::span pixels(reinterpret_cast<const m3colours::Argb*>(image.constBits()),
        static_cast<std::size_t>(image.sizeInBytes()) / sizeof(QRgb));

    const auto quant = m3colours::quantise::celebi(pixels, k_maxColours);
    const auto scored = m3colours::score::scoreByCutoff(quant.colourToCount);
    return { scored.begin(), scored.end() };
}

} // namespace

ScoreCache::ScoreCache()
    : BlobCache(u"schemes/scores"_s, k_maxSize, k_cacheVersion) {}

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
