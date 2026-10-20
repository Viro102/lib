#pragma once

#include <qbytearray.h>
#include <qfuture.h>
#include <qimage.h>
#include <qlist.h>
#include <qstring.h>

#include <m3colours/scheme.hpp>

#include <optional>

namespace caelestia::scheme {

struct SchemeColours {
    // Material 3, 2025 spec
    m3colours::Argb primaryPaletteKeyColour;
    m3colours::Argb secondaryPaletteKeyColour;
    m3colours::Argb tertiaryPaletteKeyColour;
    m3colours::Argb neutralPaletteKeyColour;
    m3colours::Argb neutralVariantPaletteKeyColour;
    m3colours::Argb errorPaletteKeyColour;
    m3colours::Argb background;
    m3colours::Argb onBackground;
    m3colours::Argb surface;
    m3colours::Argb surfaceDim;
    m3colours::Argb surfaceBright;
    m3colours::Argb surfaceContainerLowest;
    m3colours::Argb surfaceContainerLow;
    m3colours::Argb surfaceContainer;
    m3colours::Argb surfaceContainerHigh;
    m3colours::Argb surfaceContainerHighest;
    m3colours::Argb onSurface;
    m3colours::Argb onSurfaceVariant;
    m3colours::Argb outline;
    m3colours::Argb outlineVariant;
    m3colours::Argb inverseSurface;
    m3colours::Argb inverseOnSurface;
    m3colours::Argb primary;
    m3colours::Argb onPrimary;
    m3colours::Argb primaryContainer;
    m3colours::Argb onPrimaryContainer;
    m3colours::Argb primaryFixed;
    m3colours::Argb primaryFixedDim;
    m3colours::Argb onPrimaryFixed;
    m3colours::Argb onPrimaryFixedVariant;
    m3colours::Argb inversePrimary;
    m3colours::Argb secondary;
    m3colours::Argb onSecondary;
    m3colours::Argb secondaryContainer;
    m3colours::Argb onSecondaryContainer;
    m3colours::Argb secondaryFixed;
    m3colours::Argb secondaryFixedDim;
    m3colours::Argb onSecondaryFixed;
    m3colours::Argb onSecondaryFixedVariant;
    m3colours::Argb tertiary;
    m3colours::Argb onTertiary;
    m3colours::Argb tertiaryContainer;
    m3colours::Argb onTertiaryContainer;
    m3colours::Argb tertiaryFixed;
    m3colours::Argb tertiaryFixedDim;
    m3colours::Argb onTertiaryFixed;
    m3colours::Argb onTertiaryFixedVariant;
    m3colours::Argb error;
    m3colours::Argb onError;
    m3colours::Argb errorContainer;
    m3colours::Argb onErrorContainer;

    // Extended material
    m3colours::Argb success;
    m3colours::Argb onSuccess;
    m3colours::Argb successContainer;
    m3colours::Argb onSuccessContainer;

    // Terminal colours
    m3colours::Argb term0;
    m3colours::Argb term1;
    m3colours::Argb term2;
    m3colours::Argb term3;
    m3colours::Argb term4;
    m3colours::Argb term5;
    m3colours::Argb term6;
    m3colours::Argb term7;
    m3colours::Argb term8;
    m3colours::Argb term9;
    m3colours::Argb term10;
    m3colours::Argb term11;
    m3colours::Argb term12;
    m3colours::Argb term13;
    m3colours::Argb term14;
    m3colours::Argb term15;

    // Named colours
    m3colours::Argb rosewater;
    m3colours::Argb flamingo;
    m3colours::Argb pink;
    m3colours::Argb mauve;
    m3colours::Argb red;
    m3colours::Argb maroon;
    m3colours::Argb peach;
    m3colours::Argb yellow;
    m3colours::Argb green;
    m3colours::Argb teal;
    m3colours::Argb sky;
    m3colours::Argb sapphire;
    m3colours::Argb blue;
    m3colours::Argb lavender;

    // KDE colours (for Qt theming)
    m3colours::Argb klink;
    m3colours::Argb klinkSelection;
    m3colours::Argb kvisited;
    m3colours::Argb kvisitedSelection;
    m3colours::Argb knegative;
    m3colours::Argb knegativeSelection;
    m3colours::Argb kneutral;
    m3colours::Argb kneutralSelection;
    m3colours::Argb kpositive;
    m3colours::Argb kpositiveSelection;
};

struct Scheme {
    enum class Mode : quint8 {
        Light,
        Dark,
    };

    const QString name;
    const QString flavour;
    const Mode mode;
    const SchemeColours colours;
};

struct NamedColour {
    QStringView name;
    m3colours::Argb colour;
};

QList<NamedColour> namedColours(const SchemeColours& colours);
[[nodiscard]] std::optional<SchemeColours> coloursFromBytes(const QByteArray& bytes);
[[nodiscard]] QByteArray coloursToBytes(const SchemeColours& colours);
QString modeName(Scheme::Mode mode);

QFuture<Scheme> schemeForName(const QString& name, const QString& flavour, Scheme::Mode mode);
Scheme schemeForImage(const QImage& image, m3colours::scheme::Variant flavour, Scheme::Mode mode);
QFuture<Scheme> schemeForPath(const QString& path, m3colours::scheme::Variant flavour, Scheme::Mode mode);

} // namespace caelestia::scheme
