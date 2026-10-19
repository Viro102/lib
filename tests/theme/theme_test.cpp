#include <qcolor.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlockfile.h>
#include <qstandardpaths.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include <array>
#include <bit>

#include "caelestia/config/config.hpp"
#include "caelestia/scheme/scheme.hpp"
#include "caelestia/theme/theme.hpp"
#include "common.hpp"

using namespace caelestia;
using Qt::StringLiterals::operator""_s;

namespace {

qsizetype countFiles(const QString& root) {
    qsizetype count = 0;
    for (QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories); it.hasNext();
        it.next())
        ++count;
    return count;
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

    void rendersRemainingColourForms() {
        const auto path = m_root.filePath(u"tpl.txt"_s);
        writeFile(path, "{{ primary.green }} {{ primary.blue }} {{ primary.saturation }} {{ primary.lightness }} "
                        "{{primary.hex}} {{   mode   }}");
        const auto qc = QColor::fromRgb(0x29, 0x62, 0xFF);
        const auto expected =
            u"98 255 %1 %2 2962ff dark"_s.arg(qRound(qc.hslSaturationF() * 100)).arg(qRound(qc.lightnessF() * 100));
        QCOMPARE(theme::Theme(scheme()).render(path), expected);
        QCOMPARE(expected, u"98 255 100 58 2962ff dark"_s);
    }

    void renderOfMissingTemplateIsEmpty() { QVERIFY(theme::Theme(scheme()).render(u"/nonexistent/path"_s).isEmpty()); }

    void renderLeavesTextWithoutFieldsUntouched() {
        const auto path = m_root.filePath(u"plain.txt"_s);
        const QByteArray text = "plain {{ lone\nopen }} close }} and {{ {\n}\n";
        writeFile(path, text);
        QCOMPARE(theme::Theme(scheme()).render(path), QString::fromUtf8(text));
    }

    void zedReplacesSymlink() {
        const auto target = m_root.filePath(u"zed-target.json"_s);
        const auto link = m_root.filePath(u"config/zed/themes/caelestia.json"_s);
        writeFile(target, "original");
        QFile::remove(link);
        QVERIFY(QDir().mkpath(QFileInfo(link).absolutePath()));
        QVERIFY(QFile::link(target, link));
        QVERIFY(QFileInfo(link).isSymLink());

        theme::Theme(scheme()).applyZed();

        QVERIFY(!QFileInfo(link).isSymLink());
        QVERIFY(QFileInfo(link).isFile());
        QVERIFY(readFile(link).contains(u"\"appearance\": \"dark\""));
        QCOMPARE(readFile(target), u"original"_s);
    }

    void customTemplatesInvalidJsonIsIgnored() {
        const auto json = config::ConfigPaths::customThemesDir() + u"/templates.json"_s;

        writeFile(json, "{not json");
        const auto before = countFiles(m_root.path());
        theme::Theme(scheme()).applyCustom();
        QCOMPARE(countFiles(m_root.path()), before);

        writeFile(json, R"({"from":"a","to":"b"})");
        theme::Theme(scheme()).applyCustom();
        QCOMPARE(countFiles(m_root.path()), before);
    }

    void customTemplatesExpandTilde() {
        const auto oldHome = qgetenv("HOME");
        const auto home = m_root.filePath(u"home"_s);
        QVERIFY(QDir().mkpath(home));
        qputenv("HOME", home.toUtf8());

        const auto themesDir = config::ConfigPaths::customThemesDir();
        writeFile(themesDir + u"/tilde.tpl"_s, "{{ surface.hex }}");
        writeFile(themesDir + u"/templates.json"_s, R"([{"from":"tilde.tpl","to":"~/custom/tilde.conf"}])");

        theme::Theme(scheme()).applyCustom();

        qputenv("HOME", oldHome);
        QCOMPARE(readFile(home + u"/custom/tilde.conf"_s), u"101418"_s);
    }

    void customTemplateMissingSourceIsSkipped() {
        const auto out = m_root.filePath(u"missing-source/out.conf"_s);
        writeFile(config::ConfigPaths::customThemesDir() + u"/templates.json"_s,
            u"[{\"from\":\"does-not-exist.tpl\",\"to\":\"%1\"}]"_s.arg(out).toUtf8());

        theme::Theme(scheme()).applyCustom();

        QVERIFY(!QFileInfo::exists(out));
    }

    void postHookReceivesFlavour() {
        auto& config = config::Config::instance();
        const auto out = m_root.filePath(u"hook-flavour.txt"_s);
        QVERIFY(config.setValue(u"theme.targets.discord", false));
        QVERIFY(config.setValue(u"theme.postHook", u"echo \"$SCHEME_FLAVOUR\" > "_s + out));

        theme::Theme(scheme()).apply().waitForFinished();

        QVERIFY(config.setValue(u"theme.postHook", u""_s));
        QCOMPARE(readFile(out).trimmed(), u"default"_s);
    }

    void stateRoundTrips() {
        QVERIFY(theme::Theme::saveState(scheme(scheme::Scheme::Mode::Light)));

        const auto loaded = theme::Theme::loadState();
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->name, u"test"_s);
        QCOMPARE(loaded->flavour, u"default"_s);
        QCOMPARE(loaded->mode, scheme::Scheme::Mode::Light);
        QCOMPARE(scheme::coloursToBytes(loaded->colours), scheme::coloursToBytes(m_colours));
    }

    void invalidStateIsRejected() {
        const auto path = config::ConfigPaths::stateDir() + u"/scheme.bin"_s;

        QFile::remove(path);
        QVERIFY(!theme::Theme::loadState().has_value());

        writeFile(path, "garbage");
        QVERIFY(!theme::Theme::loadState().has_value());

        QByteArray truncated;
        QDataStream out(&truncated, QIODevice::WriteOnly);
        out << u"test"_s << u"default"_s << scheme::Scheme::Mode::Dark << QByteArray("abcd");
        writeFile(path, truncated);
        QVERIFY(!theme::Theme::loadState().has_value());
    }

    void appliesNewestStateOnceLockIsReleased() {
        const auto stateDir = config::ConfigPaths::stateDir();
        const auto statePath = stateDir + u"/scheme.bin"_s;
        QVERIFY(QDir().mkpath(stateDir));
        QFile::remove(statePath);

        QLockFile lock(stateDir + u"/theme.lock"_s);
        QVERIFY(lock.tryLock(0));

        auto pending = theme::Theme(scheme()).apply();
        QTRY_VERIFY(QFileInfo::exists(statePath));

        auto newer = m_colours;
        newer.primary = m3colours::Argb{ 0xFF00FF00 };
        QVERIFY(theme::Theme::saveState(
            { .name = u"newer"_s, .flavour = u"default"_s, .mode = scheme::Scheme::Mode::Dark, .colours = newer }));

        lock.unlock();
        pending.waitForFinished();

        const auto hypr = readFile(m_root.filePath(u"config/hypr/scheme/current.conf"_s));
        QVERIFY(hypr.contains(u"$primary = 00ff00\n"));
        QVERIFY(!hypr.contains(u"2962ff"));
    }
};

QTEST_GUILESS_MAIN(TestTheme)

#include "theme_test.moc"
