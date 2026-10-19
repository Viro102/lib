#pragma once

#include <qstring.h>
#include <qtclasshelpermacros.h>

namespace caelestia::config {

class Config;

class ConfigTheme {
public:
    Q_DISABLE_COPY_MOVE(ConfigTheme)


    [[nodiscard]] bool targetEnabled(QStringView target) const;
    void setTargetEnabled(QStringView target, bool enabled);

    [[nodiscard]] QString iconTheme() const;
    void setIconTheme(const QString& iconTheme);

    [[nodiscard]] QString iconThemeLight() const;
    void setIconThemeLight(const QString& iconTheme);

    [[nodiscard]] QString iconThemeDark() const;
    void setIconThemeDark(const QString& iconTheme);

    [[nodiscard]] QString postHook() const;
    void setPostHook(const QString& hook);

private:
    explicit ConfigTheme(Config* config);

    Config* const m_config;

    friend Config;
};

} // namespace caelestia::config
