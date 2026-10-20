#include "dynamicscheme.hpp"

#include <qbytearray.h>
#include <qstringlist.h>

#include <array>
#include <stdexcept>
#include <utility>

#include "caelestia/scheme/generator.hpp"
#include "schemecache.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

using m3colours::scheme::Variant;

constexpr int k_maxSize = 1024 * 1024;
constexpr int k_cacheVersion = 1;

constexpr std::array k_variants = {
    std::pair{ Variant::Monochrome, QStringView(u"monochrome") },
    std::pair{ Variant::Neutral, QStringView(u"neutral") },
    std::pair{ Variant::TonalSpot, QStringView(u"tonalspot") },
    std::pair{ Variant::Vibrant, QStringView(u"vibrant") },
    std::pair{ Variant::Expressive, QStringView(u"expressive") },
    std::pair{ Variant::Fidelity, QStringView(u"fidelity") },
    std::pair{ Variant::Content, QStringView(u"content") },
    std::pair{ Variant::Rainbow, QStringView(u"rainbow") },
    std::pair{ Variant::FruitSalad, QStringView(u"fruitsalad") },
    std::pair{ Variant::Cmf, QStringView(u"cmf") },
};

QString toString(Variant variant) {
    for (const auto& [v, name] : k_variants) {
        if (v == variant)
            return name.toString();
    }
    Q_UNREACHABLE_RETURN(QString());
}

Variant toVariant(QStringView name) {
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

DynamicScheme::DynamicScheme(m3colours::Argb colour)
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

QString SchemeCache::keyFor(m3colours::Argb sourceColour, Variant flavour, Mode mode) {
    return utils::Cache::keyFor(
        u"%1|%2|%3"_s.arg(QString::number(sourceColour.value(), 16), toString(flavour), modeName(mode)));
}

} // namespace caelestia::scheme
