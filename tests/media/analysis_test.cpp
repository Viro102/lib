#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include "caelestia/media/analysis.hpp"

using namespace caelestia::media;
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

bool sameAnalysis(const Analysis& a, const Analysis& b) {
    return a.colours == b.colours && a.dominant == b.dominant &&
           qFuzzyCompare(a.colourfulness + 1, b.colourfulness + 1) && qFuzzyCompare(a.luminance + 1, b.luminance + 1) &&
           qFuzzyCompare(a.tone + 1, b.tone + 1);
}

QImage solid(QRgb colour) {
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(colour);
    return image;
}

} // namespace

class TestAnalysis : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    QImage m_image;
    QString m_path;

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CACHE_HOME", m_root.path().toUtf8());

        m_image = solid(k_sourceColour.value());
        m_path = m_root.filePath(u"wallpaper.png"_s);
        QVERIFY(m_image.save(m_path));
    }

    void coloursFromImage() {
        const auto analysis = analyseImage(m_image);
        QVERIFY(!analysis.colours.isEmpty());
        QCOMPARE(analysis.colours.first(), k_sourceColour);
    }

    void dominantPrefersMostCommon() {
        auto image = solid(k_sourceColour.value());
        for (int x = 0; x < 2; ++x)
            image.setPixel(x, 0, k_firstFrameColour.value());
        QCOMPARE(analyseImage(image).dominant, k_sourceColour);
    }

    void colourfulnessIsZeroForGrey() { QVERIFY(qFuzzyIsNull(analyseImage(solid(0xFF808080)).colourfulness)); }

    void colourfulnessIsPositiveForColour() { QVERIFY(analyseImage(m_image).colourfulness > 10); }

    void luminanceSpansBlackToWhite() {
        QVERIFY(qFuzzyIsNull(analyseImage(solid(0xFF000000)).luminance));
        QVERIFY(qFuzzyCompare(analyseImage(solid(0xFFFFFFFF)).luminance, 1.0));
    }

    void toneSpansBlackToWhite() {
        QVERIFY(analyseImage(solid(0xFF000000)).tone < 1);
        QVERIFY(analyseImage(solid(0xFFFFFFFF)).tone > 99);
    }

    void ignoresTransparentPixels() {
        QImage image(8, 8, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        image.setPixel(0, 0, k_sourceColour.value());
        QCOMPARE(analyseImage(image).dominant, k_sourceColour);
    }

    void nullImageIsEmpty() { QVERIFY(sameAnalysis(analyseImage(QImage()), Analysis{})); }

    void analysisFromPath() { QVERIFY(sameAnalysis(analyseForPath(m_path).result(), analyseImage(m_image))); }

    void analysisFromAnimatedImage() {
        const auto path = QFINDTESTDATA("data/animated.gif");
        QVERIFY(!path.isEmpty());
        const auto analysis = analyseForPath(path).result();
        QVERIFY(!analysis.colours.isEmpty());
        QVERIFY(analysis.colours.first() != k_firstFrameColour);
        QVERIFY(closeTo(analysis.colours.first(), k_sourceColour));
    }

    void analysisFromVideo() {
        const auto path = QFINDTESTDATA("data/video.mp4");
        QVERIFY(!path.isEmpty());
        const auto analysis = analyseForPath(path).result();
        QVERIFY(!analysis.colours.isEmpty());
        QVERIFY(analysis.colours.first() != k_firstFrameColour);
        QVERIFY(closeTo(analysis.colours.first(), k_sourceColour));
    }

    void analysisFromMissingPath() {
        QVERIFY(sameAnalysis(analyseForPath(m_root.filePath(u"missing.png"_s)).result(), Analysis{}));
    }

    void analysisFromPathUsesCache() {
        const auto path = m_root.filePath(u"cached.png"_s);
        QVERIFY(QFile::copy(m_path, path));
        const auto analysis = analyseForPath(path).result();
        QVERIFY(!analysis.colours.isEmpty());

        QFile file(path);
        const auto modified = QFileInfo(file).lastModified();
        const auto size = file.size();
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(QByteArray(size, '\0')), size);
        QVERIFY(file.setFileTime(modified, QFileDevice::FileModificationTime));
        file.close();

        QVERIFY(sameAnalysis(analyseForPath(path).result(), analysis));
    }
};

QTEST_GUILESS_MAIN(TestAnalysis)

#include "analysis_test.moc"
