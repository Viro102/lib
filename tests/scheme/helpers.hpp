#pragma once

#include <qcolor.h>
#include <qlist.h>

#include "caelestia/scheme/scheme.hpp"

inline QList<QColor> allColours(const caelestia::scheme::SchemeColours& colours) {
    QList<QColor> out;
    const auto named = caelestia::scheme::namedColours(colours);
    for (const auto& [name, colour] : named)
        out.append(colour);
    return out;
}
