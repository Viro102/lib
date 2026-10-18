#pragma once

#include "caelestia/utils/cache.hpp"

namespace caelestia::scheme {

class ScoreCache : public utils::BlobCache {
public:
    static ScoreCache& instance();

private:
    explicit ScoreCache();
};

} // namespace caelestia::scheme
