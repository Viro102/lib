#include <qfile.h>
#include <qfileinfo.h>

#include <csignal>

#include "caelestia/theme/theme.hpp"
#include "util.hpp"

namespace caelestia::theme {

using Qt::StringLiterals::operator""_s;

void Theme::applySpicetify() const {
    writeTemplate(u"spicetify.ini"_s, configDir() + u"/spicetify/Themes/caelestia/color.ini"_s);
}

void Theme::applyPandora() const {
    writeTemplate(u"pandora.json"_s, dataDir() + u"/PandoraLauncher/themes/caelestia.json"_s);
}

void Theme::applyFuzzel() const {
    writeTemplate(u"fuzzel.ini"_s, configDir() + u"/fuzzel/fuzzel.ini"_s);
}

void Theme::applyBtop() const {
    writeTemplate(u"btop.theme"_s, configDir() + u"/btop/themes/caelestia.theme"_s);
    signalProcess(u"btop"_s, SIGUSR2);
}

void Theme::applyNvtop() const {
    writeTemplate(u"nvtop.colors"_s, configDir() + u"/nvtop/nvtop.colors"_s);
}

void Theme::applyHtop() const {
    writeTemplate(u"htop.theme"_s, configDir() + u"/htop/htoprc"_s);
    signalProcess(u"htop"_s, SIGUSR2);
}

void Theme::applyQt() const {
    writeTemplate(u"qt.colors"_s, configDir() + u"/qtengine/caelestia.colors"_s);
    writeTemplate(u"qtengine.json"_s, configDir() + u"/qtengine/config.json"_s);
}

void Theme::applyWarp() const {
    writeTemplate(u"warp.yaml"_s, dataDir() + u"/warp-terminal/themes/caelestia.yaml"_s);
}

void Theme::applyZed() const {
    const auto path = configDir() + u"/zed/themes/caelestia.json"_s;
    // Zed's file watcher does not follow symlinks, so replace one with a regular file
    if (QFileInfo(path).isSymLink())
        QFile::remove(path);
    writeTemplate(u"zed.json"_s, path);
}

void Theme::applyCava() const {
    writeTemplate(u"cava.conf"_s, configDir() + u"/cava/config"_s);
    signalProcess(u"cava"_s, SIGUSR2);
}

} // namespace caelestia::theme
