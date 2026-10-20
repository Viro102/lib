#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qtemporarydir.h>
#include <qtest.h>

#include <array>
#include <stdexcept>
#include <utility>

#include "caelestia/config/paths.hpp"
#include "caelestia/scheme/generator.hpp"
#include "caelestia/scheme/scheme.hpp"
#include "helpers.hpp"
#include "schemecache.hpp"

using namespace caelestia::scheme;
using m3colours::Argb;
using m3colours::scheme::Variant;
using Qt::StringLiterals::operator""_s;

namespace {

constexpr Argb k_sourceColour{ 0xFF2962FF };

constexpr std::array k_variantNames = {
    std::pair{ Variant::Monochrome, QStringView(u"monochrome") },
    std::pair{ Variant::Neutral, QStringView(u"neutral") },
    std::pair{ Variant::TonalSpot, QStringView(u"tonalspot") },
    std::pair{ Variant::Vibrant, QStringView(u"vibrant") },
    std::pair{ Variant::Expressive, QStringView(u"expressive") },
    std::pair{ Variant::Fidelity, QStringView(u"fidelity") },
    std::pair{ Variant::Content, QStringView(u"content") },
    std::pair{ Variant::Rainbow, QStringView(u"rainbow") },
    std::pair{ Variant::FruitSalad, QStringView(u"fruitsalad") },
    std::pair{ Variant::Cmf, QStringView(u"cmf") },
};

QByteArray toBytes(const SchemeColours& colours) {
    return { reinterpret_cast<const char*>(&colours), sizeof(colours) };
}

QStringList names(const QList<std::shared_ptr<Scheme>>& schemes) {
    QStringList out;
    for (const auto& scheme : schemes)
        out.append(scheme->name());
    return out;
}

QStringList names(const QList<Flavour>& flavours) {
    QStringList out;
    for (const auto& flavour : flavours)
        out.append(flavour.name());
    return out;
}

SchemeData dataFor(const QString& name, const QString& flavour, Mode mode) {
    const auto scheme = Scheme::forName(name).result();
    if (!scheme)
        throw std::invalid_argument("no such scheme");
    const auto f = scheme->flavour(flavour);
    if (!f)
        throw std::invalid_argument("no such flavour");
    return f->data(mode).result();
}

} // namespace

class TestScheme : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_root;
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

        QImage image(8, 8, QImage::Format_RGB32);
        image.fill(k_sourceColour.value());
        m_path = m_root.filePath(u"wallpaper.png"_s);
        QVERIFY(image.save(m_path));
    }

    void bundledSchemeLoads() {
        const auto scheme = Scheme::forName(u"caelestia"_s).result();
        QVERIFY(scheme);
        QCOMPARE(scheme->name(), u"caelestia"_s);
        QCOMPARE(names(scheme->flavours()), QStringList{ u"default"_s });

        const auto flavour = scheme->flavour(u"default"_s);
        QVERIFY(flavour.has_value());
        QCOMPARE(flavour->name(), u"default"_s);
        QCOMPARE(flavour->modes(), (QList<Mode>{ Mode::Light, Mode::Dark }));

        const auto data = flavour->data(Mode::Dark).result();
        QCOMPARE(data.name, u"caelestia"_s);
        QCOMPARE(data.flavour, u"default"_s);
        QCOMPARE(data.mode, Mode::Dark);
        QCOMPARE(data.colours.background, Argb(0xFF0A0F0F));
        QCOMPARE(data.colours.onBackground, Argb(0xFFDCE8E6));
    }

    void bundledSchemeLightMode() {
        const auto data = dataFor(u"caelestia"_s, u"default"_s, Mode::Light);
        QCOMPARE(data.mode, Mode::Light);
        QVERIFY(data.colours.background.red() > 0x80);
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
            const auto mode = info.baseName() == u"light" ? Mode::Light : Mode::Dark;
            const auto tag = info.filePath().toUtf8();

            const auto data = dataFor(name, flavour, mode);
            QCOMPARE(data.name, name);
            QCOMPARE(data.flavour, flavour);
            QCOMPARE(data.mode, mode);
            for (const auto colour : allColours(data.colours))
                QVERIFY2(colour.alpha() == 0xFF, tag.constData());
        }
    }

    void unknownSchemeIsNull() {
        QVERIFY(!Scheme::forName(u"missing"_s).result());
        QVERIFY(!Scheme::forName(QString()).result());
        QVERIFY(!Scheme::forName(u"caelestia/default"_s).result());
    }

    void unknownFlavourIsNullopt() {
        const auto scheme = Scheme::forName(u"caelestia"_s).result();
        QVERIFY(scheme);
        QVERIFY(!scheme->flavour(u"missing"_s).has_value());
    }

    void allListsBundledAndCustom() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"listed"_s, u"custom"_s);
        QVERIFY(QDir().mkpath(caelestia::config::ConfigPaths::customSchemesDir() + u"/empty/custom"_s));

        const auto schemes = Scheme::all().result();
        const auto listed = names(schemes);
        QVERIFY(listed.contains(u"caelestia"_s));
        QVERIFY(listed.contains(u"gruvbox"_s));
        QVERIFY(listed.contains(u"listed"_s));
        QVERIFY(!listed.contains(u"empty"_s));
        QCOMPARE(listed.size(), QSet<QString>(listed.cbegin(), listed.cend()).size());
        for (const auto& scheme : schemes)
            QVERIFY2(!scheme->flavours().isEmpty(), scheme->name().toUtf8().constData());
    }

    void allSnapshotsCatalogue() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"snapshot"_s, u"custom"_s);
        const auto schemes = Scheme::all().result();
        QVERIFY(names(schemes).contains(u"snapshot"_s));
        QVERIFY(QDir(caelestia::config::ConfigPaths::customSchemesDir() + u"/snapshot"_s).removeRecursively());
        QVERIFY(names(schemes).contains(u"snapshot"_s));
        QVERIFY(!names(Scheme::all().result()).contains(u"snapshot"_s));
    }

    void customSchemeLoads() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"mine"_s, u"custom"_s);
        const auto custom = dataFor(u"mine"_s, u"custom"_s, Mode::Dark);
        const auto bundled = dataFor(u"dracula"_s, u"medium"_s, Mode::Dark);
        QCOMPARE(custom.name, u"mine"_s);
        QCOMPARE(custom.flavour, u"custom"_s);
        QCOMPARE(custom.mode, Mode::Dark);
        QCOMPARE(allColours(custom.colours), allColours(bundled.colours));
    }

    void customSchemeMissingModeThrows() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"darkonly"_s, u"custom"_s);
        const auto scheme = Scheme::forName(u"darkonly"_s).result();
        QVERIFY(scheme);
        const auto flavour = scheme->flavour(u"custom"_s);
        QVERIFY(flavour.has_value());
        QCOMPARE(flavour->modes(), QList<Mode>{ Mode::Dark });
        const auto future = flavour->data(Mode::Light);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, future.result());
    }

    void customSchemeOverridesBundled() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"caelestia"_s, u"default"_s);
        const auto data = dataFor(u"caelestia"_s, u"default"_s, Mode::Dark);
        const auto dracula = dataFor(u"dracula"_s, u"medium"_s, Mode::Dark);
        QCOMPARE(allColours(data.colours), allColours(dracula.colours));
        QCOMPARE(dataFor(u"caelestia"_s, u"default"_s, Mode::Light).colours.background, Argb(0xFFF6FAF9));
    }

    void customFlavourMergesIntoBundled() {
        installCustomScheme(u":/schemes/dracula/medium/dark.txt"_s, u"gruvbox"_s, u"extra"_s);
        const auto scheme = Scheme::forName(u"gruvbox"_s).result();
        QVERIFY(scheme);
        QCOMPARE(names(scheme->flavours()), (QStringList{ u"extra"_s, u"hard"_s, u"medium"_s, u"soft"_s }));
        QCOMPARE(scheme->flavour(u"extra"_s)->modes(), QList<Mode>{ Mode::Dark });
        QCOMPARE(scheme->flavour(u"hard"_s)->modes(), (QList<Mode>{ Mode::Light, Mode::Dark }));
    }

    void unreadableCustomFallsBackToBundled() {
        const auto dir = caelestia::config::ConfigPaths::customSchemesDir() + u"/nord/medium"_s;
        QVERIFY(QDir().mkpath(dir + u"/dark.txt"_s));
        const auto data = dataFor(u"nord"_s, u"medium"_s, Mode::Dark);
        QCOMPARE(data.colours.background, Argb(0xFF2E3440));
    }

    void customSchemeParsing() {
        const auto dir = caelestia::config::ConfigPaths::customSchemesDir() + u"/handwritten/custom"_s;
        QVERIFY(QDir().mkpath(dir));
        QVERIFY(QFile::copy(u":/schemes/dracula/medium/dark.txt"_s, dir + u"/dark.txt"_s));
        QFile file(dir + u"/dark.txt"_s);
        QVERIFY(file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        QVERIFY(file.open(QIODevice::Append | QIODevice::Text));
        file.write("\n// secondary 0000ff\n"
                   "background #112233\n"
                   "onBackground 445566\n"
                   "surface\n"
                   "outline 778899 aabbcc\n"
                   "primary #ABCDEF\n");
        file.close();

        const auto data = dataFor(u"handwritten"_s, u"custom"_s, Mode::Dark);
        const auto dracula = dataFor(u"dracula"_s, u"medium"_s, Mode::Dark);
        QCOMPARE(data.colours.background, Argb(0xFF112233));
        QCOMPARE(data.colours.onBackground, Argb(0xFF445566));
        QCOMPARE(data.colours.primary, Argb(0xFFABCDEF));
        QCOMPARE(data.colours.secondary, dracula.colours.secondary);
        QCOMPARE(data.colours.surface, dracula.colours.surface);
        QCOMPARE(data.colours.outline, dracula.colours.outline);
    }

    void incompleteCustomSchemeIsRejected() {
        const auto dir = caelestia::config::ConfigPaths::customSchemesDir() + u"/partial/custom"_s;
        QVERIFY(QDir().mkpath(dir));
        QFile file(dir + u"/dark.txt"_s);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("background 112233\n");
        file.close();

        const auto scheme = Scheme::forName(u"partial"_s).result();
        QVERIFY(scheme);
        const auto future = scheme->flavour(u"custom"_s)->data(Mode::Dark);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, future.result());
    }

    void incompleteCustomFallsBackToBundled() {
        const auto dir = caelestia::config::ConfigPaths::customSchemesDir() + u"/onedark/default"_s;
        QVERIFY(QDir().mkpath(dir));
        QFile file(dir + u"/dark.txt"_s);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("background 112233\n");
        file.close();

        const auto data = dataFor(u"onedark"_s, u"default"_s, Mode::Dark);
        QCOMPARE(data.colours.background, Argb(0xFF121317));
    }

    void namedColoursMatchStruct() {
        const auto colours = generate(k_sourceColour, Variant::TonalSpot, Mode::Dark);
        const auto named = namedColours(colours);
        const auto all = allColours(colours);
        QCOMPARE(named.size(), static_cast<qsizetype>(all.size()));
        for (qsizetype i = 0; i < named.size(); ++i)
            QVERIFY2(named[i].colour == all[static_cast<size_t>(i)], named[i].name.toUtf8().constData());
    }

    void modeNames() {
        QCOMPARE(modeName(Mode::Light), u"light"_s);
        QCOMPARE(modeName(Mode::Dark), u"dark"_s);
    }

    void dynamicSchemeFlavours() {
        const auto scheme = Scheme::forColour(k_sourceColour);
        QVERIFY(scheme);
        QCOMPARE(scheme->name(), u"dynamic"_s);

        const auto flavours = scheme->flavours();
        QCOMPARE(flavours.size(), static_cast<qsizetype>(k_variantNames.size()));
        for (qsizetype i = 0; i < flavours.size(); ++i) {
            QCOMPARE(flavours[i].name(), k_variantNames[static_cast<size_t>(i)].second.toString());
            QCOMPARE(flavours[i].modes(), (QList<Mode>{ Mode::Light, Mode::Dark }));
        }
        QVERIFY(!scheme->flavour(u"bogus"_s).has_value());
    }

    void schemeForColour() {
        const auto data = Scheme::forColour(k_sourceColour)->flavour(u"tonalspot"_s)->data(Mode::Dark).result();
        QCOMPARE(data.name, u"dynamic"_s);
        QCOMPARE(data.flavour, u"tonalspot"_s);
        QCOMPARE(data.mode, Mode::Dark);
        QCOMPARE(allColours(data.colours), allColours(generate(k_sourceColour, Variant::TonalSpot, Mode::Dark)));
    }

    void schemeForColourLightMode() {
        const auto data = Scheme::forColour(k_sourceColour)->flavour(u"vibrant"_s)->data(Mode::Light).result();
        QCOMPARE(data.flavour, u"vibrant"_s);
        QCOMPARE(data.mode, Mode::Light);
        QCOMPARE(allColours(data.colours), allColours(generate(k_sourceColour, Variant::Vibrant, Mode::Light)));
    }

    void schemeForMediaMatchesColour() {
        const auto scheme = Scheme::forMedia(m_path).result();
        QVERIFY(scheme);
        QCOMPARE(scheme->name(), u"dynamic"_s);
        const auto fromMedia = scheme->flavour(u"monochrome"_s)->data(Mode::Light).result();
        const auto fromColour = Scheme::forColour(k_sourceColour)->flavour(u"monochrome"_s)->data(Mode::Light).result();
        QCOMPARE(fromMedia.flavour, fromColour.flavour);
        QCOMPARE(fromMedia.mode, fromColour.mode);
        QCOMPARE(allColours(fromMedia.colours), allColours(fromColour.colours));
    }

    void schemeForMissingMediaIsNull() { QVERIFY(!Scheme::forMedia(m_root.filePath(u"missing.png"_s)).result()); }

    void dynamicSchemeUsesCache() {
        constexpr auto k_variant = Variant::FruitSalad;
        constexpr auto k_mode = Mode::Dark;
        const auto seeded = generate(Argb(0xFFFF0000), Variant::Vibrant, Mode::Light);
        const auto key = SchemeCache::keyFor(k_sourceColour, k_variant, k_mode);
        SchemeCache::instance().store(key, toBytes(seeded));
        QCOMPARE(SchemeCache::instance().lookup(key).result(), toBytes(seeded));

        const auto data = Scheme::forColour(k_sourceColour)->flavour(u"fruitsalad"_s)->data(k_mode).result();
        QCOMPARE(allColours(data.colours), allColours(seeded));
        QVERIFY(allColours(data.colours) != allColours(generate(k_sourceColour, k_variant, k_mode)));
    }

    void dynamicSchemeIgnoresWrongSizedCache() {
        constexpr auto k_variant = Variant::Rainbow;
        constexpr auto k_mode = Mode::Light;
        const auto key = SchemeCache::keyFor(k_sourceColour, k_variant, k_mode);
        const QByteArray bad(static_cast<qsizetype>(sizeof(SchemeColours)) - 1, '\x7f');
        SchemeCache::instance().store(key, bad);
        QCOMPARE(SchemeCache::instance().lookup(key).result(), bad);

        const auto data = Scheme::forColour(k_sourceColour)->flavour(u"rainbow"_s)->data(k_mode).result();
        QCOMPARE(allColours(data.colours), allColours(generate(k_sourceColour, k_variant, k_mode)));
    }
};

QTEST_GUILESS_MAIN(TestScheme)

#include "scheme_test.moc"
