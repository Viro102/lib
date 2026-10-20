#pragma once

#include <m3colours/scheme.hpp>

#include "caelestia/scheme/scheme.hpp"

namespace caelestia::scheme {

[[nodiscard]] SchemeColours generate(m3colours::Argb sourceColour, m3colours::scheme::Variant flavour, Mode mode);

} // namespace caelestia::scheme
