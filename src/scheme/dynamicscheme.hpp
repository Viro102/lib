#pragma once

#include <qfuture.h>
#include <qlist.h>
#include <qstring.h>

#include <m3colours/common.hpp>

#include "caelestia/scheme/scheme.hpp"

namespace caelestia::scheme {

class DynamicScheme : public Scheme {
public:
    explicit DynamicScheme(m3colours::Argb colour);

private:
    const m3colours::Argb m_colour;

    [[nodiscard]] QList<Mode> modes(const QString& flavour) const override;
    [[nodiscard]] QFuture<SchemeData> load(const QString& flavour, Mode mode) const override;
};

} // namespace caelestia::scheme
