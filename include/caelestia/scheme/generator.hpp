#pragma once

#include "caelestia/scheme/scheme.hpp"

namespace caelestia::scheme {

enum class DynamicVariant : quint8 {
    Monochrome,
    Neutral,
    TonalSpot,
    Vibrant,
    Expressive,
    Fidelity,
    Content,
    Rainbow,
    FruitSalad,
    Cmf,
};

[[nodiscard]] SchemeColours generate(QColor sourceColour, DynamicVariant flavour, Mode mode);

} // namespace caelestia::scheme
