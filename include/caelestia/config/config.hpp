#pragma once

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qstring.h>
#include <qtclasshelpermacros.h>

#include "caelestia/config/apps.hpp"
#include "caelestia/config/paths.hpp"
#include "caelestia/config/theme.hpp"

namespace caelestia::config {

class Config {
public:
    Q_DISABLE_COPY_MOVE(Config)

    static Config& instance();

    [[nodiscard]] static QString configDir();
    [[nodiscard]] static QString configPath();

    [[nodiscard]] QJsonObject root() const;
    [[nodiscard]] QJsonValue value(QStringView path, const QJsonValue& fallback = QJsonValue::Undefined) const;

    void reload();
    bool setValue(QStringView path, const QJsonValue& value);

    // Config options
    [[nodiscard]] const ConfigApps& apps() const;
    [[nodiscard]] const ConfigPaths& paths() const;
    [[nodiscard]] const ConfigTheme& theme() const;

private:
    explicit Config();

    [[nodiscard]] bool write() const;

    QJsonObject m_root;

    ConfigApps const m_apps;
    ConfigPaths const m_paths;
    ConfigTheme const m_theme;
};

} // namespace caelestia::config
