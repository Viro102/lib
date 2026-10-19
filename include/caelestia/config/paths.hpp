#pragma once

#include <qstring.h>
#include <qtclasshelpermacros.h>

namespace caelestia::config {

class Config;

class ConfigPaths {
public:
    Q_DISABLE_COPY_MOVE(ConfigPaths)

    [[nodiscard]] static QString defaultWallpapersDir();
    [[nodiscard]] QString wallpapersDir() const;
    void setWallpapersDir(const QString& dir);

    [[nodiscard]] static QString defaultScreenshotsDir();
    [[nodiscard]] QString screenshotsDir() const;
    void setScreenshotsDir(const QString& dir);

    [[nodiscard]] static QString defaultRecordingsDir();
    [[nodiscard]] QString recordingsDir() const;
    void setRecordingsDir(const QString& dir);

private:
    explicit ConfigPaths(Config* config);

    Config* const m_config;

    friend Config;
};

} // namespace caelestia::config
