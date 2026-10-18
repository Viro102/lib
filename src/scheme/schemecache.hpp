#pragma once

#include "caelestia/scheme/scheme.hpp"
#include "caelestia/utils/cache.hpp"

namespace caelestia::scheme {

class SchemeCache : public utils::BlobCache {
public:
    static SchemeCache& instance();

    static QString keyFor(m3colours::Argb sourceColour, m3colours::scheme::Variant flavour, Scheme::Mode mode);

private:
    explicit SchemeCache();
};

} // namespace caelestia::scheme
