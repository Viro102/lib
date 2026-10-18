#include "caelestia/config/paths.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qjsondocument.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qsavefile.h>
#include <qstandardpaths.h>

#include "caelestia/config/config.hpp"

namespace caelestia::config {

using Qt::StringLiterals::operator""_s;

ConfigPaths::ConfigPaths(Config* config)
    : m_config(config) {}

QString ConfigPaths::wallpapersDir() const {
    return m_config->value(u"paths.wallpapersDir"_s).toString();
}

void ConfigPaths::setWallpapersDir(const QString& dir) {
    m_config->setValue(u"paths.wallpapersDir"_s, dir);
}

QString ConfigPaths::screenshotsDir() const {
    return m_config->value(u"paths.screenshotsDir"_s).toString();
}

void ConfigPaths::setScreenshotsDir(const QString& dir) {
    m_config->setValue(u"paths.screenshotsDir"_s, dir);
}

QString ConfigPaths::recordingsDir() const {
    return m_config->value(u"paths.recordingsDir"_s).toString();
}

void ConfigPaths::setRecordingsDir(const QString& dir) {
    m_config->setValue(u"paths.recordingsDir"_s, dir);
}

} // namespace caelestia::config
