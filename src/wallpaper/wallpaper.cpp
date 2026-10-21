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
#include "caelestia/theme/theme.hpp"

namespace caelestia::wallpaper {

using Qt::StringLiterals::operator""_s;

namespace {

Q_LOGGING_CATEGORY(lcWallpaper, "caelestia.wallpaper", QtInfoMsg)

constexpr double k_neutralColourfulness = 10;
constexpr double k_contentColourfulness = 20;
constexpr double k_lightTone = 60;

QString statePath() {
    return config::ConfigPaths::stateDir() + u"/wallpaper/path.txt"_s;
}

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

QString smartFlavour(const media::Analysis& analysis) {
    if (analysis.colourfulness < k_neutralColourfulness)
        return u"neutral"_s;
    if (analysis.colourfulness < k_contentColourfulness)
        return u"content"_s;
    return u"tonalspot"_s;
}

scheme::Mode smartMode(const media::Analysis& analysis) {
    return analysis.tone > k_lightTone ? scheme::Mode::Light : scheme::Mode::Dark;
}

QFuture<scheme::SchemeData> dynamicScheme(const QString& path, const scheme::SchemeData& state, bool smart) {
    return media::analyseForPath(path)
        .then([state, smart](const media::Analysis& analysis) {
            if (analysis.colours.isEmpty())
                return QtFuture::makeReadyValueFuture(state);

            const auto flavourName = smart ? smartFlavour(analysis) : state.flavour;
            const auto mode = smart ? smartMode(analysis) : state.mode;
            const auto flavour = scheme::Scheme::forColour(analysis.colours.first())->flavour(flavourName);
            return flavour ? flavour->data(mode) : QtFuture::makeReadyValueFuture(state);
        })
        .unwrap();
}

QFuture<scheme::SchemeData> fileScheme(const scheme::SchemeData& state) {
    return scheme::Scheme::forName(state.name)
        .then([state](const std::shared_ptr<scheme::Scheme>& scheme) {
            const auto flavour = scheme ? scheme->flavour(state.flavour) : std::nullopt;
            if (!flavour || !flavour->modes().contains(state.mode))
                return QtFuture::makeReadyValueFuture(state);
            return flavour->data(state.mode);
        })
        .unwrap();
}

QFuture<scheme::SchemeData> resolveScheme(const QString& path, bool smart) {
    const auto state = scheme::loadState();
    if (!state)
        return scheme::defaultScheme();
    if (state->name == u"dynamic"_s)
        return dynamicScheme(path, *state, smart);
    return fileScheme(*state);
}

} // namespace

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
