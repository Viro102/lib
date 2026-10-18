#include "caelestia/scheme/scheme.hpp"

#include <qfile.h>
#include <qloggingcategory.h>
#include <qtextstream.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <optional>
#include <type_traits>

#include "generator.hpp"
#include "schemecache.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

Q_LOGGING_CATEGORY(lcScheme, "caelestia.scheme", QtInfoMsg)

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

} // namespace

QFuture<Scheme> schemeForName(const QString& name, const QString& flavour, Scheme::Mode mode) {
    // TODO
}

QFuture<Scheme> schemeForPath(const QString& path, m3colours::scheme::Variant flavour, Scheme::Mode mode) {
    auto& cache = SchemeCache::instance();
    return coloursFor(path).then([&cache, path, flavour, mode](const QList<m3colours::Argb>& colours) -> Scheme {
        const auto best = colours[0];
        auto future = cache.lookup(SchemeCache::keyFor(best, flavour, mode));
        future.waitForFinished();

        const auto res = future.result();
        if (!res.isEmpty()) {
            if (const auto cached = coloursFromBytes(res))
                return { .name = path, .flavour = toString(flavour), .mode = mode, .colours = *cached };
        }

        const auto gen = gen::genScheme(best, flavour, mode);
        cache.store(SchemeCache::keyFor(best, flavour, mode), coloursToBytes(gen));

        return { .name = path, .flavour = toString(flavour), .mode = mode, .colours = gen };
    });
}

SchemeCache::SchemeCache()
    : BlobCache(u"schemes"_s, 1024, k_cacheVersion) {}

SchemeCache& SchemeCache::instance() {
    static SchemeCache s_instance;
    return s_instance;
}

QString SchemeCache::keyFor(m3colours::Argb sourceColour, m3colours::scheme::Variant flavour, Scheme::Mode mode) {
    return utils::Cache::keyFor(
        u"%1|%2|%3"_s.arg(QString::number(sourceColour.value(), 16), toString(flavour), toString(mode)));
}

} // namespace caelestia::scheme
