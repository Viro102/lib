#include "caelestia/utils/cache.hpp"

#include <qcryptographichash.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstandardpaths.h>

#include <utility>

#include "cacheworker.hpp"

namespace caelestia::utils {

using Qt::StringLiterals::operator""_s;

FileCache::FileCache(const QString& dir, qint64 maxSize, QString suffix, int version)
    : m_dir(cacheDir() + u'/' + dir)
    , m_maxSize(maxSize)
    , m_suffix(std::move(suffix))
    , m_version(version)
    , m_worker(new FileCacheWorker(*this)) {
    m_worker->moveToThread(&m_thread);
    QObject::connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start(QThread::LowPriority);
    QMetaObject::invokeMethod(m_worker, &FileCacheWorker::open, Qt::QueuedConnection);
}

FileCache::~FileCache() {
    m_thread.quit();
    m_thread.wait();
}

QString FileCache::cacheDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + u"/caelestia"_s;
}

QString FileCache::dir() const {
    return m_dir;
}

qint64 FileCache::maxSize() const {
    return m_maxSize;
}

QString FileCache::suffix() const {
    return m_suffix;
}

int FileCache::version() const {
    return m_version;
}

QString FileCache::keyFor(const QString& path, QStringView extraInfo) {
    const QFileInfo info(path);
    if (!info.exists())
        return {};

    const auto raw = u"%1|%2|%3|%4"_s.arg(info.absoluteFilePath(),
        QString::number(info.lastModified().toSecsSinceEpoch()), QString::number(info.size()), extraInfo);
    return QString::fromLatin1(QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString FileCache::pathFor(const QString& key) const {
    return m_dir + u'/' + key + m_suffix;
}

QUrl FileCache::lookup(const QString& key) const {
    if (key.isEmpty())
        return {};

    const auto path = pathFor(key);
    if (!QFile::exists(path))
        return {};

    QMetaObject::invokeMethod(
        m_worker,
        [worker = m_worker, key] {
            worker->touch(key);
        },
        Qt::QueuedConnection);
    return QUrl::fromLocalFile(path);
}

void FileCache::store(const QString& key, const QByteArray& data) {
    if (key.isEmpty())
        return;

    QMetaObject::invokeMethod(
        m_worker,
        [worker = m_worker, key, data] {
            worker->store(key, data);
        },
        Qt::QueuedConnection);
}

void FileCache::store(const QString& key, Producer producer) {
    if (key.isEmpty())
        return;

    QMetaObject::invokeMethod(
        m_worker,
        [worker = m_worker, key, producer = std::move(producer)] {
            worker->store(key, producer);
        },
        Qt::QueuedConnection);
}

void FileCache::remove(const QString& key) {
    QMetaObject::invokeMethod(
        m_worker,
        [worker = m_worker, key] {
            worker->remove(key);
        },
        Qt::QueuedConnection);
}

void FileCache::clear() {
    QMetaObject::invokeMethod(m_worker, &FileCacheWorker::clear, Qt::QueuedConnection);
}

} // namespace caelestia::utils
