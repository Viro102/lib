#pragma once

#include <qfuture.h>
#include <qimage.h>
#include <qlist.h>
#include <qstring.h>

#include <m3colours/common.hpp>

namespace caelestia::media {

struct Analysis {
    QList<m3colours::Argb> colours; // Scored palette, most suitable first
    m3colours::Argb dominant{ 0 };
    double colourfulness = 0;
    double luminance = 0;
    double tone = 0;
};

Analysis analyseImage(const QImage& image);
QFuture<Analysis> analyseForPath(const QString& path);

} // namespace caelestia::media
