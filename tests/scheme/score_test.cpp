#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include "caelestia/scheme/scheme.hpp"

using namespace caelestia::scheme;
using m3colours::Argb;
using Qt::StringLiterals::operator""_s;

namespace {

constexpr Argb k_sourceColour{ 0xFF2962FF };

} // namespace

class TestScore : public QObject {
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

    void coloursFromImage() {
        const auto colours = coloursForImage(m_image);
        QVERIFY(!colours.isEmpty());
        QCOMPARE(colours.first(), k_sourceColour);
    }

    void coloursFromImageConvertsFormat() {
        QImage rgb888(8, 8, QImage::Format_RGB888);
        rgb888.fill(k_sourceColour.value());
        QCOMPARE(coloursForImage(rgb888), coloursForImage(m_image));
    }

    void coloursFromImageRanksDominant() {
        QImage image(8, 8, QImage::Format_RGB32);
        image.fill(k_sourceColour.value());
        for (int x = 0; x < 2; ++x)
            image.setPixel(x, 0, 0xFFD50000);

        const auto colours = coloursForImage(image);
        QVERIFY(colours.size() >= 2);
        QCOMPARE(colours.first(), k_sourceColour);
    }

    void coloursFromPath() {
        const auto colours = coloursForPath(m_path).result();
        QCOMPARE(colours, coloursForImage(m_image));
    }
};

QTEST_GUILESS_MAIN(TestScore)

#include "score_test.moc"
