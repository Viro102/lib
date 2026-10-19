#include "caelestia/scheme/scheme.hpp"

#include <qfile.h>
#include <qfuture.h>
#include <qhash.h>
#include <qiodevice.h>
#include <qloggingcategory.h>
#include <qtextstream.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <type_traits>

#include "caelestia/config/paths.hpp"
#include "caelestia/media/analysis.hpp"
#include "colournames.hpp"
#include "generator.hpp"
#include "schemecache.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

Q_LOGGING_CATEGORY(lcScheme, "caelestia.scheme", QtInfoMsg)

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 1;
constexpr m3colours::Argb k_fallbackColour{ 0xFF4285F4 };

QString toString(m3colours::scheme::Variant flavour) {
    switch (flavour) {
    case m3colours::scheme::Variant::Monochrome:
        return u"monochrome"_s;
    case m3colours::scheme::Variant::Neutral:
        return u"neutral"_s;
    case m3colours::scheme::Variant::TonalSpot:
        return u"tonalspot"_s;
    case m3colours::scheme::Variant::Vibrant:
        return u"vibrant"_s;
    case m3colours::scheme::Variant::Expressive:
        return u"expressive"_s;
    case m3colours::scheme::Variant::Fidelity:
        return u"fidelity"_s;
    case m3colours::scheme::Variant::Content:
        return u"content"_s;
    case m3colours::scheme::Variant::Rainbow:
        return u"rainbow"_s;
    case m3colours::scheme::Variant::FruitSalad:
        return u"fruitsalad"_s;
    case m3colours::scheme::Variant::Cmf:
        return u"cmf"_s;
    }
    Q_UNREACHABLE_RETURN(QString());
}

} // namespace

static_assert(std::is_trivially_copyable_v<SchemeColours>);

std::optional<SchemeColours> coloursFromBytes(const QByteArray& bytes) {
    if (bytes.size() != static_cast<qsizetype>(sizeof(SchemeColours))) {
        qCWarning(lcScheme) << "Cached scheme has unexpected size:" << bytes.size();
        return std::nullopt;
    }

    std::array<std::byte, sizeof(SchemeColours)> raw;
    std::memcpy(raw.data(), bytes.constData(), raw.size());
    return std::bit_cast<SchemeColours>(raw);
}

QByteArray coloursToBytes(const SchemeColours& colours) {
    return { reinterpret_cast<const char*>(&colours), sizeof(colours) };
}

namespace {

m3colours::Argb sourceColour(const QList<m3colours::Argb>& colours) {
    if (colours.isEmpty())
        qCWarning(lcScheme) << "No usable colours, falling back to" << Qt::hex << k_fallbackColour.value();
    return colours.value(0, k_fallbackColour);
}

QHash<QString, m3colours::Argb> parseColours(QIODevice& device) {
    QHash<QString, m3colours::Argb> colours;
    QTextStream in(&device);
    while (!in.atEnd()) {
        const auto line = in.readLine().trimmed();
        if (line.startsWith(u"//"))
            continue;

        const auto parts = line.split(u' ', Qt::SkipEmptyParts);
        if (parts.size() != 2)
            continue;

        QStringView value = parts[1];
        if (value.startsWith(u'#'))
            value = value.sliced(1);

        colours.insert(parts[0], m3colours::Argb{ 0xFF000000u | value.toUInt(nullptr, 16) });
    }
    return colours;
}

Scheme parseScheme(const QString& path, const QString& name, const QString& flavour, Scheme::Mode mode) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::invalid_argument(u"No such scheme: %1-%2-%3"_s.arg(name, flavour, modeName(mode)).toStdString());

    const auto map = parseColours(file);
    const auto colours = [&](QStringView key) {
        const auto it = map.constFind(key);
        if (it == map.cend()) {
            qCWarning(lcScheme) << "Scheme" << path << "is missing colour" << key;
            return m3colours::Argb{ 0 };
        }
        return *it;
    };

#define COLOUR(name) .name = colours(u## #name),

    return {
        .name = name,
        .flavour = flavour,
        .mode = mode,
        .colours = { CAELESTIA_SCHEME_COLOURS(COLOUR) },
    };

#undef COLOUR
}

} // namespace

QFuture<Scheme> schemeForName(const QString& name, const QString& flavour, Scheme::Mode mode) {
    const auto path = u":/schemes/%1/%2/%3.txt"_s.arg(name, flavour, modeName(mode));
    return QtFuture::makeReadyValueFuture(path)
        .then(QtFuture::Launch::Async,
            [name, flavour, mode](const QString& p) {
                return parseScheme(p, name, flavour, mode);
            })
        .onFailed([name, flavour, mode] {
            const auto customPath =
                config::ConfigPaths::customSchemesDir() + u"/%1/%2/%3.txt"_s.arg(name, flavour, modeName(mode));
            return parseScheme(customPath, name, flavour, mode);
        });
}

Scheme schemeForImage(const QImage& image, m3colours::scheme::Variant flavour, Scheme::Mode mode) {
    const auto colours = coloursForImage(image);
    return {
        .name = u"dynamic"_s,
        .flavour = toString(flavour),
        .mode = mode,
        .colours = gen::genScheme(sourceColour(colours), flavour, mode),
    };
}

QFuture<Scheme> schemeForPath(const QString& path, m3colours::scheme::Variant flavour, Scheme::Mode mode) {
    return coloursForPath(path).then([path, flavour, mode](const QList<m3colours::Argb>& colours) -> Scheme {
        const auto best = sourceColour(colours);
        auto future = SchemeCache::instance().lookup(SchemeCache::keyFor(best, flavour, mode));
        future.waitForFinished();

        const auto res = future.result();
        if (!res.isEmpty()) {
            if (const auto cached = coloursFromBytes(res))
                return { .name = u"dynamic"_s, .flavour = toString(flavour), .mode = mode, .colours = *cached };
        }

        const auto gen = gen::genScheme(best, flavour, mode);
        SchemeCache::instance().store(SchemeCache::keyFor(best, flavour, mode), coloursToBytes(gen));

        return { .name = u"dynamic"_s, .flavour = toString(flavour), .mode = mode, .colours = gen };
    });
}

SchemeCache::SchemeCache()
    : BlobCache(u"schemes"_s, k_maxSize, k_cacheVersion) {}

SchemeCache& SchemeCache::instance() {
    static SchemeCache s_instance;
    return s_instance;
}

QString SchemeCache::keyFor(m3colours::Argb sourceColour, m3colours::scheme::Variant flavour, Scheme::Mode mode) {
    return utils::Cache::keyFor(
        u"%1|%2|%3"_s.arg(QString::number(sourceColour.value(), 16), toString(flavour), modeName(mode)));
}

QString modeName(Scheme::Mode mode) {
    switch (mode) {
    case Scheme::Mode::Light:
        return u"light"_s;
    case Scheme::Mode::Dark:
        return u"dark"_s;
    }
    Q_UNREACHABLE_RETURN(QString());
}

QList<NamedColour> namedColours(const SchemeColours& colours) {
#define COLOUR(name) NamedColour{ u## #name, colours.name },
    return { CAELESTIA_SCHEME_COLOURS(COLOUR) };
#undef COLOUR
}

QList<m3colours::Argb> coloursForImage(const QImage& image) {
    return media::analyseImage(image).colours;
}

QFuture<QList<m3colours::Argb>> coloursForPath(const QString& path) {
    return media::analyseForPath(path).then([](const media::Analysis& analysis) {
        return analysis.colours;
    });
}

} // namespace caelestia::scheme
