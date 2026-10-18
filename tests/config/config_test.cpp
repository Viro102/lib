#include <qdir.h>
#include <qfile.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include "caelestia/config/config.hpp"

using namespace caelestia::config;
using Qt::StringLiterals::operator""_s;

namespace {

void writeFile(const QString& path, const QByteArray& data) {
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), data.size());
}

QJsonObject readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

} // namespace

class TestConfig : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CONFIG_HOME", m_root.path().toUtf8());
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

    void fallbackForMissing() {
        const auto& config = Config::instance();
        QVERIFY(config.value(u"missing").isUndefined());
        QCOMPARE(config.value(u"missing", 7).toInt(), 7);
        QCOMPARE(config.value(u"name.nested", 7).toInt(), 7);
        QCOMPARE(config.value(u"bar.missing.deep", 7).toInt(), 7);
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

    void setValueRejectsEmptyPath() { QVERIFY(!Config::instance().setValue(u"", 1)); }

    void reloadDropsInvalidFile() {
        auto& config = Config::instance();
        writeFile(Config::configPath(), "not json");
        config.reload();
        QVERIFY(config.root().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestConfig)

#include "config_test.moc"
