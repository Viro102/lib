#pragma once

#include "caelestia/utils/cache.hpp"

namespace caelestia::media {

class AnalysisCache : public utils::BlobCache {
public:
    static AnalysisCache& instance();

private:
    explicit AnalysisCache();
};

} // namespace caelestia::media
