#include <qjsonobject.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include "caelestia/config/config.hpp"
#include "common.hpp"

using namespace caelestia::config;
using Qt::StringLiterals::operator""_s;

class TestConfigApps : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;

    static QJsonObject writtenApps() { return readJson(Config::configPath()).value(u"apps"_s).toObject(); }

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CONFIG_HOME", m_root.path().toUtf8());
    }

    void defaultsWhenUnset() {
        const auto& apps = Config::instance().apps();
        QVERIFY(Config::instance().root().isEmpty());
        QCOMPARE(apps.terminal(), ConfigApps::defaultTerminal());
        QCOMPARE(apps.browser(), ConfigApps::defaultBrowser());
        QCOMPARE(apps.editor(), ConfigApps::defaultEditor());
        QCOMPARE(apps.fileExplorer(), ConfigApps::defaultFileExplorer());
        QCOMPARE(apps.audioSettings(), ConfigApps::defaultAudioSettings());
    }

    void defaultValues() {
        QCOMPARE(ConfigApps::defaultTerminal(), u"foot"_s);
        QCOMPARE(ConfigApps::defaultBrowser(), u"firefox"_s);
        QCOMPARE(ConfigApps::defaultEditor(), u"codium"_s);
        QCOMPARE(ConfigApps::defaultFileExplorer(), u"thunar"_s);
        QCOMPARE(ConfigApps::defaultAudioSettings(), u"pwvucontrol"_s);
    }

    void readsConfiguredValues() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"apps":{
            "terminal":"kitty","browser":"zen","editor":"nvim",
            "fileExplorer":"nautilus","audioSettings":"pavucontrol"}})");
        config.reload();

        const auto& apps = config.apps();
        QCOMPARE(apps.terminal(), u"kitty"_s);
        QCOMPARE(apps.browser(), u"zen"_s);
        QCOMPARE(apps.editor(), u"nvim"_s);
        QCOMPARE(apps.fileExplorer(), u"nautilus"_s);
        QCOMPARE(apps.audioSettings(), u"pavucontrol"_s);
    }

    void emptyOrNonStringFallsBackToDefault() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"apps":{"terminal":"","browser":42,"editor":null,"fileExplorer":{}}})");
        config.reload();

        const auto& apps = config.apps();
        QCOMPARE(apps.terminal(), ConfigApps::defaultTerminal());
        QCOMPARE(apps.browser(), ConfigApps::defaultBrowser());
        QCOMPARE(apps.editor(), ConfigApps::defaultEditor());
        QCOMPARE(apps.fileExplorer(), ConfigApps::defaultFileExplorer());
    }

    void settersUpdateAndPersist() {
        auto& config = Config::instance();
        config.reload();
        const auto& apps = config.apps();

        apps.setTerminal(u"alacritty"_s);
        apps.setBrowser(u"chromium"_s);
        apps.setEditor(u"helix"_s);
        apps.setFileExplorer(u"dolphin"_s);
        apps.setAudioSettings(u"qpwgraph"_s);

        QCOMPARE(apps.terminal(), u"alacritty"_s);
        QCOMPARE(apps.browser(), u"chromium"_s);
        QCOMPARE(apps.editor(), u"helix"_s);
        QCOMPARE(apps.fileExplorer(), u"dolphin"_s);
        QCOMPARE(apps.audioSettings(), u"qpwgraph"_s);

        const auto written = writtenApps();
        QCOMPARE(written[u"terminal"_s].toString(), u"alacritty"_s);
        QCOMPARE(written[u"browser"_s].toString(), u"chromium"_s);
        QCOMPARE(written[u"editor"_s].toString(), u"helix"_s);
        QCOMPARE(written[u"fileExplorer"_s].toString(), u"dolphin"_s);
        QCOMPARE(written[u"audioSettings"_s].toString(), u"qpwgraph"_s);
    }

    void settersPreserveOtherKeys() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"apps":{"browser":"zen"},"other":1})");
        config.reload();

        config.apps().setTerminal(u"kitty"_s);
        QCOMPARE(config.apps().browser(), u"zen"_s);
        QCOMPARE(config.value(u"other").toInt(), 1);
        QCOMPARE(writtenApps().value(u"browser"_s).toString(), u"zen"_s);
    }

    void settingEmptyRevertsToDefault() {
        const auto& apps = Config::instance().apps();
        apps.setTerminal(u""_s);
        QCOMPARE(apps.terminal(), ConfigApps::defaultTerminal());
        QCOMPARE(writtenApps().value(u"terminal"_s).toString(), u""_s);
    }
};

QTEST_GUILESS_MAIN(TestConfigApps)

#include "apps_test.moc"
