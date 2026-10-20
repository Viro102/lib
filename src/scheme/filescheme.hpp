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
    using Paths = QHash<QString, QStringList>;

    [[nodiscard]] static std::shared_ptr<Scheme> open(const QStringList& roots, const QString& name);
    [[nodiscard]] static QString key(const QString& flavour, Mode mode);

    FileScheme(QString name, QStringList flavours, Paths paths);

private:
    const Paths m_paths;

    [[nodiscard]] QList<Mode> modes(const QString& flavour) const override;
    [[nodiscard]] QFuture<SchemeData> load(const QString& flavour, Mode mode) const override;
};

} // namespace caelestia::scheme
