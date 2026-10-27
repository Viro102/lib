#pragma once

#include <qhash.h>
#include <qstring.h>

namespace caelestia::utils {

struct ComponentInfo {
    QString version;
    QString distributor;
    QString gitRevision;

    bool operator==(const ComponentInfo&) const = default;
};

[[nodiscard]] QHash<QString, ComponentInfo> installedComponents();

} // namespace caelestia::utils
