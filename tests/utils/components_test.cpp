#include <qtemporarydir.h>
#include <qtest.h>

#include "caelestia/utils/components.hpp"
#include "common.hpp"

using namespace caelestia::utils;
using Qt::StringLiterals::operator""_s;

namespace {

const auto k_componentsDir = u"/caelestia/components"_s;

} // namespace

class TestComponents : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    QString m_home;
    QString m_system;

    [[nodiscard]] static QString manifest(
        const QString& version, const QString& distributor, const QString& revision = {}) {
        return u"[Component]\nVersion=%1\nDistributor=%2\nGitRevision=%3\n"_s.arg(version, distributor, revision);
    }

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        m_home = m_root.path() + u"/home"_s;
        m_system = m_root.path() + u"/system"_s;
        qputenv("XDG_DATA_HOME", m_home.toUtf8());
        qputenv("XDG_DATA_DIRS", m_system.toUtf8());
    }

    void emptyWhenNoManifests() { QVERIFY(installedComponents().isEmpty()); }

    void readsManifestsKeyedByFileName() {
        const auto dir = m_system + k_componentsDir;
        writeFile(dir + u"/shell.ini"_s, manifest(u"1.2.3"_s, u"Arch Linux"_s, u"abc123"_s).toUtf8());
        writeFile(dir + u"/cli.ini"_s, manifest(u"0.1"_s, u"Nix"_s).toUtf8());
        writeFile(dir + u"/ignored.txt"_s, "nope");

        const auto components = installedComponents();
        QCOMPARE(components.size(), 2);
        QCOMPARE(components[u"shell"_s], (ComponentInfo{ u"1.2.3"_s, u"Arch Linux"_s, u"abc123"_s }));
        QCOMPARE(components[u"cli"_s], (ComponentInfo{ u"0.1"_s, u"Nix"_s, {} }));
    }

    void userDataOverridesSystem() {
        writeFile(m_home + k_componentsDir + u"/shell.ini"_s, manifest(u"2.0"_s, u"Source"_s).toUtf8());

        const auto components = installedComponents();
        QCOMPARE(components.size(), 2);
        QCOMPARE(components[u"shell"_s].version, u"2.0"_s);
        QCOMPARE(components[u"shell"_s].distributor, u"Source"_s);
    }
};

QTEST_GUILESS_MAIN(TestComponents)

#include "components_test.moc"
