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
constexpr Argb k_firstFrameColour{ 0xFFD50000 };
constexpr int k_tolerance = 4;

bool closeTo(Argb a, Argb b) {
    return qAbs(qRed(a.value()) - qRed(b.value())) <= k_tolerance &&
           qAbs(qGreen(a.value()) - qGreen(b.value())) <= k_tolerance &&
           qAbs(qBlue(a.value()) - qBlue(b.value())) <= k_tolerance;
}

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

    void coloursFromAnimatedImage() {
        const auto path = QFINDTESTDATA("data/animated.gif");
        QVERIFY(!path.isEmpty());
        const auto colours = coloursForPath(path).result();
        QVERIFY(!colours.isEmpty());
        QVERIFY(colours.first() != k_firstFrameColour);
        QVERIFY(closeTo(colours.first(), k_sourceColour));
    }

    void coloursFromVideo() {
        const auto path = QFINDTESTDATA("data/video.mp4");
        QVERIFY(!path.isEmpty());
        const auto colours = coloursForPath(path).result();
        QVERIFY(!colours.isEmpty());
        QVERIFY(colours.first() != k_firstFrameColour);
        QVERIFY(closeTo(colours.first(), k_sourceColour));
    }

    void coloursFromMissingPath() { QVERIFY(coloursForPath(m_root.filePath(u"missing.png"_s)).result().isEmpty()); }

    void coloursFromPathUsesCache() {
        const auto path = m_root.filePath(u"cached.png"_s);
        QVERIFY(QFile::copy(m_path, path));
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
};

QTEST_GUILESS_MAIN(TestScore)

#include "score_test.moc"
