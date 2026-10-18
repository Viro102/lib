#pragma once

#include <qobject.h>
#include <qsqldatabase.h>
#include <qstring.h>

#include "caelestia/utils/cache.hpp"

namespace caelestia::utils {

class FileCacheWorker : public QObject {
public:
    explicit FileCacheWorker(const FileCache& cache, QObject* parent = nullptr);
    ~FileCacheWorker() override;

    void open();
    void touch(const QString& key);
    void store(const QString& key, const QByteArray& data);
    void store(const QString& key, const FileCache::Producer& producer);
    void remove(const QString& key);
    void clear();

private:
    const FileCache& m_cache;
    const QString m_connection;
    QSqlDatabase m_db;

    void reconcile();
    void evict();
    void forget(const QString& key);
};

} // namespace caelestia::utils
