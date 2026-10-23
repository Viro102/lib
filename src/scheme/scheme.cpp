#include "caelestia/scheme/scheme.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qfuture.h>
#include <qjsondocument.h>
#include <qjsonvalue.h>
#include <qsavefile.h>
#include <qset.h>

#include <algorithm>
#include <memory>
#include <optional>
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

std::optional<QColor> colourFromJson(const QJsonValue& value) {
    const auto text = value.toString();
    bool ok = false;
    const auto rgb = text.toUInt(&ok, 16);
    if (!ok || text.size() != 6)
        return std::nullopt;
    return QColor::fromRgba(0xFF000000u | rgb);
}

std::optional<Mode> modeFromName(const QString& name) {
    if (name == u"light"_s)
        return Mode::Light;
    if (name == u"dark"_s)
        return Mode::Dark;
    return std::nullopt;
}

} // namespace

QJsonObject toJson(const SchemeData& data) {
    QJsonObject colours;
    const auto named = namedColours(data.colours);
    for (const auto& [name, colour] : named)
        colours.insert(name.toString(), colour.name().sliced(1));

    return {
        { u"name"_s, data.name },
        { u"flavour"_s, data.flavour },
        { u"mode"_s, modeName(data.mode) },
        { u"colours"_s, colours },
    };
}

QString statePath() {
    return config::ConfigPaths::stateDir() + u"/scheme.json"_s;
}

std::optional<SchemeData> fromJson(const QJsonObject& json) {
    const auto name = json[u"name"_s].toString();
    const auto flavour = json[u"flavour"_s].toString();
    const auto mode = modeFromName(json[u"mode"_s].toString());
    const auto colours = json[u"colours"_s].toObject();
    if (name.isEmpty() || flavour.isEmpty() || !mode) {
        qCWarning(lcScheme) << "Scheme state is missing name, flavour or mode";
        return std::nullopt;
    }

    bool complete = true;
    const auto colour = [&](const QString& key) -> QColor {
        const auto parsed = colourFromJson(colours[key]);
        if (!parsed) {
            qCWarning(lcScheme) << "Scheme state is missing colour" << key;
            complete = false;
            return {};
        }
        return *parsed;
    };

#define COLOUR(name) .name = colour(u## #name ""_s),
    const SchemeColours result{ CAELESTIA_SCHEME_COLOURS(COLOUR) };
#undef COLOUR

    if (!complete)
        return std::nullopt;
    return SchemeData{ .name = name, .flavour = flavour, .mode = *mode, .colours = result };
}

QFuture<SchemeData> defaultScheme() {
    return Scheme::forName(u"caelestia"_s)
        .then([](const std::shared_ptr<Scheme>& scheme) {
            return scheme->flavour(u"default"_s)->data(Mode::Dark);
        })
        .unwrap();
}

bool saveState(const SchemeData& data) {
    QDir().mkpath(config::ConfigPaths::stateDir());
    QSaveFile file(statePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcScheme) << "Unable to open" << file.fileName() << file.errorString();
        return false;
    }

    file.write(QJsonDocument(toJson(data)).toJson(QJsonDocument::Compact));
    if (!file.commit()) {
        qCWarning(lcScheme) << "Unable to write" << file.fileName() << file.errorString();
        return false;
    }

    return true;
}

std::optional<SchemeData> loadState() {
    QFile file(statePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcScheme) << "Unable to open" << file.fileName() << file.errorString();
        return std::nullopt;
    }

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (!doc.isObject()) {
        qCWarning(lcScheme) << "Invalid" << file.fileName() << error.errorString();
        return std::nullopt;
    }

    return fromJson(doc.object());
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

Mode Flavour::defaultMode() const {
    const auto available = modes();
    return available.contains(Mode::Dark) ? Mode::Dark : available.first();
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
        std::ranges::sort(names);

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

std::shared_ptr<Scheme> Scheme::forColour(QColor colour) {
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

Flavour Scheme::defaultFlavour() const {
    return { shared_from_this(), m_flavours.first() };
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
