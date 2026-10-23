#include "dynamicscheme.hpp"

#include <qbytearray.h>
#include <qdatastream.h>
#include <qiodevice.h>
#include <qstringlist.h>

#include <array>
#include <stdexcept>
#include <utility>

#include "caelestia/scheme/generator.hpp"
#include "colournames.hpp"
#include "logging.hpp"
#include "schemecache.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

std::optional<SchemeColours> coloursFromBytes(const QByteArray& bytes) {
    QDataStream stream(bytes);
    stream.setVersion(QDataStream::Qt_6_0);
    const auto colour = [&stream] {
        QRgb rgba = 0;
        stream >> rgba;
        return QColor::fromRgba(rgba);
    };

#define COLOUR(name) .name = colour(),
    const SchemeColours colours{ CAELESTIA_SCHEME_COLOURS(COLOUR) };
#undef COLOUR

    if (stream.status() != QDataStream::Ok || !stream.atEnd()) {
        qCWarning(lcScheme) << "Cached scheme has unexpected size:" << bytes.size();
        return std::nullopt;
    }
    return colours;
}

QByteArray coloursToBytes(const SchemeColours& colours) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
#define COLOUR(name) stream << colours.name.rgba();
    CAELESTIA_SCHEME_COLOURS(COLOUR)
#undef COLOUR
    return bytes;
}

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 2;

constexpr std::array k_variants = {
    std::pair{ DynamicVariant::Monochrome, QStringView(u"monochrome") },
    std::pair{ DynamicVariant::Neutral, QStringView(u"neutral") },
    std::pair{ DynamicVariant::TonalSpot, QStringView(u"tonalspot") },
    std::pair{ DynamicVariant::Vibrant, QStringView(u"vibrant") },
    std::pair{ DynamicVariant::Expressive, QStringView(u"expressive") },
    std::pair{ DynamicVariant::Fidelity, QStringView(u"fidelity") },
    std::pair{ DynamicVariant::Content, QStringView(u"content") },
    std::pair{ DynamicVariant::Rainbow, QStringView(u"rainbow") },
    std::pair{ DynamicVariant::FruitSalad, QStringView(u"fruitsalad") },
    std::pair{ DynamicVariant::Cmf, QStringView(u"cmf") },
};

QString toString(DynamicVariant variant) {
    for (const auto& [v, name] : k_variants) {
        if (v == variant)
            return name.toString();
    }
    Q_UNREACHABLE_RETURN(QString());
}

DynamicVariant toVariant(QStringView name) {
    for (const auto& [variant, n] : k_variants) {
        if (n == name)
            return variant;
    }
    throw std::invalid_argument(u"No such variant: %1"_s.arg(name).toStdString());
}

QStringList variantNames() {
    QStringList names;
    names.reserve(k_variants.size());
    for (const auto& [variant, name] : k_variants)
        names.append(name.toString());
    return names;
}

} // namespace

DynamicScheme::DynamicScheme(QColor colour)
    : Scheme(u"dynamic"_s, variantNames())
    , m_colour(colour) {}

QList<Mode> DynamicScheme::modes(const QString& /*flavour*/) const {
    return { Mode::Light, Mode::Dark };
}

QFuture<SchemeData> DynamicScheme::load(const QString& flavour, Mode mode) const {
    const auto variant = toVariant(flavour);
    const auto key = SchemeCache::keyFor(m_colour, variant, mode);
    return SchemeCache::instance().lookup(key).then(QtFuture::Launch::Async,
        [colour = m_colour, flavour, variant, mode, key](const QByteArray& cached) -> SchemeData {
            if (!cached.isEmpty()) {
                if (const auto colours = coloursFromBytes(cached))
                    return { .name = u"dynamic"_s, .flavour = flavour, .mode = mode, .colours = *colours };
            }

            const auto colours = generate(colour, variant, mode);
            SchemeCache::instance().store(key, coloursToBytes(colours));
            return { .name = u"dynamic"_s, .flavour = flavour, .mode = mode, .colours = colours };
        });
}

SchemeCache::SchemeCache()
    : BlobCache(u"schemes"_s, k_maxSize, k_cacheVersion) {}

SchemeCache& SchemeCache::instance() {
    static SchemeCache s_instance;
    return s_instance;
}

QString SchemeCache::keyFor(QColor sourceColour, DynamicVariant flavour, Mode mode) {
    return utils::Cache::keyFor(
        u"%1|%2|%3"_s.arg(QString::number(sourceColour.rgba(), 16), toString(flavour), modeName(mode)));
}

} // namespace caelestia::scheme
