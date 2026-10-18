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

QByteArray bytes(int n, char c) {
    return { n, c };
}

} // namespace

class TestBlobCache : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    QString m_file;

private slots:

    void init() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CACHE_HOME", m_root.path().toUtf8());
        m_file = BlobCache::cacheDir() + u"/test.db"_s;
        QVERIFY(!QFile::exists(m_file) || QFile::remove(m_file));
    }

    void createsDatabase() {
        const BlobCache cache(u"test"_s, k_maxSize);
        QCOMPARE(cache.file(), m_file);
        QCOMPARE(cache.maxSize(), k_maxSize);
        QCOMPARE(cache.version(), 1);
        QTRY_VERIFY(QFile::exists(m_file));
    }

    void nestedNameCreatesParentDirs() {
        BlobCache cache(u"test/nested/blobs"_s, k_maxSize);
        QCOMPARE(cache.file(), BlobCache::cacheDir() + u"/test/nested/blobs.db"_s);
        cache.store(u"a"_s, bytes(4, 'a'));
        QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
        QVERIFY(QFile::exists(cache.file()));
    }

    void storeAndLookup() {
        BlobCache cache(u"test"_s, k_maxSize);
        QVERIFY(cache.lookup(u"a"_s).result().isNull());

        cache.store(u"a"_s, bytes(4, 'a'));
        QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
    }

    void emptyKeyIsIgnored() {
        BlobCache cache(u"test"_s, k_maxSize);
        cache.store({}, bytes(4, 'a'));
        cache.store(u"b"_s, bytes(1, 'b'));
        QCOMPARE(cache.lookup(u"b"_s).result(), bytes(1, 'b'));
        QVERIFY(cache.lookup(QString()).result().isNull());
    }

    void producerRunsOnlyWhenMissing() {
        BlobCache cache(u"test"_s, k_maxSize);
        std::atomic<int> calls = 0;
        const auto producer = [&calls] {
            ++calls;
            return bytes(4, 'p');
        };

        cache.store(u"p"_s, producer);
        QCOMPARE(cache.lookup(u"p"_s).result(), bytes(4, 'p'));
        QCOMPARE(calls.load(), 1);

        cache.store(u"p"_s, producer);
        QCOMPARE(cache.lookup(u"p"_s).result(), bytes(4, 'p'));
        QCOMPARE(calls.load(), 1);
    }

    void producerReturningNullStoresNothing() {
        BlobCache cache(u"test"_s, k_maxSize);
        cache.store(u"n"_s, [] {
            return QByteArray();
        });
        QVERIFY(cache.lookup(u"n"_s).result().isNull());
    }

    void oversizeEntryIsRefused() {
        BlobCache cache(u"test"_s, k_maxSize);
        cache.store(u"big"_s, bytes(k_maxSize + 1, 'x'));
        QVERIFY(cache.lookup(u"big"_s).result().isNull());
    }

    void evictsLeastRecentlyUsed() {
        BlobCache cache(u"test"_s, k_maxSize);
        cache.store(u"a"_s, bytes(4, 'a'));
        QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
        QTest::qSleep(1100);

        cache.store(u"b"_s, bytes(4, 'b'));
        QCOMPARE(cache.lookup(u"b"_s).result(), bytes(4, 'b'));
        QTest::qSleep(1100);

        QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
        cache.store(u"c"_s, bytes(4, 'c'));
        QCOMPARE(cache.lookup(u"c"_s).result(), bytes(4, 'c'));

        QVERIFY(cache.lookup(u"b"_s).result().isNull());
        QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
    }

    void removeDeletesEntry() {
        BlobCache cache(u"test"_s, k_maxSize);
        cache.store(u"a"_s, bytes(4, 'a'));
        QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));

        cache.remove(u"a"_s);
        QVERIFY(cache.lookup(u"a"_s).result().isNull());
    }

    void clearDeletesEverything() {
        BlobCache cache(u"test"_s, k_maxSize);
        cache.store(u"a"_s, bytes(4, 'a'));
        cache.store(u"b"_s, bytes(4, 'b'));
        QCOMPARE(cache.lookup(u"b"_s).result(), bytes(4, 'b'));

        cache.clear();
        QVERIFY(cache.lookup(u"a"_s).result().isNull());
        QVERIFY(cache.lookup(u"b"_s).result().isNull());
    }

    void entriesPersistAcrossInstances() {
        {
            BlobCache cache(u"test"_s, k_maxSize);
            cache.store(u"a"_s, bytes(4, 'a'));
            QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
        }
        BlobCache cache(u"test"_s, k_maxSize);
        QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
    }

    void versionBumpWipesCache() {
        {
            BlobCache cache(u"test"_s, k_maxSize, 1);
            cache.store(u"a"_s, bytes(4, 'a'));
            QCOMPARE(cache.lookup(u"a"_s).result(), bytes(4, 'a'));
        }
        {
            BlobCache cache(u"test"_s, k_maxSize, 2);
            QVERIFY(cache.lookup(u"a"_s).result().isNull());
            cache.store(u"b"_s, bytes(4, 'b'));
            QCOMPARE(cache.lookup(u"b"_s).result(), bytes(4, 'b'));
        }
        BlobCache cache(u"test"_s, k_maxSize, 2);
        QCOMPARE(cache.lookup(u"b"_s).result(), bytes(4, 'b'));
    }
};

QTEST_GUILESS_MAIN(TestBlobCache)

#include "blobcache_test.moc"
