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

#include "caelestia/scheme/scheme.hpp"

using namespace caelestia::scheme;
using m3colours::Argb;
using m3colours::scheme::Variant;
using Qt::StringLiterals::operator""_s;

namespace {

constexpr Argb k_sourceColour{ 0xFF2962FF };

auto allColours(const SchemeColours& colours) {
    return std::bit_cast<std::array<Argb, sizeof(SchemeColours) / sizeof(Argb)>>(colours);
}

bool isGrayscale(Argb colour) {
    return colour.red() == colour.green() && colour.green() == colour.blue();
}

} // namespace

class TestScheme : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    QString m_image;

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CACHE_HOME", m_root.path().toUtf8());

        m_image = m_root.filePath(u"wallpaper.png"_s);
        QImage image(8, 8, QImage::Format_RGB32);
        image.fill(k_sourceColour.value());
        QVERIFY(image.save(m_image));
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

    void coloursForImage() {
        const auto colours = coloursForPath(m_image).result();
        QVERIFY(!colours.isEmpty());
        QCOMPARE(colours.first(), k_sourceColour);
    }

    void coloursForMissingImage() { QVERIFY(coloursForPath(m_root.filePath(u"missing.png"_s)).result().isEmpty()); }

    void coloursForUsesCache() {
        const auto path = m_root.filePath(u"cached.png"_s);
        QVERIFY(QFile::copy(m_image, path));
        const auto colours = coloursForPath(path).result();
        QVERIFY(!colours.isEmpty());

        QFile file(path);
        const auto modified = QFileInfo(file).lastModified();
        const auto size = file.size();
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(QByteArray(size, '\0')), size);
        QVERIFY(file.setFileTime(modified, QFileDevice::FileModificationTime));
        file.close();

        QCOMPARE(coloursForPath(path).result(), colours);
    }

    void generatedSchemeFromImage() {
        const auto scheme = schemeForPath(m_image, Variant::TonalSpot, Scheme::Mode::Dark).result();
        QCOMPARE(scheme.name, m_image);
        QCOMPARE(scheme.flavour, u"tonalspot"_s);
        QCOMPARE(scheme.mode, Scheme::Mode::Dark);
        for (const auto colour : allColours(scheme.colours)) {
            QCOMPARE(colour.alpha(), 0xFF);
        }
        QVERIFY(scheme.colours.background.red() < 0x80);
        QVERIFY(scheme.colours.onBackground.red() > 0x80);
    }

    void generatedSchemeLightMode() {
        const auto scheme = schemeForPath(m_image, Variant::TonalSpot, Scheme::Mode::Light).result();
        QCOMPARE(scheme.mode, Scheme::Mode::Light);
        QVERIFY(scheme.colours.background.red() > 0x80);
        QVERIFY(scheme.colours.onBackground.red() < 0x80);
    }

    void generatedSchemeMonochrome() {
        const auto scheme = schemeForPath(m_image, Variant::Monochrome, Scheme::Mode::Dark).result();
        QCOMPARE(scheme.flavour, u"monochrome"_s);
        QVERIFY(isGrayscale(scheme.colours.primary));
        QVERIFY(isGrayscale(scheme.colours.term1));
        QVERIFY(isGrayscale(scheme.colours.red));
    }

    void generatedSchemeIsStable() {
        const auto first = schemeForPath(m_image, Variant::Vibrant, Scheme::Mode::Dark).result();
        const auto second = schemeForPath(m_image, Variant::Vibrant, Scheme::Mode::Dark).result();
        QCOMPARE(allColours(first.colours), allColours(second.colours));
    }
};

QTEST_GUILESS_MAIN(TestScheme)

#include "scheme_test.moc"
