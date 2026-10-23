#pragma once

#include <qcolor.h>
#include <qfuture.h>
#include <qimage.h>
#include <qlist.h>
#include <qstring.h>

namespace caelestia::media {

struct Analysis {
    QList<QColor> colours; // Scored palette, most suitable first
    QColor dominant;
    double colourfulness = 0;
    double luminance = 0;
    double tone = 0;
};

[[nodiscard]] bool isSupported(const QString& path);
Analysis analyseImage(const QImage& image);
QFuture<Analysis> analyseForPath(const QString& path);

} // namespace caelestia::media
