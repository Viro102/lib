#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

#include "caelestia/config/paths.hpp"
#include "caelestia/scheme/scheme.hpp"
#include "generator.hpp"
#include "schemecache.hpp"

using namespace caelestia::scheme;
using m3colours::Argb;
using m3colours::scheme::Variant;
using Qt::StringLiterals::operator""_s;

namespace {

constexpr Argb k_sourceColour{ 0xFF2962FF };
constexpr Argb k_fallbackColour{ 0xFF4285F4 };

const std::array k_variantNames = {
    std::pair{ Variant::Monochrome, u"monochrome"_s },
    std::pair{ Variant::Neutral, u"neutral"_s },
    std::pair{ Variant::TonalSpot, u"tonalspot"_s },
    std::pair{ Variant::Vibrant, u"vibrant"_s },
    std::pair{ Variant::Expressive, u"expressive"_s },
    std::pair{ Variant::Fidelity, u"fidelity"_s },
    std::pair{ Variant::Content, u"content"_s },
    std::pair{ Variant::Rainbow, u"rainbow"_s },
    std::pair{ Variant::FruitSalad, u"fruitsalad"_s },
    std::pair{ Variant::Cmf, u"cmf"_s },
};

QByteArray toBytes(const SchemeColours& colours) {
    return { reinterpret_cast<const char*>(&colours), sizeof(colours) };
}

auto allColours(const SchemeColours& colours) {
    return std::bit_cast<std::array<Argb, sizeof(SchemeColours) / sizeof(Argb)>>(colours);
}

} // namespace

class TestScheme : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    QImage m_image;
    QString m_path;

    void installCustomScheme(const QString& source, const QString& name, const QString& flavour) {
        const auto dir = caelestia::config::ConfigPaths::customSchemesDir() + u"/%1/%2"_s.arg(name, flavour);
        QVERIFY(QDir().mkpath(dir));
        QVERIFY(QFile::copy(source, dir + u"/dark.txt"_s));
    }

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CACHE_HOME", m_root.path().toUtf8());
        qputenv("XDG_CONFIG_HOME", m_root.path().toUtf8());

        m_image = QImage(8, 8, QImage::Format_RGB32);
        m_image.fill(k_sourceColour.value());
        m_path = m_root.filePath(u"wallpaper.png"_s);
        QVERIFY(m_image.save(m_path));
    }

    void staticSchemeLoads() {
        const auto scheme = schemeForName(u"caelestia"_s, u"default"_s, Scheme::Mode::Dark).result();
        QCOMPARE(scheme.name, u"caelestia"_s);
        QCOMPARE(scheme.flavour, u"default"_s);
        QCOMPARE(scheme.mode, Scheme::Mode::Dark);
        QCOMPARE(scheme.colours.background, Argb(0xFF0A0F0F));
        QCOMPARE(scheme.colours.onBackground, Argb(0xFFDCE8E6));
    }

    void staticSchemeLightMode() {
        const auto scheme = schemeForName(u"caelestia"_s, u"default"_s, Scheme::Mode::Light).result();
        QCOMPARE(scheme.mode, Scheme::Mode::Light);
        QVERIFY(scheme.colours.background.red() > 0x80);
    }

    void bundledSchemesHaveAllColours() {
        QDirIterator it(u":/schemes"_s, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo info(it.next());
            if (!info.isFile())
                continue;

            const auto parts = info.path().split(u'/', Qt::SkipEmptyParts);
            const auto& name = parts[parts.size() - 2];
            const auto& flavour = parts.last();
            const auto mode = info.baseName() == u"light" ? Scheme::Mode::Light : Scheme::Mode::Dark;
            const auto tag = info.filePath().toUtf8();

            const auto scheme = schemeForName(name, flavour, mode).result();
            QCOMPARE(scheme.name, name);
            QCOMPARE(scheme.flavour, flavour);
            QCOMPARE(scheme.mode, mode);
            for (const auto colour : allColours(scheme.colours))
                QVERIFY2(colour.alpha() == 0xFF, tag.constData());
        }
    }

    void missingStaticSchemeThrows() {
        const auto future = schemeForName(u"missing"_s, u"default"_s, Scheme::Mode::Dark);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, future.result());
    }

    void customSchemeLoads() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"mine"_s, u"custom"_s);
        const auto custom = schemeForName(u"mine"_s, u"custom"_s, Scheme::Mode::Dark).result();
        const auto bundled = schemeForName(u"dracula"_s, u"medium"_s, Scheme::Mode::Dark).result();
        QCOMPARE(custom.name, u"mine"_s);
        QCOMPARE(custom.flavour, u"custom"_s);
        QCOMPARE(custom.mode, Scheme::Mode::Dark);
        QCOMPARE(allColours(custom.colours), allColours(bundled.colours));
    }

    void customSchemeMissingModeThrows() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"darkonly"_s, u"custom"_s);
        const auto future = schemeForName(u"darkonly"_s, u"custom"_s, Scheme::Mode::Light);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, future.result());
    }

    void bundledSchemeShadowsCustom() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"caelestia"_s, u"default"_s);
        const auto scheme = schemeForName(u"caelestia"_s, u"default"_s, Scheme::Mode::Dark).result();
        QCOMPARE(scheme.colours.background, Argb(0xFF0A0F0F));
    }

    void customSchemeParsing() {
        const auto dir = caelestia::config::ConfigPaths::customSchemesDir() + u"/handwritten/custom"_s;
        QVERIFY(QDir().mkpath(dir));
        QFile file(dir + u"/dark.txt"_s);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("// secondary 0000ff\n"
                   "background #112233\n"
                   "onBackground 445566\n"
                   "surface\n"
                   "outline 778899 aabbcc\n"
                   "primary #ABCDEF\n");
        file.close();

        const auto scheme = schemeForName(u"handwritten"_s, u"custom"_s, Scheme::Mode::Dark).result();
        QCOMPARE(scheme.colours.background, Argb(0xFF112233));
        QCOMPARE(scheme.colours.onBackground, Argb(0xFF445566));
        QCOMPARE(scheme.colours.primary, Argb(0xFFABCDEF));
        QCOMPARE(scheme.colours.secondary, Argb(0));
        QCOMPARE(scheme.colours.surface, Argb(0));
        QCOMPARE(scheme.colours.outline, Argb(0));
        QCOMPARE(scheme.colours.error, Argb(0));
    }

    void namedColoursMatchStruct() {
        const auto colours = gen::genScheme(k_sourceColour, Variant::TonalSpot, Scheme::Mode::Dark);
        const auto named = namedColours(colours);
        const auto all = allColours(colours);
        QCOMPARE(named.size(), static_cast<qsizetype>(all.size()));
        for (qsizetype i = 0; i < named.size(); ++i)
            QVERIFY2(named[i].colour == all[static_cast<size_t>(i)], named[i].name.toUtf8().constData());
    }

    void modeNames() {
        QCOMPARE(modeName(Scheme::Mode::Light), u"light"_s);
        QCOMPARE(modeName(Scheme::Mode::Dark), u"dark"_s);
    }

    void flavourNames() {
        for (const auto& [variant, name] : k_variantNames)
            QCOMPARE(schemeForImage(m_image, variant, Scheme::Mode::Dark).flavour, name);
    }

    void schemeFromImage() {
        const auto scheme = schemeForImage(m_image, Variant::TonalSpot, Scheme::Mode::Dark);
        QCOMPARE(scheme.name, u"dynamic"_s);
        QCOMPARE(scheme.flavour, u"tonalspot"_s);
        QCOMPARE(scheme.mode, Scheme::Mode::Dark);
        QCOMPARE(allColours(scheme.colours),
            allColours(gen::genScheme(k_sourceColour, Variant::TonalSpot, Scheme::Mode::Dark)));
    }

    void schemeFromImageLightMode() {
        const auto scheme = schemeForImage(m_image, Variant::Vibrant, Scheme::Mode::Light);
        QCOMPARE(scheme.flavour, u"vibrant"_s);
        QCOMPARE(scheme.mode, Scheme::Mode::Light);
        QCOMPARE(allColours(scheme.colours),
            allColours(gen::genScheme(k_sourceColour, Variant::Vibrant, Scheme::Mode::Light)));
    }

    void schemeFromPath() {
        const auto scheme = schemeForPath(m_path, Variant::TonalSpot, Scheme::Mode::Dark).result();
        QCOMPARE(scheme.name, u"dynamic"_s);
        QCOMPARE(scheme.flavour, u"tonalspot"_s);
        QCOMPARE(scheme.mode, Scheme::Mode::Dark);
        QCOMPARE(allColours(scheme.colours),
            allColours(gen::genScheme(k_sourceColour, Variant::TonalSpot, Scheme::Mode::Dark)));
    }

    void schemeFromPathMatchesImage() {
        const auto fromPath = schemeForPath(m_path, Variant::Monochrome, Scheme::Mode::Light).result();
        const auto fromImage = schemeForImage(m_image, Variant::Monochrome, Scheme::Mode::Light);
        QCOMPARE(fromPath.name, fromImage.name);
        QCOMPARE(fromPath.flavour, fromImage.flavour);
        QCOMPARE(fromPath.mode, fromImage.mode);
        QCOMPARE(allColours(fromPath.colours), allColours(fromImage.colours));
    }

    void schemeFromPathUsesCache() {
        constexpr auto variant = Variant::FruitSalad;
        constexpr auto mode = Scheme::Mode::Dark;
        const auto seeded = gen::genScheme(Argb(0xFFFF0000), Variant::Vibrant, Scheme::Mode::Light);
        const auto key = SchemeCache::keyFor(k_sourceColour, variant, mode);
        SchemeCache::instance().store(key, toBytes(seeded));
        QCOMPARE(SchemeCache::instance().lookup(key).result(), toBytes(seeded));

        const auto scheme = schemeForPath(m_path, variant, mode).result();
        QCOMPARE(allColours(scheme.colours), allColours(seeded));
        QVERIFY(allColours(scheme.colours) != allColours(gen::genScheme(k_sourceColour, variant, mode)));
    }

    void schemeFromPathIgnoresWrongSizedCache() {
        constexpr auto variant = Variant::Rainbow;
        constexpr auto mode = Scheme::Mode::Light;
        const auto key = SchemeCache::keyFor(k_sourceColour, variant, mode);
        const QByteArray bad(static_cast<qsizetype>(sizeof(SchemeColours)) - 1, '\x7f');
        SchemeCache::instance().store(key, bad);
        QCOMPARE(SchemeCache::instance().lookup(key).result(), bad);

        const auto scheme = schemeForPath(m_path, variant, mode).result();
        QCOMPARE(allColours(scheme.colours), allColours(gen::genScheme(k_sourceColour, variant, mode)));
    }

    void schemeFromNullImageUsesFallback() {
        const auto scheme = schemeForImage(QImage(), Variant::TonalSpot, Scheme::Mode::Dark);
        QCOMPARE(allColours(scheme.colours),
            allColours(gen::genScheme(k_fallbackColour, Variant::TonalSpot, Scheme::Mode::Dark)));
    }

    void schemeFromMissingPathUsesFallback() {
        const auto scheme =
            schemeForPath(m_root.filePath(u"missing.png"_s), Variant::TonalSpot, Scheme::Mode::Dark).result();
        QCOMPARE(allColours(scheme.colours),
            allColours(gen::genScheme(k_fallbackColour, Variant::TonalSpot, Scheme::Mode::Dark)));
    }
};

QTEST_GUILESS_MAIN(TestScheme)

#include "scheme_test.moc"
