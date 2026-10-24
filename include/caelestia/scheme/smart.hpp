#pragma once

#include <qstring.h>

#include "caelestia/media/analysis.hpp"
#include "caelestia/scheme/scheme.hpp"

namespace caelestia::scheme {

struct SmartChoice {
    QString flavour;
    Mode mode;
};

[[nodiscard]] SmartChoice smartChoice(const media::Analysis& analysis);

} // namespace caelestia::scheme
