#include "filescheme.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qiodevice.h>
#include <qtextstream.h>

#include <stdexcept>
#include <utility>

#include "colournames.hpp"
#include "logging.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

QHash<QString, m3colours::Argb> parseColours(QIODevice& device) {
    QHash<QString, m3colours::Argb> colours;
    QTextStream in(&device);
    while (!in.atEnd()) {
        const auto line = in.readLine().trimmed();
        if (line.startsWith(u"//"))
            continue;

        const auto parts = line.split(u' ', Qt::SkipEmptyParts);
        if (parts.size() != 2)
            continue;

        QStringView value = parts[1];
        if (value.startsWith(u'#'))
            value = value.sliced(1);

        colours.insert(parts[0], m3colours::Argb{ 0xFF000000u | value.toUInt(nullptr, 16) });
    }
    return colours;
}

SchemeData parseScheme(const QString& path, const QString& name, const QString& flavour, Mode mode) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::invalid_argument(u"No such scheme: %1-%2-%3"_s.arg(name, flavour, modeName(mode)).toStdString());

    const auto map = parseColours(file);
    const auto colours = [&](QStringView key) {
        const auto it = map.constFind(key);
        if (it == map.cend()) {
            qCWarning(lcScheme) << "Scheme" << path << "is missing colour" << key;
            return m3colours::Argb{ 0 };
        }
        return *it;
    };

#define COLOUR(name) .name = colours(u## #name),

    return {
        .name = name,
        .flavour = flavour,
        .mode = mode,
        .colours = { CAELESTIA_SCHEME_COLOURS(COLOUR) },
    };

#undef COLOUR
}

QList<Mode> modesIn(const QString& dir) {
    QList<Mode> modes;
    for (const auto mode : { Mode::Light, Mode::Dark }) {
        if (QFileInfo::exists(dir + u"/%1.txt"_s.arg(modeName(mode))))
            modes.append(mode);
    }
    return modes;
}

} // namespace

std::shared_ptr<Scheme> FileScheme::open(const QString& root, const QString& name) {
    const auto dir = root + u'/' + name;
    QStringList flavours;
    QHash<QString, QList<Mode>> modes;
    const auto entries = QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& flavour : entries) {
        auto found = modesIn(dir + u'/' + flavour);
        if (found.isEmpty())
            continue;
        flavours.append(flavour);
        modes.insert(flavour, std::move(found));
    }

    if (flavours.isEmpty())
        return nullptr;
    return std::make_shared<FileScheme>(dir, name, std::move(flavours), std::move(modes));
}

FileScheme::FileScheme(QString dir, QString name, QStringList flavours, QHash<QString, QList<Mode>> modes)
    : Scheme(std::move(name), std::move(flavours))
    , m_dir(std::move(dir))
    , m_modes(std::move(modes)) {}

QList<Mode> FileScheme::modes(const QString& flavour) const {
    return m_modes.value(flavour);
}

QFuture<SchemeData> FileScheme::load(const QString& flavour, Mode mode) const {
    const auto path = u"%1/%2/%3.txt"_s.arg(m_dir, flavour, modeName(mode));
    return QtFuture::makeReadyValueFuture(path).then(
        QtFuture::Launch::Async, [schemeName = name(), flavour, mode](const QString& p) {
            return parseScheme(p, schemeName, flavour, mode);
        });
}

} // namespace caelestia::scheme
