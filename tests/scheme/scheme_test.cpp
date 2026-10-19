#include <qdiriterator.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include <array>
#include <bit>
#include <stdexcept>

#include "caelestia/scheme/scheme.hpp"
#include "generator.hpp"

using namespace caelestia::scheme;
using m3colours::Argb;
using m3colours::scheme::Variant;
using Qt::StringLiterals::operator""_s;

namespace {

constexpr Argb k_sourceColour{ 0xFF2962FF };

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

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CACHE_HOME", m_root.path().toUtf8());

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
        const auto first = schemeForPath(m_path, Variant::Expressive, Scheme::Mode::Dark).result();
        const auto second = schemeForPath(m_path, Variant::Expressive, Scheme::Mode::Dark).result();
        QCOMPARE(allColours(first.colours), allColours(second.colours));
    }
};

QTEST_GUILESS_MAIN(TestScheme)

#include "scheme_test.moc"
