#pragma once

#include "caelestia/scheme/generator.hpp"
#include "caelestia/utils/cache.hpp"

namespace caelestia::scheme {

class SchemeCache : public utils::BlobCache {
public:
    static SchemeCache& instance();

    static QString keyFor(QColor sourceColour, DynamicVariant flavour, Mode mode);

private:
    explicit SchemeCache();
};

} // namespace caelestia::scheme
