#pragma once

#include <qbytearray.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qstring.h>
#include <qtest.h>

inline void writeFile(const QString& path, const QByteArray& data) {
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(data), data.size());
}

inline QString readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

inline QJsonObject readJson(const QString& path) {
    return QJsonDocument::fromJson(readFile(path).toUtf8()).object();
}

inline QByteArray bytes(int n, char c) {
    return { n, c };
}
