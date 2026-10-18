#pragma once

#include <qstring.h>

#include <m3colours/scheme.hpp>

namespace caelestia::scheme {

QFuture<QList<m3colours::Argb>> coloursFor(const QString& path);

} // namespace caelestia::scheme
