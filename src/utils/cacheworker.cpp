#include "cacheworker.hpp"

#include <qdatetime.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qloggingcategory.h>
#include <qsavefile.h>
#include <qset.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <quuid.h>

#include <utility>

namespace {

Q_LOGGING_CATEGORY(lcCache, "caelestia.utils.cache", QtInfoMsg)

} // namespace

namespace caelestia::utils {

using Qt::StringLiterals::operator""_s;

namespace {

qint64 now() {
    return QDateTime::currentSecsSinceEpoch();
}

} // namespace

FileCacheWorker::FileCacheWorker(const FileCache& cache, QObject* parent)
    : QObject(parent)
    , m_cache(cache)
    , m_connection(QUuid::createUuid().toString()) {}

FileCacheWorker::~FileCacheWorker() {
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connection);
}

void FileCacheWorker::open() {
    if (!QDir().mkpath(m_cache.dir())) {
        qCWarning(lcCache).noquote() << "Failed to create cache dir" << m_cache.dir();
        return;
    }

    m_db = QSqlDatabase::addDatabase(u"QSQLITE"_s, m_connection);
    m_db.setDatabaseName(m_cache.dir() + u"/index.db"_s);
    if (!m_db.open()) {
        qCWarning(lcCache).noquote() << "Failed to open cache index:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
    query.exec(u"CREATE TABLE IF NOT EXISTS entries (key TEXT PRIMARY KEY, bytes INTEGER NOT NULL, "
               "last_used INTEGER NOT NULL)"_s);

    query.exec(u"PRAGMA user_version"_s);
    if (query.next() && query.value(0).toInt() != m_cache.version()) {
        qCInfo(lcCache).noquote() << "Cache version changed, clearing" << m_cache.dir();
        reconcile();
        clear();
        query.exec(u"PRAGMA user_version = %1"_s.arg(m_cache.version()));
        return;
    }

    reconcile();
    evict();
}

void FileCacheWorker::touch(const QString& key) {
    QSqlQuery query(m_db);
    query.prepare(u"UPDATE entries SET last_used = :now WHERE key = :key"_s);
    query.bindValue(u":now"_s, now());
    query.bindValue(u":key"_s, key);
    query.exec();
}

void FileCacheWorker::store(const QString& key, const FileCache::Producer& producer) {
    if (QFile::exists(m_cache.pathFor(key)))
        return;

    const auto data = producer();
    if (data.isNull()) {
        qCWarning(lcCache).noquote() << "Producer returned no data for" << key;
        return;
    }
    store(key, data);
}

void FileCacheWorker::store(const QString& key, const QByteArray& data) {
    if (data.size() > m_cache.maxSize())
        return;

    QSaveFile file(m_cache.pathFor(key));
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        qCWarning(lcCache).noquote() << "Failed to write" << file.fileName() << file.errorString();
        return;
    }

    QSqlQuery query(m_db);
    query.prepare(u"INSERT OR REPLACE INTO entries (key, bytes, last_used) VALUES (:key, :bytes, :now)"_s);
    query.bindValue(u":key"_s, key);
    query.bindValue(u":bytes"_s, data.size());
    query.bindValue(u":now"_s, now());
    query.exec();

    qCDebug(lcCache).noquote() << "Stored" << key << data.size() << "bytes";
    evict();
}

void FileCacheWorker::remove(const QString& key) {
    QFile::remove(m_cache.pathFor(key));
    forget(key);
}

void FileCacheWorker::clear() {
    QSqlQuery query(m_db);
    query.exec(u"SELECT key FROM entries"_s);
    while (query.next())
        QFile::remove(m_cache.pathFor(query.value(0).toString()));
    query.exec(u"DELETE FROM entries"_s);
}

void FileCacheWorker::reconcile() {
    QSet<QString> indexed;
    QSqlQuery query(m_db);
    query.exec(u"SELECT key FROM entries"_s);
    while (query.next())
        indexed.insert(query.value(0).toString());

    QSet<QString> onDisk;
    const auto files =
        QDir(m_cache.dir())
            .entryInfoList({ u"*"_s + m_cache.pathFor({}).sliced(m_cache.dir().size() + 1) }, QDir::Files);
    for (const auto& info : files) {
        if (info.fileName() == u"index.db"_s)
            continue;

        const auto key = info.completeBaseName();
        onDisk.insert(key);
        if (indexed.contains(key))
            continue;

        query.prepare(u"INSERT INTO entries (key, bytes, last_used) VALUES (:key, :bytes, :now)"_s);
        query.bindValue(u":key"_s, key);
        query.bindValue(u":bytes"_s, info.size());
        query.bindValue(u":now"_s, now());
        query.exec();
    }

    for (const QString& key : indexed)
        if (!onDisk.contains(key))
            forget(key);
}

void FileCacheWorker::evict() {
    QSqlQuery query(m_db);
    query.exec(u"SELECT COALESCE(SUM(bytes), 0) FROM entries"_s);
    qint64 total = query.next() ? query.value(0).toLongLong() : 0;
    if (total <= m_cache.maxSize())
        return;

    QStringList stale;
    query.exec(u"SELECT key, bytes FROM entries ORDER BY last_used ASC"_s);
    while (total > m_cache.maxSize() && query.next()) {
        stale << query.value(0).toString();
        total -= query.value(1).toLongLong();
    }
    query.finish();

    for (const auto& key : std::as_const(stale)) {
        remove(key);
        qCDebug(lcCache).noquote() << "Evicted" << key;
    }
}

void FileCacheWorker::forget(const QString& key) {
    QSqlQuery query(m_db);
    query.prepare(u"DELETE FROM entries WHERE key = :key"_s);
    query.bindValue(u":key"_s, key);
    query.exec();
}

} // namespace caelestia::utils
