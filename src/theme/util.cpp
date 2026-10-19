#include "util.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qprocess.h>
#include <qsavefile.h>
#include <qstandardpaths.h>

#include <csignal>

namespace caelestia::theme {

using Qt::StringLiterals::operator""_s;

Q_LOGGING_CATEGORY(lcTheme, "caelestia.theme", QtInfoMsg)

QString configDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
}

QString dataDir() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
}

QString hex(m3colours::Argb colour) {
    return u"%1"_s.arg(colour.value() & 0xFFFFFFu, 6, 16, QChar(u'0'));
}

bool writeFile(const QString& path, const QString& content) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        qCWarning(lcTheme) << "Unable to create directory for" << path;
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(lcTheme) << "Unable to open" << path << file.errorString();
        return false;
    }

    file.write(content.toUtf8());
    if (!file.commit()) {
        qCWarning(lcTheme) << "Unable to write" << path << file.errorString();
        return false;
    }
    return true;
}

int run(const QString& program, const QStringList& args, const QByteArray& input) {
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process.setStandardOutputFile(QProcess::nullDevice());
    process.start(program, args);
    if (!process.waitForStarted()) {
        qCDebug(lcTheme) << "Unable to start" << program << process.errorString();
        return -1;
    }
    process.write(input);
    process.closeWriteChannel();
    process.waitForFinished(-1);
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}

void signalProcess(const QString& name, int sig) {
    const auto entries = QDir(u"/proc"_s).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const auto& entry : entries) {
        bool isPid = false;
        const auto pid = entry.toInt(&isPid);
        if (!isPid)
            continue;

        QFile comm(u"/proc/%1/comm"_s.arg(entry));
        if (!comm.open(QIODevice::ReadOnly))
            continue;

        if (QString::fromUtf8(comm.readAll()).trimmed() == name)
            kill(pid, sig);
    }
}

} // namespace caelestia::theme
