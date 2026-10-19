#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstandardpaths.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include <array>
#include <bit>

#include "caelestia/config/config.hpp"
#include "caelestia/scheme/scheme.hpp"
#include "caelestia/theme/theme.hpp"

using namespace caelestia;
using Qt::StringLiterals::operator""_s;

namespace {

void writeFile(const QString& path, const QByteArray& data) {
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), data.size());
}

QString readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

} // namespace

class TestTheme : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    scheme::SchemeColours m_colours =
        std::bit_cast<scheme::SchemeColours>(std::array<quint32, sizeof(scheme::SchemeColours) / sizeof(quint32)>{});

    [[nodiscard]] scheme::Scheme scheme(scheme::Scheme::Mode mode = scheme::Scheme::Mode::Dark) const {
        return { .name = u"test"_s, .flavour = u"default"_s, .mode = mode, .colours = m_colours };
    }

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CONFIG_HOME", m_root.filePath(u"config"_s).toUtf8());
        qputenv("XDG_DATA_HOME", m_root.filePath(u"data"_s).toUtf8());
        qputenv("XDG_STATE_HOME", m_root.filePath(u"state"_s).toUtf8());
        qunsetenv("HYPRLAND_INSTANCE_SIGNATURE");

        writeFile(config::Config::configPath(), R"({"theme":{"targets":{
            "terminal":false,"discord":false,"btop":false,"htop":false,"gtk":false,"qt":false,
            "chromium":false,"cava":false
        }}})");

        m_colours.primary = m3colours::Argb{ 0xFF2962FF };
        m_colours.surface = m3colours::Argb{ 0xFF101418 };
        m_colours.onSurface = m3colours::Argb{ 0xFFE0E3E8 };
        m_colours.term1 = m3colours::Argb{ 0x80FF0000 };
    }

    void rendersColourForms() {
        const auto path = m_root.filePath(u"tpl.txt"_s);
        writeFile(path, "{{ primary.hex }} {{ term1.hexalpha }} {{ primary.rgb }} {{ term1.rgbalpha }} "
                        "{{ primary.red }} {{ primary.hsl }} {{ primary.hue }} {{ mode }} {{ iconTheme }} "
                        "{{ missing.hex }} {{ primary.bogus }} {{ nested }}");
        QCOMPARE(theme::Theme(scheme()).render(path),
            u"2962ff ff000080 rgb(41,98,255) rgba(255,0,0,128) 41 hsl(224,100%,58%) 224 dark Papirus-Dark "
            u"{{ missing.hex }} {{ primary.bogus }} {{ nested }}"_s);
    }

    void rendersSwitches() {
        const auto path = m_root.filePath(u"tpl.txt"_s);
        writeFile(path, "{{ mode ? dark=D | light=L }}"
                        "{{ mode ? light=no }}"
                        "{{ mode ? dark=primary.hex | 000000 }}"
                        "{{ iconTheme ? Papirus-Dark=I | x }}"
                        "{{ mode ? a|b=no | 1 | 2 }}"
                        "{{ missing ? dark=x | y }}");
        QCOMPARE(theme::Theme(scheme()).render(path), u"D2962ffI1{{ missing ? dark=x | y }}"_s);
        QCOMPARE(
            theme::Theme(scheme(scheme::Scheme::Mode::Light)).render(path), u"Lno000000x1{{ missing ? dark=x | y }}"_s);
    }

    void rendersShippedTemplate() {
        const auto out = theme::Theme(scheme()).render(u":/templates/fuzzel.ini"_s);
        QVERIFY(!out.isEmpty());
        QVERIFY(out.contains(u"2962ff"));
        QVERIFY(!out.contains(u"{{ primary"));
    }

    void generatesSequences() {
        const auto seqs = theme::Theme(scheme()).sequences();
        QVERIFY(seqs.startsWith(u"\x1b]10;rgb:e0/e3/e8\x1b\\\x1b]11;rgb:10/14/18\x1b\\"_s));
        QVERIFY(seqs.contains(u"\x1b]4;1;rgb:ff/00/00\x1b\\"_s));
        QVERIFY(seqs.endsWith(u"\x1b]4;18;rgb:00/00/00\x1b\\"_s));
    }

    void appliesEnabledTargets() {
        const auto configDir = m_root.filePath(u"config"_s);
        const auto dataDir = m_root.filePath(u"data"_s);
        const auto themesDir = config::ConfigPaths::customThemesDir();
        writeFile(themesDir + u"/custom.conf"_s, "bg=#{{ surface.hex }} {{ mode }}");
        writeFile(themesDir + u"/templates.json"_s,
            u"[{\"from\":\"custom.conf\",\"to\":\"%1/custom/out.conf\"},{\"from\":\"custom.conf\"}]"_s.arg(configDir)
                .toUtf8());

        theme::Theme(scheme()).apply().waitForFinished();

        QVERIFY(readFile(configDir + u"/hypr/scheme/current.conf"_s).contains(u"$primary = 2962ff\n"));
        QVERIFY(readFile(configDir + u"/fuzzel/fuzzel.ini"_s).contains(u"2962ff"));
        QVERIFY(readFile(configDir + u"/spicetify/Themes/caelestia/color.ini"_s).contains(u"2962ff"));
        QVERIFY(readFile(configDir + u"/nvtop/nvtop.colors"_s).contains(u"#2962ff"));
        QVERIFY(readFile(configDir + u"/zed/themes/caelestia.json"_s).contains(u"\"appearance\": \"dark\""));
        QVERIFY(readFile(dataDir + u"/warp-terminal/themes/caelestia.yaml"_s).contains(u"details: darker"));
        QVERIFY(readFile(dataDir + u"/PandoraLauncher/themes/caelestia.json"_s).contains(u"\"mode\": \"dark\""));
        QCOMPARE(readFile(configDir + u"/custom/out.conf"_s), u"bg=#101418 dark"_s);

        QVERIFY(!QFileInfo::exists(configDir + u"/btop/themes/caelestia.theme"_s));
        QVERIFY(!QFileInfo::exists(configDir + u"/gtk-3.0/gtk.css"_s));
    }

    void compilesDiscordTheme() {
        if (QStandardPaths::findExecutable(u"sass"_s).isEmpty())
            QSKIP("sass not installed");
        auto& config = config::Config::instance();
        QVERIFY(config.setValue(u"theme.targets.discord", true));

        theme::Theme(scheme()).apply().waitForFinished();

        const auto css = readFile(m_root.filePath(u"config/Vencord/themes/caelestia.theme.css"_s));
        QVERIFY(css.contains(u"--bg-3: #101418;"));
        QVERIFY(css.contains(u"--blue-1: #2962ff;"));
    }

    void runsPostHook() {
        auto& config = config::Config::instance();
        const auto out = m_root.filePath(u"hook.txt"_s);
        QVERIFY(config.setValue(u"theme.postHook", u"echo \"$SCHEME_NAME $SCHEME_MODE $SCHEME_COLOURS\" > "_s + out));

        theme::Theme(scheme(scheme::Scheme::Mode::Light)).apply().waitForFinished();

        const auto result = readFile(out);
        QVERIFY(result.startsWith(u"test light {"));
        QVERIFY(result.contains(u"\"primary\":\"2962ff\""));
    }
};

QTEST_GUILESS_MAIN(TestTheme)

#include "theme_test.moc"
