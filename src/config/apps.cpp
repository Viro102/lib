#include "caelestia/config/apps.hpp"

#include "caelestia/config/config.hpp"

namespace caelestia::config {

using Qt::StringLiterals::operator""_s;

ConfigApps::ConfigApps(Config* config)
    : m_config(config) {}

// TODO: impls for the methods in apps.hpp
QString ConfigApps::defaultTerminal() {
    return u"foot"_s;
}

QString ConfigApps::terminal() const {
    const auto val = m_config->value(u"apps.terminal"_s).toString();
    return val.isEmpty() ? defaultTerminal() : val;
}

void ConfigApps::setTerminal(const QString& dir) const {
    m_config->setValue(u"apps.terminal"_s, dir);
}

QString ConfigApps::defaultBrowser() {
    return u"firefox"_s;
}

QString ConfigApps::browser() const {
    const auto val = m_config->value(u"apps.browser"_s).toString();
    return val.isEmpty() ? defaultBrowser() : val;
}

void ConfigApps::setBrowser(const QString& browser) const {
    m_config->setValue(u"apps.browser"_s, browser);
}

QString ConfigApps::defaultEditor() {
    return u"codium"_s;
}

QString ConfigApps::editor() const {
    const auto val = m_config->value(u"apps.editor"_s).toString();
    return val.isEmpty() ? defaultEditor() : val;
}

void ConfigApps::setEditor(const QString& editor) const {
    m_config->setValue(u"apps.editor"_s, editor);
}

QString ConfigApps::defaultFileExplorer() {
    return u"thunar"_s;
}

QString ConfigApps::fileExplorer() const {
    const auto val = m_config->value(u"apps.fileExplorer"_s).toString();
    return val.isEmpty() ? defaultFileExplorer() : val;
}

void ConfigApps::setFileExplorer(const QString& fileExplorer) const {
    m_config->setValue(u"apps.fileExplorer"_s, fileExplorer);
}

QString ConfigApps::defaultAudioSettings() {
    return u"pwvucontrol"_s;
}

QString ConfigApps::audioSettings() const {
    const auto val = m_config->value(u"apps.audioSettings"_s).toString();
    return val.isEmpty() ? defaultAudioSettings() : val;
}

void ConfigApps::setAudioSettings(const QString& audioSettings) const {
    m_config->setValue(u"apps.audioSettings"_s, audioSettings);
}

} // namespace caelestia::config
