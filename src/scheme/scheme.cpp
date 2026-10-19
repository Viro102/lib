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

#include "generator.hpp"
#include "schemecache.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

Q_LOGGING_CATEGORY(lcScheme, "caelestia.scheme", QtInfoMsg)

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 1;

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

QString toString(Scheme::Mode mode) {
    switch (mode) {
    case Scheme::Mode::Light:
        return u"light"_s;
    case Scheme::Mode::Dark:
        return u"dark"_s;
    }
    Q_UNREACHABLE_RETURN(QString());
}

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
        throw std::invalid_argument("No such scheme: " + path.toStdString());

    const auto map = parseColours(file);
    const auto colours = [&](QStringView key) {
        const auto it = map.constFind(key);
        if (it == map.cend()) {
            qCWarning(lcScheme) << "Scheme" << path << "is missing colour" << key;
            return m3colours::Argb{ 0 };
        }
        return *it;
    };

#define COLOUR(name) .name = colours(u## #name)

    return {
        .name = name,
        .flavour = flavour,
        .mode = mode,
        .colours = {
            COLOUR(primaryPaletteKeyColour),
            COLOUR(secondaryPaletteKeyColour),
            COLOUR(tertiaryPaletteKeyColour),
            COLOUR(neutralPaletteKeyColour),
            COLOUR(neutralVariantPaletteKeyColour),
            COLOUR(errorPaletteKeyColour),
            COLOUR(background),
            COLOUR(onBackground),
            COLOUR(surface),
            COLOUR(surfaceDim),
            COLOUR(surfaceBright),
            COLOUR(surfaceContainerLowest),
            COLOUR(surfaceContainerLow),
            COLOUR(surfaceContainer),
            COLOUR(surfaceContainerHigh),
            COLOUR(surfaceContainerHighest),
            COLOUR(onSurface),
            COLOUR(onSurfaceVariant),
            COLOUR(outline),
            COLOUR(outlineVariant),
            COLOUR(inverseSurface),
            COLOUR(inverseOnSurface),
            COLOUR(primary),
            COLOUR(onPrimary),
            COLOUR(primaryContainer),
            COLOUR(onPrimaryContainer),
            COLOUR(primaryFixed),
            COLOUR(primaryFixedDim),
            COLOUR(onPrimaryFixed),
            COLOUR(onPrimaryFixedVariant),
            COLOUR(inversePrimary),
            COLOUR(secondary),
            COLOUR(onSecondary),
            COLOUR(secondaryContainer),
            COLOUR(onSecondaryContainer),
            COLOUR(secondaryFixed),
            COLOUR(secondaryFixedDim),
            COLOUR(onSecondaryFixed),
            COLOUR(onSecondaryFixedVariant),
            COLOUR(tertiary),
            COLOUR(onTertiary),
            COLOUR(tertiaryContainer),
            COLOUR(onTertiaryContainer),
            COLOUR(tertiaryFixed),
            COLOUR(tertiaryFixedDim),
            COLOUR(onTertiaryFixed),
            COLOUR(onTertiaryFixedVariant),
            COLOUR(error),
            COLOUR(onError),
            COLOUR(errorContainer),
            COLOUR(onErrorContainer),
            COLOUR(success),
            COLOUR(onSuccess),
            COLOUR(successContainer),
            COLOUR(onSuccessContainer),
            COLOUR(term0),
            COLOUR(term1),
            COLOUR(term2),
            COLOUR(term3),
            COLOUR(term4),
            COLOUR(term5),
            COLOUR(term6),
            COLOUR(term7),
            COLOUR(term8),
            COLOUR(term9),
            COLOUR(term10),
            COLOUR(term11),
            COLOUR(term12),
            COLOUR(term13),
            COLOUR(term14),
            COLOUR(term15),
            COLOUR(rosewater),
            COLOUR(flamingo),
            COLOUR(pink),
            COLOUR(mauve),
            COLOUR(red),
            COLOUR(maroon),
            COLOUR(peach),
            COLOUR(yellow),
            COLOUR(green),
            COLOUR(teal),
            COLOUR(sky),
            COLOUR(sapphire),
            COLOUR(blue),
            COLOUR(lavender),
            COLOUR(klink),
            COLOUR(klinkSelection),
            COLOUR(kvisited),
            COLOUR(kvisitedSelection),
            COLOUR(knegative),
            COLOUR(knegativeSelection),
            COLOUR(kneutral),
            COLOUR(kneutralSelection),
            COLOUR(kpositive),
            COLOUR(kpositiveSelection),
        },
    };

#undef COLOUR
}

} // namespace

QFuture<Scheme> schemeForName(const QString& name, const QString& flavour, Scheme::Mode mode) {
    const auto path = u":/schemes/%1/%2/%3.txt"_s.arg(name, flavour, toString(mode));
    return QtFuture::makeReadyValueFuture(path).then(QtFuture::Launch::Async, [name, flavour, mode](const QString& p) {
        return parseScheme(p, name, flavour, mode);
    });
}

Scheme schemeForImage(const QImage& image, m3colours::scheme::Variant flavour, Scheme::Mode mode) {
    const auto colours = coloursForImage(image);
    return {
        .name = u"dynamic"_s,
        .flavour = toString(flavour),
        .mode = mode,
        .colours = gen::genScheme(colours[0], flavour, mode),
    };
}

QFuture<Scheme> schemeForPath(const QString& path, m3colours::scheme::Variant flavour, Scheme::Mode mode) {
    return coloursForPath(path).then([path, flavour, mode](const QList<m3colours::Argb>& colours) -> Scheme {
        const auto best = colours[0];
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
        u"%1|%2|%3"_s.arg(QString::number(sourceColour.value(), 16), toString(flavour), toString(mode)));
}

} // namespace caelestia::scheme
