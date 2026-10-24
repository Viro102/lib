#include <qtest.h>

#include "caelestia/media/analysis.hpp"
#include "caelestia/scheme/smart.hpp"

using namespace caelestia;
using namespace caelestia::scheme;
using Qt::StringLiterals::operator""_s;

namespace {

media::Analysis analysisWith(double colourfulness, double tone) {
    media::Analysis analysis;
    analysis.colourfulness = colourfulness;
    analysis.tone = tone;
    return analysis;
}

} // namespace

class SmartTest : public QObject {
    Q_OBJECT

private slots:

    void flavourFollowsColourfulness_data() {
        QTest::addColumn<double>("colourfulness");
        QTest::addColumn<QString>("flavour");

        QTest::newRow("grey") << 0.0 << u"neutral"_s;
        QTest::newRow("below neutral") << 9.9 << u"neutral"_s;
        QTest::newRow("at neutral") << 10.0 << u"content"_s;
        QTest::newRow("below content") << 19.9 << u"content"_s;
        QTest::newRow("at content") << 20.0 << u"tonalspot"_s;
        QTest::newRow("vivid") << 80.0 << u"tonalspot"_s;
    }

    void flavourFollowsColourfulness() {
        QFETCH(const double, colourfulness);
        QFETCH(const QString, flavour);
        QCOMPARE(smartChoice(analysisWith(colourfulness, 0)).flavour, flavour);
    }

    void modeFollowsTone_data() {
        QTest::addColumn<double>("tone");
        QTest::addColumn<Mode>("mode");

        QTest::newRow("black") << 0.0 << Mode::Dark;
        QTest::newRow("at threshold") << 60.0 << Mode::Dark;
        QTest::newRow("above threshold") << 60.1 << Mode::Light;
        QTest::newRow("white") << 100.0 << Mode::Light;
    }

    void modeFollowsTone() {
        QFETCH(const double, tone);
        QFETCH(const Mode, mode);
        QCOMPARE(smartChoice(analysisWith(0, tone)).mode, mode);
    }
};

QTEST_GUILESS_MAIN(SmartTest)
#include "smart_test.moc"
