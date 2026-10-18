#include <qdir.h>
#include <qfile.h>
#include <qstandardpaths.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include <atomic>

#include "caelestia/utils/cache.hpp"

using namespace caelestia::utils;
using Qt::StringLiterals::operator""_s;

namespace {

constexpr qint64 k_maxSize = 10;
constexpr auto k_suffix = ".bin";

QByteArray bytes(int n, char c) {
    return { n, c };
}

void writeFile(const QString& path, const QByteArray& data) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), data.size());
}

} // namespace

class TestFileCache : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    QString m_dir;

    [[nodiscard]] QString pathFor(const QString& key) const { return m_dir + u'/' + key + k_suffix; }

private slots:

    void init() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CACHE_HOME", m_root.path().toUtf8());
        m_dir = FileCache::cacheDir() + u"/test"_s;
        QVERIFY(QDir(m_dir).removeRecursively());
    }

    void createsDirAndIndex() {
        const FileCache cache(u"test"_s, k_maxSize, k_suffix);
        QCOMPARE(cache.dir(), m_dir);
        QCOMPARE(cache.file(), m_dir + u"/index.db"_s);
        QCOMPARE(cache.maxSize(), k_maxSize);
        QCOMPARE(cache.suffix(), k_suffix);
        QCOMPARE(cache.version(), 1);
        QCOMPARE(cache.pathFor(u"k"_s), pathFor(u"k"_s));
        QTRY_VERIFY(QFile::exists(m_dir + u"/index.db"_s));
    }

    void storeAndLookup() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        QVERIFY(cache.lookup(u"a"_s).result().isEmpty());

        cache.store(u"a"_s, bytes(4, 'a'));
        QTRY_VERIFY(!cache.lookup(u"a"_s).result().isEmpty());
        QCOMPARE(cache.lookup(u"a"_s).result(), QUrl::fromLocalFile(pathFor(u"a"_s)));

        QFile file(pathFor(u"a"_s));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), bytes(4, 'a'));
    }

    void emptyKeyIsIgnored() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        cache.store({}, bytes(4, 'a'));
        cache.store(u"b"_s, bytes(1, 'b'));
        QTRY_VERIFY(!cache.lookup(u"b"_s).result().isEmpty());
        QVERIFY(cache.lookup(QString()).result().isEmpty());
        QCOMPARE(QDir(m_dir).entryList({ u"*"_s + k_suffix }, QDir::Files), QStringList{ u"b"_s + k_suffix });
    }

    void producerRunsOnlyWhenMissing() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        std::atomic<int> calls = 0;
        const auto producer = [&calls] {
            ++calls;
            return bytes(4, 'p');
        };

        cache.store(u"p"_s, producer);
        QTRY_VERIFY(!cache.lookup(u"p"_s).result().isEmpty());
        QCOMPARE(calls.load(), 1);

        cache.store(u"p"_s, producer);
        cache.store(u"q"_s, bytes(1, 'q'));
        QTRY_VERIFY(!cache.lookup(u"q"_s).result().isEmpty());
        QCOMPARE(calls.load(), 1);
    }

    void producerReturningNullStoresNothing() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        cache.store(u"n"_s, [] {
            return QByteArray();
        });
        cache.store(u"q"_s, bytes(1, 'q'));
        QTRY_VERIFY(!cache.lookup(u"q"_s).result().isEmpty());
        QVERIFY(cache.lookup(u"n"_s).result().isEmpty());
    }

    void oversizeEntryIsRefused() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        cache.store(u"big"_s, bytes(k_maxSize + 1, 'x'));
        cache.store(u"q"_s, bytes(1, 'q'));
        QTRY_VERIFY(!cache.lookup(u"q"_s).result().isEmpty());
        QVERIFY(cache.lookup(u"big"_s).result().isEmpty());
    }

    void evictsLeastRecentlyUsed() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        cache.store(u"a"_s, bytes(4, 'a'));
        QTRY_VERIFY(!cache.lookup(u"a"_s).result().isEmpty());
        QTest::qSleep(1100);

        cache.store(u"b"_s, bytes(4, 'b'));
        QTRY_VERIFY(!cache.lookup(u"b"_s).result().isEmpty());
        QTest::qSleep(1100);

        QVERIFY(!cache.lookup(u"a"_s).result().isEmpty());
        cache.store(u"c"_s, bytes(4, 'c'));
        QTRY_VERIFY(!cache.lookup(u"c"_s).result().isEmpty());

        QTRY_VERIFY(cache.lookup(u"b"_s).result().isEmpty());
        QVERIFY(!cache.lookup(u"a"_s).result().isEmpty());
        QVERIFY(!QFile::exists(pathFor(u"b"_s)));
    }

    void removeDeletesFile() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        cache.store(u"a"_s, bytes(4, 'a'));
        QTRY_VERIFY(!cache.lookup(u"a"_s).result().isEmpty());

        cache.remove(u"a"_s);
        QTRY_VERIFY(cache.lookup(u"a"_s).result().isEmpty());
        QVERIFY(!QFile::exists(pathFor(u"a"_s)));
    }

    void clearDeletesEverything() {
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        cache.store(u"a"_s, bytes(4, 'a'));
        cache.store(u"b"_s, bytes(4, 'b'));
        QTRY_VERIFY(!cache.lookup(u"b"_s).result().isEmpty());

        cache.clear();
        QTRY_VERIFY(cache.lookup(u"a"_s).result().isEmpty());
        QCOMPARE(QDir(m_dir).entryList(QDir::Files), QStringList{ u"index.db"_s });
    }

    void reconcileAdoptsOrphansAndDropsDanglingRows() {
        {
            FileCache cache(u"test"_s, k_maxSize, k_suffix);
            cache.store(u"gone"_s, bytes(4, 'g'));
            QTRY_VERIFY(!cache.lookup(u"gone"_s).result().isEmpty());
        }
        QVERIFY(QFile::remove(pathFor(u"gone"_s)));
        writeFile(pathFor(u"orphan"_s), bytes(6, 'o'));

        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        QVERIFY(!cache.lookup(u"orphan"_s).result().isEmpty());
        QTest::qSleep(1100);

        cache.store(u"new"_s, bytes(6, 'n'));
        QTRY_VERIFY(!cache.lookup(u"new"_s).result().isEmpty());
        QTRY_VERIFY(cache.lookup(u"orphan"_s).result().isEmpty());
    }

    void entriesPersistAcrossInstances() {
        {
            FileCache cache(u"test"_s, k_maxSize, k_suffix);
            cache.store(u"a"_s, bytes(4, 'a'));
            QTRY_VERIFY(!cache.lookup(u"a"_s).result().isEmpty());
        }
        FileCache cache(u"test"_s, k_maxSize, k_suffix);
        cache.store(u"b"_s, bytes(1, 'b'));
        QTRY_VERIFY(!cache.lookup(u"b"_s).result().isEmpty());
        QVERIFY(!cache.lookup(u"a"_s).result().isEmpty());
    }

    void versionBumpWipesCache() {
        {
            FileCache cache(u"test"_s, k_maxSize, k_suffix, 1);
            cache.store(u"a"_s, bytes(4, 'a'));
            QTRY_VERIFY(!cache.lookup(u"a"_s).result().isEmpty());
        }
        {
            FileCache cache(u"test"_s, k_maxSize, k_suffix, 2);
            QTRY_VERIFY(cache.lookup(u"a"_s).result().isEmpty());
            cache.store(u"b"_s, bytes(4, 'b'));
            QTRY_VERIFY(!cache.lookup(u"b"_s).result().isEmpty());
        }
        FileCache cache(u"test"_s, k_maxSize, k_suffix, 2);
        cache.store(u"c"_s, bytes(1, 'c'));
        QTRY_VERIFY(!cache.lookup(u"c"_s).result().isEmpty());
        QVERIFY(!cache.lookup(u"b"_s).result().isEmpty());
    }

    void keyForPathHashesFileIdentityAndExtraInfo() {
        const auto path = m_root.filePath(u"source"_s);
        QVERIFY(FileCache::keyForPath(path).isEmpty());

        writeFile(path, bytes(3, 's'));
        const auto key = FileCache::keyForPath(path);
        QCOMPARE(key.size(), 64);
        QCOMPARE(FileCache::keyForPath(path), key);
        QCOMPARE_NE(FileCache::keyForPath(path, u"v1"), key);
        QCOMPARE_NE(FileCache::keyForPath(path, u"v1"), FileCache::keyForPath(path, u"v2"));

        writeFile(path, bytes(4, 's'));
        QCOMPARE_NE(FileCache::keyForPath(path), key);
    }
};

QTEST_GUILESS_MAIN(TestFileCache)

#include "filecache_test.moc"
