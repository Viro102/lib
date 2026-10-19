#pragma once

#include <qsize.h>
#include <qstring.h>

#include <m3colours/common.hpp>

#include <cstddef>
#include <vector>

namespace caelestia::scheme {

QSize sampleSize(int width, int height, std::size_t targetPixels);
std::vector<m3colours::Argb> videoPixels(const QString& path, int samples, std::size_t pixelsPerFrame);

} // namespace caelestia::scheme
