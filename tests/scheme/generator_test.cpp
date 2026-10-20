#include <qtest.h>

#include <m3colours/hct.hpp>

#include <array>
#include <bit>
#include <utility>

#include "caelestia/scheme/generator.hpp"
#include "caelestia/scheme/scheme.hpp"
#include "helpers.hpp"

using namespace caelestia::scheme;
using m3colours::Argb;
using m3colours::Hct;
using m3colours::scheme::Variant;

namespace {

constexpr Argb k_sourceColour{ 0xFF2962FF };

constexpr std::array k_variants = {
    std::pair{ Variant::Monochrome, "Monochrome" },
    std::pair{ Variant::Neutral, "Neutral" },
    std::pair{ Variant::TonalSpot, "TonalSpot" },
    std::pair{ Variant::Vibrant, "Vibrant" },
    std::pair{ Variant::Expressive, "Expressive" },
    std::pair{ Variant::Fidelity, "Fidelity" },
    std::pair{ Variant::Content, "Content" },
    std::pair{ Variant::Rainbow, "Rainbow" },
    std::pair{ Variant::FruitSalad, "FruitSalad" },
    std::pair{ Variant::Cmf, "Cmf" },
};

bool isGrayscale(Argb colour) {
    return colour.red() == colour.green() && colour.green() == colour.blue();
}

} // namespace

class TestGenerator : public QObject {
    Q_OBJECT

private slots:

    void darkMode() {
        const auto colours = generate(k_sourceColour, Variant::TonalSpot, Mode::Dark);
        for (const auto colour : allColours(colours))
            QCOMPARE(colour.alpha(), 0xFF);
        QVERIFY(colours.background.red() < 0x80);
        QVERIFY(colours.onBackground.red() > 0x80);
    }

    void lightMode() {
        const auto colours = generate(k_sourceColour, Variant::TonalSpot, Mode::Light);
        for (const auto colour : allColours(colours))
            QCOMPARE(colour.alpha(), 0xFF);
        QVERIFY(colours.background.red() > 0x80);
        QVERIFY(colours.onBackground.red() < 0x80);
    }

    void monochrome() {
        const auto colours = generate(k_sourceColour, Variant::Monochrome, Mode::Dark);
        QVERIFY(isGrayscale(colours.primary));
        QVERIFY(isGrayscale(colours.term1));
        QVERIFY(isGrayscale(colours.red));
    }

    void variantsDiffer() {
        const auto tonal = generate(k_sourceColour, Variant::TonalSpot, Mode::Dark);
        const auto vibrant = generate(k_sourceColour, Variant::Vibrant, Mode::Dark);
        QVERIFY(allColours(tonal) != allColours(vibrant));
    }

    void everyVariantIsOpaque_data() {
        QTest::addColumn<Variant>("variant");
        QTest::addColumn<Mode>("mode");

        for (const auto& [variant, name] : k_variants) {
            QTest::addRow("%s-dark", name) << variant << Mode::Dark;
            QTest::addRow("%s-light", name) << variant << Mode::Light;
        }
    }

    void everyVariantIsOpaque() {
        QFETCH(Variant, variant);
        QFETCH(const Mode, mode);

        const auto colours = generate(k_sourceColour, variant, mode);
        for (const auto colour : allColours(colours))
            QCOMPARE(colour.alpha(), 0xFF);
    }

    void neutralReducesChroma() {
        const auto tonal = generate(k_sourceColour, Variant::TonalSpot, Mode::Dark);
        const auto neutral = generate(k_sourceColour, Variant::Neutral, Mode::Dark);
        QVERIFY(Hct(neutral.term1).chroma() < Hct(tonal.term1).chroma());
        QVERIFY(Hct(neutral.red).chroma() < Hct(tonal.red).chroma());
    }

    void isStable() {
        const auto first = generate(k_sourceColour, Variant::Vibrant, Mode::Dark);
        const auto second = generate(k_sourceColour, Variant::Vibrant, Mode::Dark);
        QCOMPARE(allColours(first), allColours(second));
    }
};

QTEST_GUILESS_MAIN(TestGenerator)

#include "generator_test.moc"
