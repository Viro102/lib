#include "caelestia/wallpaper/wallpaper.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qloggingcategory.h>
#include <qsavefile.h>

#include <exception>
#include <stdexcept>

#include "caelestia/config/paths.hpp"
#include "caelestia/media/analysis.hpp"
#include "caelestia/scheme/scheme.hpp"
#include "caelestia/scheme/smart.hpp"
#include "caelestia/theme/theme.hpp"

namespace caelestia::wallpaper {

using Qt::StringLiterals::operator""_s;

QString statePath() {
    return config::ConfigPaths::stateDir() + u"/wallpaper/path.txt"_s;
}

namespace {

Q_LOGGING_CATEGORY(lcWallpaper, "caelestia.wallpaper", QtInfoMsg)

bool writeState(const QString& path) {
    QDir().mkpath(QFileInfo(statePath()).absolutePath());
    QSaveFile file(statePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(lcWallpaper) << "Unable to open" << file.fileName() << file.errorString();
        return false;
    }

    file.write(path.toUtf8());
    if (!file.commit()) {
        qCWarning(lcWallpaper) << "Unable to write" << file.fileName() << file.errorString();
        return false;
    }
    return true;
}

QFuture<scheme::SchemeData> dynamicScheme(const QString& path, const scheme::SchemeRef& state, bool smart) {
    return media::analyseForPath(path)
        .then([path, state, smart](const media::Analysis& analysis) {
            if (analysis.colours.isEmpty()) {
                qCWarning(lcWallpaper) << "No colours found in" << path << "- using default scheme";
                return scheme::defaultScheme();
            }

            const auto scheme = scheme::Scheme::forColour(analysis.colours.first());
            const auto choice = scheme::smartChoice(analysis);
            auto flavour = smart ? std::nullopt : scheme->flavour(state.flavour);
            if (!flavour) {
                if (!smart)
                    qCWarning(lcWallpaper) << "Unknown variant" << state.flavour << "- using smart variant";
                flavour = scheme->flavour(choice.flavour);
            }

            const auto mode = smart ? choice.mode : state.mode;
            return flavour->data(mode);
        })
        .unwrap();
}

QFuture<scheme::SchemeData> fileScheme(const scheme::SchemeRef& state) {
    return scheme::Scheme::forName(state.name)
        .then([state](const std::shared_ptr<scheme::Scheme>& scheme) {
            if (!scheme) {
                qCWarning(lcWallpaper) << "Scheme" << state.name << "no longer exists - using default scheme";
                return scheme::defaultScheme();
            }

            auto flavour = scheme->flavour(state.flavour);
            if (!flavour) {
                flavour = scheme->defaultFlavour();
                qCWarning(lcWallpaper) << "Flavour" << state.flavour << "for scheme" << state.name
                                       << "no longer exists - using" << flavour->name();
            }

            auto mode = state.mode;
            if (!flavour->modes().contains(mode)) {
                mode = flavour->defaultMode();
                qCWarning(lcWallpaper) << "Mode" << scheme::modeName(state.mode) << "for scheme"
                                       << u"%1 (%2)"_s.arg(state.name, flavour->name()) << "no longer exists - using"
                                       << scheme::modeName(mode);
            }

            return flavour->data(mode);
        })
        .unwrap();
}

QFuture<scheme::SchemeData> resolveScheme(const QString& path, bool smart) {
    const auto state = scheme::loadStateRef();
    if (!state)
        return scheme::defaultScheme();
    if (state->name == u"dynamic"_s)
        return dynamicScheme(path, *state, smart);
    return fileScheme(*state);
}

} // namespace

QFuture<scheme::SchemeData> currentScheme(bool* ok) {
    auto state = scheme::loadState();
    if (ok)
        *ok = state.has_value();
    if (state)
        return QtFuture::makeReadyValueFuture(std::move(*state));

    const auto path = currentWallpaper();
    qCWarning(lcWallpaper) << "Scheme state's colours are invalid - regenerating from" << path;
    return resolveScheme(path, false);
}

QString defaultWallpaper() {
    return QStringLiteral(CAELESTIA_DEFAULT_WALLPAPER);
}

QString currentWallpaper(bool* ok) {
    QFile file(statePath());
    if (!file.open(QIODevice::ReadOnly))
        return defaultWallpaper();

    const auto path = QString::fromUtf8(file.readAll()).trimmed();
    const auto pathValid = media::isSupported(path);
    if (ok)
        *ok = pathValid;
    return pathValid ? path : defaultWallpaper();
}

QFuture<void> set(const QString& path, bool smart) {
    const auto absolute = QFileInfo(path).absoluteFilePath();

    if (!media::isSupported(absolute))
        return QtFuture::makeExceptionalFuture<void>(std::make_exception_ptr(
            std::invalid_argument(u"%1 is not a supported image or video"_s.arg(absolute).toStdString())));

    if (!writeState(absolute))
        return QtFuture::makeExceptionalFuture<void>(
            std::make_exception_ptr(std::runtime_error("Unable to write wallpaper state")));

    return resolveScheme(absolute, smart)
        .then([](const scheme::SchemeData& data) {
            return theme::Theme(data).apply();
        })
        .unwrap();
}

} // namespace caelestia::wallpaper
