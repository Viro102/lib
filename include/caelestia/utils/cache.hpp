#pragma once

#include <qbytearray.h>
#include <qstring.h>
#include <qthread.h>
#include <qurl.h>

#include <functional>

namespace caelestia::utils {

class FileCacheWorker;

class FileCache {
public:
    Q_DISABLE_COPY_MOVE(FileCache)

    using Producer = std::function<QByteArray()>;

    explicit FileCache(const QString& dir, qint64 maxSize, QString suffix = {}, int version = 1);
    ~FileCache();

    [[nodiscard]] static QString cacheDir();
    [[nodiscard]] QString dir() const;
    [[nodiscard]] qint64 maxSize() const;
    [[nodiscard]] QString suffix() const;
    [[nodiscard]] int version() const;

    [[nodiscard]] static QString keyFor(const QString& path, QStringView extraInfo = {});
    [[nodiscard]] QString pathFor(const QString& key) const;

    [[nodiscard]] QUrl lookup(const QString& key) const;
    void store(const QString& key, const QByteArray& data);
    void store(const QString& key, Producer producer);
    void remove(const QString& key);
    void clear();

private:
    const QString m_dir;
    const qint64 m_maxSize;
    const QString m_suffix;
    const int m_version;
    QThread m_thread;
    FileCacheWorker* m_worker;
};

} // namespace caelestia::utils
