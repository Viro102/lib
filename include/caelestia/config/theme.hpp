#pragma once

#include <qstring.h>
#include <qtclasshelpermacros.h>

namespace caelestia::config {

class Config;

class ConfigTheme {
public:
    Q_DISABLE_COPY_MOVE(ConfigTheme)

    [[nodiscard]] bool targetEnabled(QStringView target) const;
    void setTargetEnabled(QStringView target, bool enabled) const;

    [[nodiscard]] QString iconTheme() const;
    void setIconTheme(const QString& iconTheme) const;

    [[nodiscard]] QString iconThemeLight() const;
    void setIconThemeLight(const QString& iconTheme) const;

    [[nodiscard]] QString iconThemeDark() const;
    void setIconThemeDark(const QString& iconTheme) const;

    [[nodiscard]] QString postHook() const;
    void setPostHook(const QString& hook) const;

private:
    explicit ConfigTheme(Config* config);

    Config* const m_config;

    friend Config;
};

} // namespace caelestia::config
