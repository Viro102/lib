#pragma once

#include <qfuture.h>
#include <qlist.h>
#include <qstring.h>

#include "caelestia/scheme/scheme.hpp"

namespace caelestia::scheme {

class DynamicScheme : public Scheme {
public:
    explicit DynamicScheme(QColor colour);

private:
    const QColor m_colour;

    [[nodiscard]] QList<Mode> modes(const QString& flavour) const override;
    [[nodiscard]] QFuture<SchemeData> load(const QString& flavour, Mode mode) const override;
};

} // namespace caelestia::scheme
