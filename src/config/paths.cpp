#include "caelestia/config/paths.hpp"

#include <qdir.h>
#include <qstandardpaths.h>

#include "caelestia/config/config.hpp"

namespace caelestia::config {

using Qt::StringLiterals::operator""_s;

namespace {

QString resolvePath(QString path, const QString& defaultPath) {
    if (path.isEmpty())
        return defaultPath;

    if (path == u'~' || path.startsWith(u"~/"_s))
        path = QDir::homePath() + path.mid(1);
    return QDir::cleanPath(QDir(defaultPath).absoluteFilePath(path));
}

} // namespace

ConfigPaths::ConfigPaths(Config* config)
    : m_config(config) {}

QString ConfigPaths::defaultWallpapersDir() {
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + u"/Wallpapers"_s;
}

QString ConfigPaths::wallpapersDir() const {
    return resolvePath(m_config->value(u"paths.wallpapersDir"_s).toString(), defaultWallpapersDir());
}

void ConfigPaths::setWallpapersDir(const QString& dir) {
    m_config->setValue(u"paths.wallpapersDir"_s, dir);
}

QString ConfigPaths::defaultScreenshotsDir() {
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + u"/Screenshots"_s;
}

QString ConfigPaths::screenshotsDir() const {
    return resolvePath(m_config->value(u"paths.screenshotsDir"_s).toString(), defaultScreenshotsDir());
}

void ConfigPaths::setScreenshotsDir(const QString& dir) {
    m_config->setValue(u"paths.screenshotsDir"_s, dir);
}

QString ConfigPaths::defaultRecordingsDir() {
    return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + u"/Recordings"_s;
}

QString ConfigPaths::recordingsDir() const {
    return resolvePath(m_config->value(u"paths.recordingsDir"_s).toString(), defaultRecordingsDir());
}

void ConfigPaths::setRecordingsDir(const QString& dir) {
    m_config->setValue(u"paths.recordingsDir"_s, dir);
}

} // namespace caelestia::config
