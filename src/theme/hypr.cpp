#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>

#include "caelestia/theme/theme.hpp"
#include "util.hpp"

namespace caelestia::theme {

using Qt::StringLiterals::operator""_s;

namespace {

bool queryLuaConfig() {
    const auto runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    const auto signature = qEnvironmentVariable("HYPRLAND_INSTANCE_SIGNATURE");
    if (runtimeDir.isEmpty() || signature.isEmpty())
        return false;

    QLocalSocket socket;
    socket.connectToServer(u"%1/hypr/%2/.socket.sock"_s.arg(runtimeDir, signature));
    if (!socket.waitForConnected(1000))
        return false;

    socket.write("j/status");
    QByteArray response;
    while (socket.waitForReadyRead(1000))
        response += socket.readAll();
    response += socket.readAll();

    return QJsonDocument::fromJson(response).object()[u"configProvider"_s].toString() == u"lua";
}

bool isLuaConfig() {
    static const bool k_lua = queryLuaConfig();
    return k_lua;
}

QString genConf(const scheme::SchemeColours& colours) {
    QString out;
    for (const auto& [name, colour] : scheme::namedColours(colours))
        out += u"$%1 = %2\n"_s.arg(name, hex(colour));
    return out;
}

QString genLua(const scheme::SchemeColours& colours) {
    QString out = u"return {\n"_s;
    for (const auto& [name, colour] : scheme::namedColours(colours))
        out += u"  %1 = \"%2\",\n"_s.arg(name, hex(colour));
    return out + u"}"_s;
}

} // namespace

void Theme::applyHypr() const {
    const auto& colours = m_scheme.colours;
    if (isLuaConfig())
        writeFile(configDir() + u"/hypr/scheme/current.lua"_s, genLua(colours));
    else
        writeFile(configDir() + u"/hypr/scheme/current.conf"_s, genConf(colours));
}

} // namespace caelestia::theme
