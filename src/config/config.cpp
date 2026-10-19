#include "caelestia/config/config.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qjsondocument.h>
#include <qlist.h>
#include <qloggingcategory.h>
#include <qsavefile.h>
#include <qstandardpaths.h>

namespace caelestia::config {

using Qt::StringLiterals::operator""_s;

namespace {

Q_LOGGING_CATEGORY(logConfig, "caelestia.config")

QJsonObject setNested(QJsonObject object, const QList<QStringView>& keys, qsizetype index, const QJsonValue& value) {
    const auto key = keys[index].toString();
    if (index == keys.size() - 1)
        object[key] = value;
    else
        object[key] = setNested(object[key].toObject(), keys, index + 1, value);
    return object;
}

} // namespace

Config& Config::instance() {
    static Config s_config;
    return s_config;
}

QString Config::configDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/caelestia"_s;
}

QString Config::configPath() {
    return configDir() + u"/config.json"_s;
}

Config::Config()
    : m_apps(this)
    , m_paths(this)
    , m_theme(this) {
    reload();
}

const ConfigApps& Config::apps() const {
    return m_apps;
}

const ConfigPaths& Config::paths() const {
    return m_paths;
}

const ConfigTheme& Config::theme() const {
    return m_theme;
}

QJsonObject Config::root() const {
    return m_root;
}

QJsonValue Config::value(QStringView path, const QJsonValue& fallback) const {
    QJsonValue current = m_root;
    const auto keys = path.split(u'.');
    for (const auto key : keys) {
        if (!current.isObject())
            return fallback;
        current = current.toObject().value(key);
    }
    return current.isUndefined() ? fallback : current;
}

void Config::reload() {
    m_root = {};

    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(logConfig) << "Unable to open" << file.fileName() << file.errorString();
        return;
    }

    QJsonParseError error{};
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(logConfig) << "Failed to parse" << file.fileName() << error.errorString();
        return;
    }
    if (!doc.isObject()) {
        qCWarning(logConfig) << "Root of" << file.fileName() << "is not an object";
        return;
    }

    m_root = doc.object();
}

bool Config::setValue(QStringView path, const QJsonValue& value) {
    const auto keys = path.split(u'.');
    if (keys.isEmpty() || keys.first().isEmpty())
        return false;

    m_root = setNested(m_root, keys, 0, value);
    return write();
}

bool Config::write() const {
    const auto path = configPath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        qCWarning(logConfig) << "Unable to create directory for" << path;
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(logConfig) << "Unable to open" << path << file.errorString();
        return false;
    }

    file.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qCWarning(logConfig) << "Unable to write" << path << file.errorString();
        return false;
    }
    return true;
}

} // namespace caelestia::config
