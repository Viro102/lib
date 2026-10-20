#include "caelestia/scheme/scheme.hpp"

#include <qdir.h>
#include <qfuture.h>
#include <qset.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

#include "caelestia/config/paths.hpp"
#include "caelestia/media/analysis.hpp"
#include "colournames.hpp"
#include "dynamicscheme.hpp"
#include "filescheme.hpp"
#include "logging.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

Q_LOGGING_CATEGORY(lcScheme, "caelestia.scheme", QtInfoMsg)

namespace {

QStringList schemeRoots() {
    return { config::ConfigPaths::customSchemesDir(), u":/schemes"_s };
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

Flavour::Flavour(std::shared_ptr<const Scheme> scheme, QString name)
    : m_scheme(std::move(scheme))
    , m_name(std::move(name)) {}

const QString& Flavour::name() const {
    return m_name;
}

QList<Mode> Flavour::modes() const {
    return m_scheme->modes(m_name);
}

QFuture<SchemeData> Flavour::data(Mode mode) const {
    return m_scheme->load(m_name, mode);
}

Scheme::Scheme(QString name, QStringList flavours)
    : m_name(std::move(name))
    , m_flavours(std::move(flavours)) {}

QFuture<QList<std::shared_ptr<Scheme>>> Scheme::all() {
    return QtFuture::makeReadyVoidFuture().then(QtFuture::Launch::Async, [] {
        const auto roots = schemeRoots();
        QSet<QString> nameSet;
        for (const auto& root : roots) {
            const auto names = QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            nameSet.unite(QSet<QString>(names.cbegin(), names.cend()));
        }

        QStringList names(nameSet.cbegin(), nameSet.cend());
        std::sort(names.begin(), names.end());

        QList<std::shared_ptr<Scheme>> schemes;
        for (const auto& name : std::as_const(names)) {
            if (auto scheme = FileScheme::open(roots, name))
                schemes.append(std::move(scheme));
        }
        return schemes;
    });
}

QFuture<std::shared_ptr<Scheme>> Scheme::forName(const QString& name) {
    if (name.isEmpty() || name.contains(u'/'))
        return QtFuture::makeReadyValueFuture(std::shared_ptr<Scheme>());

    return QtFuture::makeReadyValueFuture(name).then(QtFuture::Launch::Async, [](const QString& n) {
        return FileScheme::open(schemeRoots(), n);
    });
}

std::shared_ptr<Scheme> Scheme::forColour(m3colours::Argb colour) {
    return std::make_shared<DynamicScheme>(colour);
}

QFuture<std::shared_ptr<Scheme>> Scheme::forMedia(const QString& path) {
    return media::analyseForPath(path).then([](const media::Analysis& analysis) -> std::shared_ptr<Scheme> {
        if (analysis.colours.isEmpty())
            return nullptr;
        return forColour(analysis.colours.first());
    });
}

const QString& Scheme::name() const {
    return m_name;
}

QList<Flavour> Scheme::flavours() const {
    QList<Flavour> flavours;
    flavours.reserve(m_flavours.size());
    for (const auto& name : m_flavours)
        flavours.append(Flavour(shared_from_this(), name));
    return flavours;
}

std::optional<Flavour> Scheme::flavour(const QString& name) const {
    if (!m_flavours.contains(name))
        return std::nullopt;
    return Flavour(shared_from_this(), name);
}

QString modeName(Mode mode) {
    switch (mode) {
    case Mode::Light:
        return u"light"_s;
    case Mode::Dark:
        return u"dark"_s;
    }
    Q_UNREACHABLE_RETURN(QString());
}

QList<NamedColour> namedColours(const SchemeColours& colours) {
#define COLOUR(name) NamedColour{ u## #name, colours.name },
    return { CAELESTIA_SCHEME_COLOURS(COLOUR) };
#undef COLOUR
}

} // namespace caelestia::scheme
