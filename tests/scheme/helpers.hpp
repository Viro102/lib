#pragma once

#include <m3colours/common.hpp>

#include <array>
#include <bit>

#include "caelestia/scheme/scheme.hpp"

inline auto allColours(const caelestia::scheme::SchemeColours& colours) {
    return std::bit_cast<std::array<m3colours::Argb, sizeof(colours) / sizeof(m3colours::Argb)>>(colours);
}
