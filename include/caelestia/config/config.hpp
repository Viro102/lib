#pragma once

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qstring.h>
#include <qtclasshelpermacros.h>

#include "caelestia/config/paths.hpp"

namespace caelestia::config {

class Config {
public:
    Q_DISABLE_COPY_MOVE(Config)

    static Config& instance();

    [[nodiscard]] static QString configPath();

    [[nodiscard]] QJsonObject root() const;
    [[nodiscard]] QJsonValue value(QStringView path, const QJsonValue& fallback = QJsonValue::Undefined) const;

    void reload();
    bool setValue(QStringView path, const QJsonValue& value);

    // Config options
    [[nodiscard]] ConfigPaths paths() const;

private:
    explicit Config();

    [[nodiscard]] bool write() const;

    QJsonObject m_root;

    ConfigPaths const m_paths;
};

} // namespace caelestia::config
