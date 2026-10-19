#include <qtest.h>

#include <array>
#include <bit>

#include "caelestia/scheme/scheme.hpp"
#include "generator.hpp"

using namespace caelestia::scheme;
using m3colours::Argb;
using m3colours::scheme::Variant;

namespace {

constexpr Argb k_sourceColour{ 0xFF2962FF };

auto allColours(const SchemeColours& colours) {
    return std::bit_cast<std::array<Argb, sizeof(SchemeColours) / sizeof(Argb)>>(colours);
}

bool isGrayscale(Argb colour) {
    return colour.red() == colour.green() && colour.green() == colour.blue();
}

} // namespace

class TestGenerator : public QObject {
    Q_OBJECT

private slots:

    void darkMode() {
        const auto colours = gen::genScheme(k_sourceColour, Variant::TonalSpot, Scheme::Mode::Dark);
        for (const auto colour : allColours(colours))
            QCOMPARE(colour.alpha(), 0xFF);
        QVERIFY(colours.background.red() < 0x80);
        QVERIFY(colours.onBackground.red() > 0x80);
    }

    void lightMode() {
        const auto colours = gen::genScheme(k_sourceColour, Variant::TonalSpot, Scheme::Mode::Light);
        for (const auto colour : allColours(colours))
            QCOMPARE(colour.alpha(), 0xFF);
        QVERIFY(colours.background.red() > 0x80);
        QVERIFY(colours.onBackground.red() < 0x80);
    }

    void monochrome() {
        const auto colours = gen::genScheme(k_sourceColour, Variant::Monochrome, Scheme::Mode::Dark);
        QVERIFY(isGrayscale(colours.primary));
        QVERIFY(isGrayscale(colours.term1));
        QVERIFY(isGrayscale(colours.red));
    }

    void variantsDiffer() {
        const auto tonal = gen::genScheme(k_sourceColour, Variant::TonalSpot, Scheme::Mode::Dark);
        const auto vibrant = gen::genScheme(k_sourceColour, Variant::Vibrant, Scheme::Mode::Dark);
        QVERIFY(allColours(tonal) != allColours(vibrant));
    }

    void isStable() {
        const auto first = gen::genScheme(k_sourceColour, Variant::Vibrant, Scheme::Mode::Dark);
        const auto second = gen::genScheme(k_sourceColour, Variant::Vibrant, Scheme::Mode::Dark);
        QCOMPARE(allColours(first), allColours(second));
    }
};

QTEST_GUILESS_MAIN(TestGenerator)

#include "generator_test.moc"
