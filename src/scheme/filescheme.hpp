#pragma once

#include <qfuture.h>
#include <qhash.h>
#include <qlist.h>
#include <qstring.h>
#include <qstringlist.h>

#include <memory>

#include "caelestia/scheme/scheme.hpp"

namespace caelestia::scheme {

class FileScheme : public Scheme {
public:
    [[nodiscard]] static std::shared_ptr<Scheme> open(const QString& root, const QString& name);

    FileScheme(QString dir, QString name, QStringList flavours, QHash<QString, QList<Mode>> modes);

private:
    const QString m_dir;
    const QHash<QString, QList<Mode>> m_modes;

    [[nodiscard]] QList<Mode> modes(const QString& flavour) const override;
    [[nodiscard]] QFuture<SchemeData> load(const QString& flavour, Mode mode) const override;
};

} // namespace caelestia::scheme
