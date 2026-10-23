#include "caelestia/scheme/generator.hpp"

#include <m3colours/blend.hpp>
#include <m3colours/hct.hpp>
#include <m3colours/scheme.hpp>

#include <array>

namespace caelestia::scheme {

namespace {

using m3colours::Argb;
using m3colours::Hct;

constexpr std::array k_gruvboxLight = {
    0xFFFDF9F3,
    0xFFFF6188,
    0xFFA9DC76,
    0xFFFC9867,
    0xFFFFD866,
    0xFFF47FD4,
    0xFF78DCE8,
    0xFF333034,
    0xFF121212,
    0xFFFF6188,
    0xFFA9DC76,
    0xFFFC9867,
    0xFFFFD866,
    0xFFF47FD4,
    0xFF78DCE8,
    0xFF333034,
};
constexpr std::array k_gruvboxDark = {
    0xFF282828,
    0xFFCC241D,
    0xFF98971A,
    0xFFD79921,
    0xFF458588,
    0xFFB16286,
    0xFF689D6A,
    0xFFA89984,
    0xFF928374,
    0xFFFB4934,
    0xFFB8BB26,
    0xFFFABD2F,
    0xFF83A598,
    0xFFD3869B,
    0xFF8EC07C,
    0xFFEBDBB2,
};

constexpr std::array k_catppuccinLight = {
    0xffdc8a78,
    0xffdd7878,
    0xffea76cb,
    0xff8839ef,
    0xffd20f39,
    0xffe64553,
    0xfffe640b,
    0xffdf8e1d,
    0xff40a02b,
    0xff179299,
    0xff04a5e5,
    0xff209fb5,
    0xff1e66f5,
    0xff7287fd,
};
constexpr std::array k_catppuccinDark = {
    0xfff5e0dc,
    0xfff2cdcd,
    0xfff5c2e7,
    0xffcba6f7,
    0xfff38ba8,
    0xffeba0ac,
    0xfffab387,
    0xfff9e2af,
    0xffa6e3a1,
    0xff94e2d5,
    0xff89dceb,
    0xff74c7ec,
    0xff89b4fa,
    0xffb4befe,
};

constexpr std::array k_kColours = {
    0xff2980b9,
    0xff9b59b6,
    0xffda4453,
    0xfff67400,
    0xff27ae60,
};

m3colours::scheme::DynamicScheme schemeForVariant(DynamicVariant variant, Argb sourceColour, bool isDark) {
    const Hct hct(sourceColour);

    using namespace m3colours::scheme;

    switch (variant) {
    case DynamicVariant::Monochrome:
        return SchemeMonochrome(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::Neutral:
        return SchemeNeutral(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::TonalSpot:
        return SchemeTonalSpot(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::Vibrant:
        return SchemeVibrant(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::Expressive:
        return SchemeExpressive(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::Fidelity:
        return SchemeFidelity(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::Content:
        return SchemeContent(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::Rainbow:
        return SchemeRainbow(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::FruitSalad:
        return SchemeFruitSalad(hct, isDark, 0.0, SpecVersion::Spec2025);
    case DynamicVariant::Cmf:
        return SchemeCmf(hct, isDark, 0.0);
    }

    Q_UNREACHABLE();
}

Argb darken(Argb colour, qreal amount) {
    const Hct hct(colour);
    const auto diff = hct.tone() * amount;
    return Hct(hct.hue(), hct.chroma() - diff / 5, hct.tone() - diff).toArgb();
}

Argb lighten(Argb colour, qreal amount) {
    const Hct hct(colour);
    const auto diff = (100 - hct.tone()) * amount;
    return Hct(hct.hue(), hct.chroma() + diff / 5, hct.tone() + diff).toArgb();
}

Argb grayscale(Argb colour, Mode mode) {
    Hct hct(mode == Mode::Light ? darken(colour, 0.35) : lighten(colour, 0.65));
    hct.setChroma(0.0);
    return hct.toArgb();
}

Argb adjustNeutral(Argb colour, DynamicVariant flavour) {
    if (flavour != DynamicVariant::Neutral)
        return colour;

    Hct hct(colour);
    hct.setChroma(hct.chroma() - 15);
    return hct.toArgb();
}

QColor adjustGrayscale(Argb colour, DynamicVariant flavour, Mode mode) {
    colour = adjustNeutral(colour, flavour);

    if (flavour == DynamicVariant::Monochrome)
        return QColor::fromRgba(grayscale(colour, mode).value());
    return QColor::fromRgba(colour.value());
}

QColor adjusted(Argb colour, Argb blendColour, DynamicVariant flavour, Mode mode, qreal toneBoost) {
    if (flavour == DynamicVariant::Monochrome)
        return QColor::fromRgba(grayscale(colour, mode).value());

    Hct hct(m3colours::blend::harmonise(colour, blendColour));
    hct.setTone(hct.tone() * (1.0 + toneBoost));

    if (flavour == DynamicVariant::Neutral)
        hct.setChroma(hct.chroma() - 15);

    return QColor::fromRgba(hct.toArgb().value());
}

} // namespace

SchemeColours generate(QColor sourceColour, DynamicVariant flavour, Mode mode) {
    const auto isDark = mode == Mode::Dark;
    const auto scheme = schemeForVariant(flavour, Argb{ sourceColour.rgba() }, isDark);
    const auto paletteColour = scheme.primaryPaletteKeyColour();
    const auto primary = scheme.primary();
    const auto onPrimaryFixedVariant = scheme.onPrimaryFixedVariant();

    const auto termRef = isDark ? k_gruvboxDark : k_gruvboxLight;
    const auto namedRef = isDark ? k_catppuccinDark : k_catppuccinLight;

#define COLOUR(name) .name = QColor::fromRgba(adjustNeutral(scheme.name(), flavour).value())
#define TERM(i)                                                                                                        \
    .term##i = adjusted(Argb{ termRef[i] }, paletteColour, flavour, mode, (i < 8 ? 0.35 : 0.2) * (isDark ? 1.0 : -1.0))
#define NAMED(name, i) .name = adjusted(Argb{ namedRef[i] }, paletteColour, flavour, mode, isDark ? 0.05 : -0.2)
#define K_COLOUR(name, i)                                                                                              \
    .name = adjusted(Argb{ k_kColours[i] }, primary, flavour, mode, 0.1),                                              \
    .name##Selection = adjusted(Argb{ k_kColours[i] }, onPrimaryFixedVariant, flavour, mode, 0.1)

    return {
        COLOUR(primaryPaletteKeyColour),
        COLOUR(secondaryPaletteKeyColour),
        COLOUR(tertiaryPaletteKeyColour),
        COLOUR(neutralPaletteKeyColour),
        COLOUR(neutralVariantPaletteKeyColour),
        COLOUR(errorPaletteKeyColour),
        COLOUR(background),
        COLOUR(onBackground),
        COLOUR(surface),
        COLOUR(surfaceDim),
        COLOUR(surfaceBright),
        COLOUR(surfaceContainerLowest),
        COLOUR(surfaceContainerLow),
        COLOUR(surfaceContainer),
        COLOUR(surfaceContainerHigh),
        COLOUR(surfaceContainerHighest),
        COLOUR(onSurface),
        COLOUR(onSurfaceVariant),
        COLOUR(outline),
        COLOUR(outlineVariant),
        COLOUR(inverseSurface),
        COLOUR(inverseOnSurface),
        COLOUR(primary),
        COLOUR(onPrimary),
        COLOUR(primaryContainer),
        COLOUR(onPrimaryContainer),
        COLOUR(primaryFixed),
        COLOUR(primaryFixedDim),
        COLOUR(onPrimaryFixed),
        COLOUR(onPrimaryFixedVariant),
        COLOUR(inversePrimary),
        COLOUR(secondary),
        COLOUR(onSecondary),
        COLOUR(secondaryContainer),
        COLOUR(onSecondaryContainer),
        COLOUR(secondaryFixed),
        COLOUR(secondaryFixedDim),
        COLOUR(onSecondaryFixed),
        COLOUR(onSecondaryFixedVariant),
        COLOUR(tertiary),
        COLOUR(onTertiary),
        COLOUR(tertiaryContainer),
        COLOUR(onTertiaryContainer),
        COLOUR(tertiaryFixed),
        COLOUR(tertiaryFixedDim),
        COLOUR(onTertiaryFixed),
        COLOUR(onTertiaryFixedVariant),
        COLOUR(error),
        COLOUR(onError),
        COLOUR(errorContainer),
        COLOUR(onErrorContainer),

        .success = adjustGrayscale(Argb{ isDark ? 0xFFB5CCBA : 0xFF4F6354 }, flavour, mode),
        .onSuccess = adjustGrayscale(Argb{ isDark ? 0xFF213528 : 0xFFFFFFFF }, flavour, mode),
        .successContainer = adjustGrayscale(Argb{ isDark ? 0xFF374B3E : 0xFFD1E8D5 }, flavour, mode),
        .onSuccessContainer = adjustGrayscale(Argb{ isDark ? 0xFFD1E9D6 : 0xFF0C1F13 }, flavour, mode),

        TERM(0),
        TERM(1),
        TERM(2),
        TERM(3),
        TERM(4),
        TERM(5),
        TERM(6),
        TERM(7),
        TERM(8),
        TERM(9),
        TERM(10),
        TERM(11),
        TERM(12),
        TERM(13),
        TERM(14),
        TERM(15),

        NAMED(rosewater, 0),
        NAMED(flamingo, 1),
        NAMED(pink, 2),
        NAMED(mauve, 3),
        NAMED(red, 4),
        NAMED(maroon, 5),
        NAMED(peach, 6),
        NAMED(yellow, 7),
        NAMED(green, 8),
        NAMED(teal, 9),
        NAMED(sky, 10),
        NAMED(sapphire, 11),
        NAMED(blue, 12),
        NAMED(lavender, 13),

        K_COLOUR(klink, 0),
        K_COLOUR(kvisited, 1),
        K_COLOUR(knegative, 2),
        K_COLOUR(kneutral, 3),
        K_COLOUR(kpositive, 4),
    };

#undef COLOUR
#undef TERM
#undef NAMED
#undef K_COLOUR
}

} // namespace caelestia::scheme
