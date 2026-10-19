#include <qjsonobject.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include "caelestia/config/config.hpp"
#include "common.hpp"

using namespace caelestia::config;
using Qt::StringLiterals::operator""_s;

class TestConfigTheme : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;

    static QJsonObject writtenTheme() { return readJson(Config::configPath())[u"theme"_s].toObject(); }

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CONFIG_HOME", m_root.path().toUtf8());
    }

    void defaultsWhenUnset() {
        const auto& theme = Config::instance().theme();
        QVERIFY(Config::instance().root().isEmpty());
        QVERIFY(theme.targetEnabled(u"gtk"));
        QVERIFY(theme.targetEnabled(u"anything"));
        QVERIFY(theme.iconTheme().isEmpty());
        QCOMPARE(theme.iconThemeLight(), u"Papirus-Light"_s);
        QCOMPARE(theme.iconThemeDark(), u"Papirus-Dark"_s);
        QVERIFY(theme.postHook().isEmpty());
    }

    void targetsReadFromConfig() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"targets":{"gtk":false,"hypr":true,"qt":"yes","btop":null}}})");
        config.reload();

        const auto& theme = config.theme();
        QVERIFY(!theme.targetEnabled(u"gtk"));
        QVERIFY(theme.targetEnabled(u"hypr"));
        QVERIFY(!theme.targetEnabled(u"qt"));
        QVERIFY(!theme.targetEnabled(u"btop"));
        QVERIFY(theme.targetEnabled(u"missing"));
    }

    void targetsDefaultWhenNotObject() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"targets":"all"}})");
        config.reload();
        QVERIFY(config.theme().targetEnabled(u"gtk"));
    }

    void setTargetEnabledPersists() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"targets":{"hypr":true}}})");
        config.reload();
        const auto& theme = config.theme();

        theme.setTargetEnabled(u"gtk", false);
        QVERIFY(!theme.targetEnabled(u"gtk"));
        QVERIFY(theme.targetEnabled(u"hypr"));

        auto targets = writtenTheme()[u"targets"_s].toObject();
        QCOMPARE(targets[u"gtk"_s].toBool(), false);
        QCOMPARE(targets[u"hypr"_s].toBool(), true);

        theme.setTargetEnabled(u"gtk", true);
        QVERIFY(theme.targetEnabled(u"gtk"));
        targets = writtenTheme()[u"targets"_s].toObject();
        QCOMPARE(targets[u"gtk"_s].toBool(), true);
    }

    void iconThemeLightDarkFallBackToIconTheme() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"iconTheme":"Tela"}})");
        config.reload();

        const auto& theme = config.theme();
        QCOMPARE(theme.iconTheme(), u"Tela"_s);
        QCOMPARE(theme.iconThemeLight(), u"Tela"_s);
        QCOMPARE(theme.iconThemeDark(), u"Tela"_s);
    }

    void iconThemeVariantsOverrideIconTheme() {
        auto& config = Config::instance();
        writeFile(Config::configPath(),
            R"({"theme":{"iconTheme":"Tela","iconThemeLight":"Tela-light","iconThemeDark":"Tela-dark"}})");
        config.reload();

        const auto& theme = config.theme();
        QCOMPARE(theme.iconThemeLight(), u"Tela-light"_s);
        QCOMPARE(theme.iconThemeDark(), u"Tela-dark"_s);
    }

    void iconThemeVariantsIndependent() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"iconThemeLight":"Tela-light"}})");
        config.reload();

        const auto& theme = config.theme();
        QCOMPARE(theme.iconThemeLight(), u"Tela-light"_s);
        QCOMPARE(theme.iconThemeDark(), u"Papirus-Dark"_s);
    }

    void emptyIconThemeUsesBuiltinDefaults() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"iconTheme":"","iconThemeLight":"","iconThemeDark":7}})");
        config.reload();

        const auto& theme = config.theme();
        QCOMPARE(theme.iconThemeLight(), u"Papirus-Light"_s);
        QCOMPARE(theme.iconThemeDark(), u"Papirus-Dark"_s);
    }

    void iconThemeSettersPersist() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), "{}");
        config.reload();
        const auto& theme = config.theme();

        theme.setIconTheme(u"A"_s);
        theme.setIconThemeLight(u"B"_s);
        theme.setIconThemeDark(u"C"_s);

        QCOMPARE(theme.iconTheme(), u"A"_s);
        QCOMPARE(theme.iconThemeLight(), u"B"_s);
        QCOMPARE(theme.iconThemeDark(), u"C"_s);

        const auto written = writtenTheme();
        QCOMPARE(written[u"iconTheme"_s].toString(), u"A"_s);
        QCOMPARE(written[u"iconThemeLight"_s].toString(), u"B"_s);
        QCOMPARE(written[u"iconThemeDark"_s].toString(), u"C"_s);
    }

    void postHookReadsAndPersists() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"postHook":"notify-send done"}})");
        config.reload();
        const auto& theme = config.theme();
        QCOMPARE(theme.postHook(), u"notify-send done"_s);

        theme.setPostHook(u"true"_s);
        QCOMPARE(theme.postHook(), u"true"_s);
        QCOMPARE(writtenTheme()[u"postHook"_s].toString(), u"true"_s);
    }

    void postHookNonStringIsEmpty() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"theme":{"postHook":["a","b"]}})");
        config.reload();
        QVERIFY(config.theme().postHook().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestConfigTheme)

#include "theme_test.moc"
