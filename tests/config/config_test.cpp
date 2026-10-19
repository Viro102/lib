#include <qdir.h>
#include <qfile.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include "caelestia/config/config.hpp"
#include "helpers.hpp"

using namespace caelestia::config;
using Qt::StringLiterals::operator""_s;

class TestConfig : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CONFIG_HOME", m_root.path().toUtf8());
        QCOMPARE(Config::configDir(), m_root.path() + u"/caelestia"_s);
        QCOMPARE(Config::configPath(), m_root.path() + u"/caelestia/config.json"_s);
        writeFile(Config::configPath(), R"({"bar":{"height":32,"shown":true},"name":"caelestia"})");
    }

    void loadsOnCreation() {
        const auto& config = Config::instance();
        QCOMPARE(config.value(u"name").toString(), u"caelestia"_s);
        QCOMPARE(config.value(u"bar.height").toInt(), 32);
        QCOMPARE(config.value(u"bar.shown").toBool(), true);
        QCOMPARE(config.root().keys(), (QStringList{ u"bar"_s, u"name"_s }));
    }

    void instanceIsSingleton() { QCOMPARE(&Config::instance(), &Config::instance()); }

    void valueReturnsObjectsAndArrays() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"obj":{"a":1},"arr":[1,2,3],"nil":null})");
        config.reload();

        QVERIFY(config.value(u"obj").isObject());
        QCOMPARE(config.value(u"obj").toObject().value(u"a"_s).toInt(), 1);
        QVERIFY(config.value(u"arr").isArray());
        QCOMPARE(config.value(u"arr").toArray().size(), 3);
        QVERIFY(config.value(u"arr.0").isUndefined());
    }

    void nullIsNotFallback() {
        const auto& config = Config::instance();
        QVERIFY(config.value(u"nil").isNull());
        QVERIFY(config.value(u"nil", 7).isNull());
    }

    void fallbackForMissing() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"bar":{"height":32,"shown":true},"name":"caelestia"})");
        config.reload();

        QVERIFY(config.value(u"missing").isUndefined());
        QCOMPARE(config.value(u"missing", 7).toInt(), 7);
        QCOMPARE(config.value(u"name.nested", 7).toInt(), 7);
        QCOMPARE(config.value(u"bar.missing.deep", 7).toInt(), 7);
        QCOMPARE(config.value(u"", 7).toInt(), 7);
        QCOMPARE(config.value(u"bar.", 7).toInt(), 7);
    }

    void doesNotWatchFile() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"name":"changed"})");
        QCOMPARE(config.value(u"name").toString(), u"caelestia"_s);
        config.reload();
        QCOMPARE(config.value(u"name").toString(), u"changed"_s);
        QVERIFY(config.value(u"bar").isUndefined());
    }

    void setValueUpdatesInPlaceAndWrites() {
        auto& config = Config::instance();
        QVERIFY(config.setValue(u"bar.height", 48));
        QVERIFY(config.setValue(u"new.deep.key", u"x"_s));
        QCOMPARE(config.value(u"bar.height").toInt(), 48);
        QCOMPARE(config.value(u"new.deep.key").toString(), u"x"_s);
        QCOMPARE(config.value(u"name").toString(), u"changed"_s);

        const auto written = readFile(Config::configPath());
        QCOMPARE(written, config.root());
        QCOMPARE(written[u"bar"_s].toObject()[u"height"_s].toInt(), 48);
    }

    void setValuePreservesSiblings() {
        auto& config = Config::instance();
        QVERIFY(config.setValue(u"bar.width", 100));
        QCOMPARE(config.value(u"bar.height").toInt(), 48);
        QCOMPARE(config.value(u"bar.width").toInt(), 100);
        QCOMPARE(config.value(u"new.deep.key").toString(), u"x"_s);
    }

    void setValueReplacesScalarWithObject() {
        auto& config = Config::instance();
        QVERIFY(config.setValue(u"name.nested", 1));
        QVERIFY(config.value(u"name").isObject());
        QCOMPARE(config.value(u"name.nested").toInt(), 1);
    }

    void setValueReplacesObjectWithScalar() {
        auto& config = Config::instance();
        QVERIFY(config.setValue(u"bar", u"flat"_s));
        QCOMPARE(config.value(u"bar").toString(), u"flat"_s);
        QVERIFY(config.value(u"bar.height").isUndefined());
    }

    void setValueAcceptsCompositeValues() {
        auto& config = Config::instance();
        QVERIFY(config.setValue(u"list", QJsonArray{ 1, 2 }));
        QVERIFY(config.setValue(u"obj", QJsonObject{ { u"k"_s, true } }));
        QVERIFY(config.setValue(u"nil", QJsonValue::Null));
        QCOMPARE(config.value(u"list").toArray(), (QJsonArray{ 1, 2 }));
        QCOMPARE(config.value(u"obj.k").toBool(), true);
        QVERIFY(config.value(u"nil").isNull());
        QCOMPARE(readFile(Config::configPath()), config.root());
    }

    void setValueRejectsEmptyPath() {
        auto& config = Config::instance();
        const auto before = config.root();
        QVERIFY(!config.setValue(u"", 1));
        QVERIFY(!config.setValue(u".leading", 1));
        QCOMPARE(config.root(), before);
    }

    void setValueCreatesMissingDirectory() {
        auto& config = Config::instance();
        QVERIFY(QDir(Config::configDir()).removeRecursively());
        QVERIFY(!QFile::exists(Config::configPath()));
        QVERIFY(config.setValue(u"name", u"recreated"_s));
        QVERIFY(QFile::exists(Config::configPath()));
        QCOMPARE(readFile(Config::configPath())[u"name"_s].toString(), u"recreated"_s);
    }

    void setValueWritesIndentedJson() {
        QFile file(Config::configPath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto text = file.readAll();
        QVERIFY(text.contains('\n'));
        QCOMPARE(text, QJsonDocument(Config::instance().root()).toJson(QJsonDocument::Indented));
    }

    void reloadDropsMissingFile() {
        auto& config = Config::instance();
        QVERIFY(QFile::remove(Config::configPath()));
        config.reload();
        QVERIFY(config.root().isEmpty());
    }

    void reloadDropsInvalidFile() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"({"name":"valid"})");
        config.reload();
        QCOMPARE(config.value(u"name").toString(), u"valid"_s);

        writeFile(Config::configPath(), "not json");
        config.reload();
        QVERIFY(config.root().isEmpty());
    }

    void reloadDropsNonObjectRoot() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), R"([1,2,3])");
        config.reload();
        QVERIFY(config.root().isEmpty());

        writeFile(Config::configPath(), R"("string")");
        config.reload();
        QVERIFY(config.root().isEmpty());
    }

    void reloadAcceptsEmptyObject() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), "{}");
        config.reload();
        QVERIFY(config.root().isEmpty());
        QVERIFY(config.setValue(u"a", 1));
        QCOMPARE(config.root().size(), 1);
    }

    void optionAccessorsReadFromConfig() {
        auto& config = Config::instance();
        writeFile(Config::configPath(),
            R"({"apps":{"terminal":"kitty"},"paths":{"wallpapersDir":"/w"},"theme":{"postHook":"h"}})");
        config.reload();
        QCOMPARE(config.apps().terminal(), u"kitty"_s);
        QCOMPARE(config.paths().wallpapersDir(), u"/w"_s);
        QCOMPARE(config.theme().postHook(), u"h"_s);
    }
};

QTEST_GUILESS_MAIN(TestConfig)

#include "config_test.moc"
