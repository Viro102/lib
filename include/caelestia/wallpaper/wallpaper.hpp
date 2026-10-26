#pragma once

#include <qfuture.h>
#include <qstring.h>

#include "caelestia/scheme/scheme.hpp"

namespace caelestia::wallpaper {

[[nodiscard]] QString statePath();
[[nodiscard]] QString defaultWallpaper();
[[nodiscard]] QString currentWallpaper(bool* ok = nullptr);
[[nodiscard]] QFuture<scheme::SchemeData> currentScheme(bool* ok = nullptr);
[[nodiscard]] QFuture<void> set(const QString& path, bool smart = true);

} // namespace caelestia::wallpaper
