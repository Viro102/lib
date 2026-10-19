#include "caelestia/config/theme.hpp"

#include "caelestia/config/config.hpp"

namespace caelestia::config {

using Qt::StringLiterals::operator""_s;

ConfigTheme::ConfigTheme(Config* config)
    : m_config(config) {}

bool ConfigTheme::targetEnabled(QStringView target) const {
    return m_config->value(u"theme.targets."_s + target, true).toBool();
}

void ConfigTheme::setTargetEnabled(QStringView target, bool enabled) const {
    m_config->setValue(u"theme.targets."_s + target, enabled);
}

QString ConfigTheme::iconTheme() const {
    return m_config->value(u"theme.iconTheme"_s).toString();
}

void ConfigTheme::setIconTheme(const QString& iconTheme) const {
    m_config->setValue(u"theme.iconTheme"_s, iconTheme);
}

QString ConfigTheme::iconThemeLight() const {
    auto val = m_config->value(u"theme.iconThemeLight"_s).toString();
    if (!val.isEmpty())
        return val;
    if (const auto theme = iconTheme(); !theme.isEmpty())
        return theme;
    return u"Papirus-Light"_s;
}

void ConfigTheme::setIconThemeLight(const QString& iconTheme) const {
    m_config->setValue(u"theme.iconThemeLight"_s, iconTheme);
}

QString ConfigTheme::iconThemeDark() const {
    auto val = m_config->value(u"theme.iconThemeDark"_s).toString();
    if (!val.isEmpty())
        return val;
    if (const auto theme = iconTheme(); !theme.isEmpty())
        return theme;
    return u"Papirus-Dark"_s;
}

void ConfigTheme::setIconThemeDark(const QString& iconTheme) const {
    m_config->setValue(u"theme.iconThemeDark"_s, iconTheme);
}

QString ConfigTheme::postHook() const {
    return m_config->value(u"theme.postHook"_s).toString();
}

void ConfigTheme::setPostHook(const QString& hook) const {
    m_config->setValue(u"theme.postHook"_s, hook);
}

} // namespace caelestia::config
