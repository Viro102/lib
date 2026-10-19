#include <qfile.h>
#include <qprocess.h>
#include <qtemporarydir.h>

#include <array>

#include "caelestia/theme/theme.hpp"
#include "util.hpp"

namespace caelestia::theme {

using Qt::StringLiterals::operator""_s;

namespace {

constexpr std::array k_clients = { u"Equicord", u"Vencord", u"BetterDiscord", u"equibop", u"vesktop", u"legcord" };

QString genScss(const scheme::SchemeColours& colours) {
    QString out;
    for (const auto& [name, colour] : scheme::namedColours(colours))
        out += u"$%1: #%2;\n"_s.arg(name, hex(colour));
    return out;
}

} // namespace

void Theme::applyDiscord() const {
    const QTemporaryDir dir;
    if (!dir.isValid() || !writeFile(dir.filePath(u"_colours.scss"_s), genScss(m_scheme.colours)) ||
        !QFile::copy(u":/templates/discord.scss"_s, dir.filePath(u"discord.scss"_s)))
        return;

    QProcess sass;
    sass.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    sass.start(u"sass"_s, { u"-I"_s, dir.path(), dir.filePath(u"discord.scss"_s) });
    if (!sass.waitForFinished(-1) || sass.exitStatus() != QProcess::NormalExit || sass.exitCode() != 0) {
        qCWarning(lcTheme) << "Failed to compile discord theme:" << sass.errorString();
        return;
    }

    const auto css = QString::fromUtf8(sass.readAllStandardOutput());
    for (const auto* const client : k_clients)
        writeFile(u"%1/%2/themes/caelestia.theme.css"_s.arg(configDir(), client), css);
}

} // namespace caelestia::theme
