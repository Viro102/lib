#pragma once

#include <qstring.h>
#include <qtclasshelpermacros.h>

namespace caelestia::config {

class Config;

class ConfigApps {
public:
    Q_DISABLE_COPY_MOVE(ConfigApps)

    [[nodiscard]] static QString defaultTerminal();
    [[nodiscard]] QString terminal() const;
    void setTerminal(const QString& dir) const;

    [[nodiscard]] static QString defaultBrowser();
    [[nodiscard]] QString browser() const;
    void setBrowser(const QString& browser) const;

    [[nodiscard]] static QString defaultEditor();
    [[nodiscard]] QString editor() const;
    void setEditor(const QString& editor) const;

    [[nodiscard]] static QString defaultFileExplorer();
    [[nodiscard]] QString fileExplorer() const;
    void setFileExplorer(const QString& fileExplorer) const;

    [[nodiscard]] static QString defaultAudioSettings();
    [[nodiscard]] QString audioSettings() const;
    void setAudioSettings(const QString& audioSettings) const;

private:
    explicit ConfigApps(Config* config);

    Config* const m_config;

    friend Config;
};

} // namespace caelestia::config
