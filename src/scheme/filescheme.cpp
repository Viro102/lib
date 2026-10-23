#include "filescheme.hpp"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qiodevice.h>
#include <qset.h>
#include <qtextstream.h>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>

#include "colournames.hpp"
#include "logging.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

constexpr auto k_modes = { Mode::Light, Mode::Dark };

QHash<QString, QColor> parseColours(QIODevice& device) {
    QHash<QString, QColor> colours;
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

        colours.insert(parts[0], QColor::fromRgba(0xFF000000u | value.toUInt(nullptr, 16)));
    }
    return colours;
}

std::optional<SchemeColours> readScheme(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcScheme) << "Unable to read scheme" << path << file.errorString();
        return std::nullopt;
    }

    const auto map = parseColours(file);
    bool complete = true;
    const auto colours = [&](QStringView key) -> QColor {
        const auto it = map.constFind(key);
        if (it == map.cend()) {
            qCWarning(lcScheme) << "Scheme" << path << "is missing colour" << key;
            complete = false;
            return {};
        }
        return *it;
    };

#define COLOUR(name) .name = colours(u## #name),
    const SchemeColours result{ CAELESTIA_SCHEME_COLOURS(COLOUR) };
#undef COLOUR

    if (!complete)
        return std::nullopt;
    return result;
}

SchemeData parseScheme(const QStringList& paths, const QString& name, const QString& flavour, Mode mode) {
    for (const auto& path : paths) {
        if (const auto colours = readScheme(path))
            return { .name = name, .flavour = flavour, .mode = mode, .colours = *colours };
    }
    throw std::invalid_argument(u"No valid scheme among: %1"_s.arg(paths.join(u", ")).toStdString());
}

} // namespace

QString FileScheme::key(const QString& flavour, Mode mode) {
    return flavour + u'/' + modeName(mode);
}

std::shared_ptr<Scheme> FileScheme::open(const QStringList& roots, const QString& name) {
    QSet<QString> flavourSet;
    Paths paths;
    for (const auto& root : roots) {
        const auto dir = root + u'/' + name;
        const auto flavours = QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& flavour : flavours) {
            for (const auto mode : k_modes) {
                const auto path = u"%1/%2/%3.txt"_s.arg(dir, flavour, modeName(mode));
                if (!QFileInfo::exists(path))
                    continue;
                flavourSet.insert(flavour);
                paths[key(flavour, mode)].append(path);
            }
        }
    }

    if (flavourSet.isEmpty())
        return nullptr;

    QStringList flavours(flavourSet.cbegin(), flavourSet.cend());
    std::ranges::sort(flavours);
    return std::make_shared<FileScheme>(name, std::move(flavours), std::move(paths));
}

FileScheme::FileScheme(QString name, QStringList flavours, Paths paths)
    : Scheme(std::move(name), std::move(flavours))
    , m_paths(std::move(paths)) {}

QList<Mode> FileScheme::modes(const QString& flavour) const {
    QList<Mode> modes;
    for (const auto mode : k_modes) {
        if (m_paths.contains(key(flavour, mode)))
            modes.append(mode);
    }
    return modes;
}

QFuture<SchemeData> FileScheme::load(const QString& flavour, Mode mode) const {
    return QtFuture::makeReadyValueFuture(m_paths.value(key(flavour, mode)))
        .then(QtFuture::Launch::Async, [schemeName = name(), flavour, mode](const QStringList& paths) {
            return parseScheme(paths, schemeName, flavour, mode);
        });
}

} // namespace caelestia::scheme
