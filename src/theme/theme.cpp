#include "caelestia/theme/theme.hpp"

#include <qcolor.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qhash.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlockfile.h>
#include <qprocess.h>
#include <qregularexpression.h>
#include <qsavefile.h>

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "caelestia/config/config.hpp"
#include "util.hpp"

namespace caelestia::theme {

using Qt::StringLiterals::operator""_s;

namespace {

struct Hsl {
    int hue;
    int saturation;
    int lightness;
};

Hsl toHsl(m3colours::Argb colour) {
    const auto qc = QColor::fromRgb(colour.red(), colour.green(), colour.blue());
    return {
        .hue = std::max(0, qc.hslHue()),
        .saturation = qRound(qc.hslSaturationF() * 100),
        .lightness = qRound(qc.lightnessF() * 100),
    };
}

std::optional<QString> colourForm(m3colours::Argb colour, QStringView form) {
    if (form == u"hex")
        return hex(colour);
    if (form == u"hexalpha")
        return hex(colour) + u"%1"_s.arg(colour.alpha(), 2, 16, u'0');
    if (form == u"rgb")
        return u"rgb(%1,%2,%3)"_s.arg(colour.red()).arg(colour.green()).arg(colour.blue());
    if (form == u"rgbalpha")
        return u"rgba(%1,%2,%3,%4)"_s.arg(colour.red()).arg(colour.green()).arg(colour.blue()).arg(colour.alpha());
    if (form == u"red")
        return QString::number(colour.red());
    if (form == u"green")
        return QString::number(colour.green());
    if (form == u"blue")
        return QString::number(colour.blue());

    const auto hsl = toHsl(colour);
    if (form == u"hsl")
        return u"hsl(%1,%2%,%3%)"_s.arg(hsl.hue).arg(hsl.saturation).arg(hsl.lightness);
    if (form == u"hue")
        return QString::number(hsl.hue);
    if (form == u"saturation")
        return QString::number(hsl.saturation);
    if (form == u"lightness")
        return QString::number(hsl.lightness);
    return std::nullopt;
}

std::optional<QString> fill(QStringView field, const QHash<QStringView, m3colours::Argb>& colours,
    const QHash<QStringView, QString>& variables) {
    const auto inner = field.trimmed();
    if (const auto it = variables.constFind(inner); it != variables.cend())
        return *it;

    const auto parts = inner.split(u'.');
    if (parts.size() != 2)
        return std::nullopt;

    const auto it = colours.constFind(parts[0]);
    if (it == colours.cend())
        return std::nullopt;
    return colourForm(*it, parts[1]);
}

std::optional<QString> select(QStringView field, const auto& resolve) {
    const auto question = field.indexOf(u'?');
    const auto subject = resolve(field.first(question).trimmed());
    if (!subject)
        return std::nullopt;

    const auto arms = field.sliced(question + 1).split(u" | ");
    for (const auto arm : arms) {
        const auto branch = arm.trimmed();
        const auto equals = branch.indexOf(u'=');
        if (equals < 0)
            return resolve(branch).value_or(branch.toString());
        if (branch.first(equals).trimmed() == *subject) {
            const auto value = branch.sliced(equals + 1).trimmed();
            return resolve(value).value_or(value.toString());
        }
    }
    return QString();
}

QString defaultIconTheme(scheme::Mode mode) {
    const auto& cfg = config::Config::instance().theme();
    return mode == scheme::Mode::Dark ? cfg.iconThemeDark() : cfg.iconThemeLight();
}

QString schemeStatePath() {
    return config::ConfigPaths::stateDir() + u"/scheme.bin"_s;
}

} // namespace

Theme::Theme(scheme::SchemeData scheme)
    : m_scheme(std::move(scheme))
    , m_iconTheme(defaultIconTheme(m_scheme.mode)) {}

bool Theme::saveState(const scheme::SchemeData& scheme) {
    QDir().mkpath(config::ConfigPaths::stateDir());
    QSaveFile file(schemeStatePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcTheme) << "Unable to open" << file.fileName() << file.errorString();
        return false;
    }

    QDataStream out(&file);
    out << scheme.name << scheme.flavour << scheme.mode << scheme::coloursToBytes(scheme.colours);
    if (!file.commit()) {
        qCWarning(lcTheme) << "Unable to write" << file.fileName() << file.errorString();
        return false;
    }
    return true;
}

std::optional<scheme::SchemeData> Theme::loadState() {
    QFile file(schemeStatePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcTheme) << "Unable to open" << file.fileName() << file.errorString();
        return std::nullopt;
    }

    QString name;
    QString flavour;
    scheme::Mode mode;
    QByteArray colours;
    QDataStream in(&file);
    in >> name >> flavour >> mode >> colours;
    if (in.status() != QDataStream::Ok) {
        qCWarning(lcTheme) << "Unable to read" << file.fileName();
        return std::nullopt;
    }

    const auto parsed = scheme::coloursFromBytes(colours);
    if (!parsed)
        return std::nullopt;
    return scheme::SchemeData{ .name = name, .flavour = flavour, .mode = mode, .colours = *parsed };
}

QFuture<void> Theme::apply() const {
    return QtFuture::makeReadyVoidFuture().then(QtFuture::Launch::Async, [theme = *this] {
        theme.applySync();
    });
}

QString Theme::render(const QString& templatePath) const {
    QFile file(templatePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcTheme) << "Unable to open template" << templatePath << file.errorString();
        return {};
    }
    const auto content = QString::fromUtf8(file.readAll());

    QHash<QStringView, m3colours::Argb> colours;
    const auto named = scheme::namedColours(m_scheme.colours);
    for (const auto& [name, colour] : named)
        colours.insert(name, colour);

    const QHash<QStringView, QString> variables = {
        { u"mode", scheme::modeName(m_scheme.mode) },
        { u"iconTheme", m_iconTheme },
    };
    const auto resolve = [&](QStringView field) {
        return fill(field, colours, variables);
    };

    static const QRegularExpression k_field(uR"(\{\{((?:(?!\{\{|\}\}).)*)\}\})"_s);

    QString out;
    qsizetype last = 0;
    for (auto it = k_field.globalMatch(content); it.hasNext();) {
        const auto match = it.next();
        out += QStringView(content).sliced(last, match.capturedStart() - last);
        last = match.capturedEnd();

        const auto field = match.capturedView(1).trimmed();
        const auto value = field.contains(u'?') ? select(field, resolve) : resolve(field);
        out += value ? QStringView(*value) : match.capturedView();
    }
    out += QStringView(content).sliced(last);

    return out;
}

QString Theme::renderTemplate(const QString& name) const {
    return render(u":/templates/"_s + name);
}

void Theme::writeTemplate(const QString& name, const QString& path) const {
    writeFile(path, renderTemplate(name));
}

void Theme::applySync() const {
    QDir().mkpath(config::ConfigPaths::stateDir());
    const bool saved = saveState(m_scheme);

    QLockFile lock(config::ConfigPaths::stateDir() + u"/theme.lock"_s);
    lock.lock();

    if (saved) {
        if (auto latest = loadState()) {
            Theme(std::move(*latest)).applyTargets();
            return;
        }
    }
    applyTargets();
}

void Theme::applyTargets() const {
    struct Target {
        QStringView name;
        void (Theme::*apply)() const;
    };

    static constexpr std::array k_targets = {
        Target{ .name = u"terminal", .apply = &Theme::applyTerminal },
        Target{ .name = u"hypr", .apply = &Theme::applyHypr },
        Target{ .name = u"discord", .apply = &Theme::applyDiscord },
        Target{ .name = u"spicetify", .apply = &Theme::applySpicetify },
        Target{ .name = u"pandora", .apply = &Theme::applyPandora },
        Target{ .name = u"fuzzel", .apply = &Theme::applyFuzzel },
        Target{ .name = u"btop", .apply = &Theme::applyBtop },
        Target{ .name = u"nvtop", .apply = &Theme::applyNvtop },
        Target{ .name = u"htop", .apply = &Theme::applyHtop },
        Target{ .name = u"gtk", .apply = &Theme::applyGtk },
        Target{ .name = u"qt", .apply = &Theme::applyQt },
        Target{ .name = u"warp", .apply = &Theme::applyWarp },
        Target{ .name = u"chromium", .apply = &Theme::applyChromium },
        Target{ .name = u"zed", .apply = &Theme::applyZed },
        Target{ .name = u"cava", .apply = &Theme::applyCava },
    };

    const auto& cfg = config::Config::instance().theme();
    for (const auto& target : k_targets) {
        if (!cfg.targetEnabled(target.name))
            continue;
        qCDebug(lcTheme) << "Applying" << target.name;
        (this->*target.apply)();
    }

    applyCustom();

    if (const auto hook = cfg.postHook(); !hook.isEmpty())
        runPostHook(hook);
}

void Theme::applyCustom() const {
    const auto dir = config::ConfigPaths::customThemesDir();
    QFile file(dir + u"/templates.json"_s);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (!doc.isArray()) {
        qCWarning(lcTheme) << "Invalid" << file.fileName() << error.errorString();
        return;
    }

    const auto entries = doc.array();
    for (const auto& entry : entries) {
        const auto obj = entry.toObject();
        const auto from = obj[u"from"_s].toString();
        const auto to = obj[u"to"_s].toString();

        if (from.isEmpty() || to.isEmpty()) {
            qCWarning(lcTheme) << "Template entry missing from/to:" << entry;
            continue;
        }

        const auto source = config::ConfigPaths::resolvePath(from, dir);
        if (!QFileInfo::exists(source)) {
            qCWarning(lcTheme) << "Template" << source << "does not exist, skipping";
            continue;
        }

        writeFile(config::ConfigPaths::resolvePath(to, QDir::homePath()), render(source));
    }
}

void Theme::runPostHook(const QString& hook) const {
    QJsonObject colours;
    const auto named = scheme::namedColours(m_scheme.colours);
    for (const auto& [name, colour] : named)
        colours[name.toString()] = hex(colour);

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(u"SCHEME_NAME"_s, m_scheme.name);
    env.insert(u"SCHEME_FLAVOUR"_s, m_scheme.flavour);
    env.insert(u"SCHEME_MODE"_s, scheme::modeName(m_scheme.mode));
    env.insert(u"SCHEME_COLOURS"_s, QString::fromUtf8(QJsonDocument(colours).toJson(QJsonDocument::Compact)));

    QProcess process;
    process.setProcessEnvironment(env);
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(u"sh"_s, { u"-c"_s, hook });
    process.waitForFinished(-1);
}

} // namespace caelestia::theme
