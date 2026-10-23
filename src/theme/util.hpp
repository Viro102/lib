#pragma once

#include <qcolor.h>
#include <qloggingcategory.h>
#include <qstring.h>

namespace caelestia::theme {

Q_DECLARE_LOGGING_CATEGORY(lcTheme)

[[nodiscard]] QString configDir();
[[nodiscard]] QString dataDir();

[[nodiscard]] QString hex(QColor colour);

bool writeFile(const QString& path, const QString& content);

int run(const QString& program, const QStringList& args, const QByteArray& input = {});
void signalProcess(const QString& name, int sig);

} // namespace caelestia::theme
