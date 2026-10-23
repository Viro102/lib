#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include <stdexcept>

#include "caelestia/config/config.hpp"
#include "caelestia/scheme/scheme.hpp"
#include "caelestia/wallpaper/wallpaper.hpp"
#include "common.hpp"

using namespace caelestia;
using Qt::StringLiterals::operator""_s;

namespace {

constexpr auto k_fixture = R"({"theme":{"targets":{
    "terminal":false,"hypr":false,"discord":false,"spicetify":false,"pandora":false,"fuzzel":false,
    "btop":false,"nvtop":false,"htop":false,"gtk":false,"qt":false,"warp":false,"chromium":false,
    "zed":false,"cava":false
}}})";

QString saveImage(const QString& path, QRgb colour) {
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(colour);
    if (!image.save(path))
        return {};
    return path;
}

} // namespace

class TestWallpaper : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
    QString m_blue;
    QString m_grey;

    [[nodiscard]] scheme::SchemeData loaded() const {
        const auto state = scheme::loadState();
        if (!state)
            throw std::runtime_error("no scheme state");
        return *state;
    }

private slots:

    void initTestCase() {
        QVERIFY(m_root.isValid());
        qputenv("XDG_CACHE_HOME", m_root.filePath(u"cache"_s).toUtf8());
        qputenv("XDG_CONFIG_HOME", m_root.filePath(u"config"_s).toUtf8());
        qputenv("XDG_DATA_HOME", m_root.filePath(u"data"_s).toUtf8());
        qputenv("XDG_STATE_HOME", m_root.filePath(u"state"_s).toUtf8());
        qunsetenv("HYPRLAND_INSTANCE_SIGNATURE");

        writeFile(config::Config::configPath(), k_fixture);
        config::Config::instance().reload();

        m_blue = saveImage(m_root.filePath(u"blue.png"_s), 0xFF2962FF);
        m_grey = saveImage(m_root.filePath(u"grey.png"_s), 0xFFEEEEEE);
        QVERIFY(!m_blue.isEmpty());
        QVERIFY(!m_grey.isEmpty());
    }

    void init() {
        QFile::remove(wallpaper::statePath());
        QFile::remove(scheme::statePath());
    }

    void currentFallsBackWhenUnset() {
        const auto current = wallpaper::currentWallpaper();
        QVERIFY(current.endsWith(u"/wallpaper.webp"_s));
        QVERIFY(QFileInfo(current).isAbsolute());
    }

    void currentFallsBackWhenFileIsGone() {
        writeFile(wallpaper::statePath(), m_root.filePath(u"gone.png"_s).toUtf8());
        QVERIFY(wallpaper::currentWallpaper().endsWith(u"/wallpaper.webp"_s));
    }

    void currentReadsState() {
        writeFile(wallpaper::statePath(), (m_blue + u"\n"_s).toUtf8());
        QCOMPARE(wallpaper::currentWallpaper(), m_blue);
    }

    void setRejectsUnsupportedFiles() {
        const auto text = m_root.filePath(u"notes.txt"_s);
        writeFile(text, "hello");

        auto future = wallpaper::set(text);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, future.waitForFinished());
        QVERIFY(!QFileInfo::exists(wallpaper::statePath()));
    }

    void setWritesStateAndDefaultScheme() {
        wallpaper::set(m_blue).waitForFinished();

        QCOMPARE(readFile(wallpaper::statePath()), m_blue);
        QCOMPARE(wallpaper::currentWallpaper(), m_blue);
        const auto data = loaded();
        QCOMPARE(data.name, u"caelestia"_s);
        QCOMPARE(data.flavour, u"default"_s);
    }

    void setKeepsFileScheme() {
        const auto latte = scheme::Scheme::forName(u"catppuccin"_s).result()->flavour(u"latte"_s);
        if (!latte) {
            QFAIL("catppuccin has no latte flavour");
        }
        QVERIFY(scheme::saveState(latte->data(scheme::Mode::Light).result()));

        wallpaper::set(m_blue).waitForFinished();

        const auto data = loaded();
        QCOMPARE(data.name, u"catppuccin"_s);
        QCOMPARE(data.flavour, u"latte"_s);
        QCOMPARE(data.mode, scheme::Mode::Light);
    }

    void setFallsBackToDefaultFlavourAndMode() {
        const auto seed = scheme::defaultScheme().result();
        QVERIFY(scheme::saveState(
            { .name = u"catppuccin"_s, .flavour = u"gone"_s, .mode = scheme::Mode::Light, .colours = seed.colours }));

        wallpaper::set(m_blue).waitForFinished();

        auto data = loaded();
        QCOMPARE(data.name, u"catppuccin"_s);
        QCOMPARE(data.flavour, u"frappe"_s);
        QCOMPARE(data.mode, scheme::Mode::Dark);

        QVERIFY(scheme::saveState(
            { .name = u"gone"_s, .flavour = u"gone"_s, .mode = scheme::Mode::Dark, .colours = seed.colours }));
        wallpaper::set(m_blue).waitForFinished();
        data = loaded();
        QCOMPARE(data.name, seed.name);
        QCOMPARE(data.flavour, seed.flavour);
    }

    void setFallsBackToSmartVariantForDynamicScheme() {
        const auto seed = scheme::defaultScheme().result();
        QVERIFY(scheme::saveState(
            { .name = u"dynamic"_s, .flavour = u"gone"_s, .mode = scheme::Mode::Light, .colours = seed.colours }));

        wallpaper::set(m_blue, false).waitForFinished();

        const auto data = loaded();
        QCOMPARE(data.flavour, u"tonalspot"_s);
        QCOMPARE(data.mode, scheme::Mode::Light);
    }

    void setPicksSmartOptionsForDynamicScheme() {
        const auto seed = scheme::defaultScheme().result();
        QVERIFY(scheme::saveState(
            { .name = u"dynamic"_s, .flavour = u"vibrant"_s, .mode = scheme::Mode::Light, .colours = seed.colours }));

        wallpaper::set(m_blue).waitForFinished();
        auto data = loaded();
        QCOMPARE(data.name, u"dynamic"_s);
        QCOMPARE(data.flavour, u"tonalspot"_s);
        QCOMPARE(data.mode, scheme::Mode::Dark);

        wallpaper::set(m_grey).waitForFinished();
        data = loaded();
        QCOMPARE(data.flavour, u"neutral"_s);
        QCOMPARE(data.mode, scheme::Mode::Light);
    }

    void setKeepsDynamicOptionsWithoutSmart() {
        const auto seed = scheme::defaultScheme().result();
        QVERIFY(scheme::saveState(
            { .name = u"dynamic"_s, .flavour = u"vibrant"_s, .mode = scheme::Mode::Light, .colours = seed.colours }));

        wallpaper::set(m_blue, false).waitForFinished();

        const auto data = loaded();
        QCOMPARE(data.name, u"dynamic"_s);
        QCOMPARE(data.flavour, u"vibrant"_s);
        QCOMPARE(data.mode, scheme::Mode::Light);
        QVERIFY(data.colours.primary != seed.colours.primary);
    }
};

QTEST_GUILESS_MAIN(TestWallpaper)

#include "wallpaper_test.moc"
