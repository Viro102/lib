#pragma once

#include <qcolor.h>
#include <qfuture.h>
#include <qjsonobject.h>
#include <qlist.h>
#include <qstring.h>

#include <memory>
#include <optional>

namespace caelestia::scheme {

struct SchemeColours {
    // Material 3, 2025 spec
    QColor primaryPaletteKeyColour;
    QColor secondaryPaletteKeyColour;
    QColor tertiaryPaletteKeyColour;
    QColor neutralPaletteKeyColour;
    QColor neutralVariantPaletteKeyColour;
    QColor errorPaletteKeyColour;
    QColor background;
    QColor onBackground;
    QColor surface;
    QColor surfaceDim;
    QColor surfaceBright;
    QColor surfaceContainerLowest;
    QColor surfaceContainerLow;
    QColor surfaceContainer;
    QColor surfaceContainerHigh;
    QColor surfaceContainerHighest;
    QColor onSurface;
    QColor onSurfaceVariant;
    QColor outline;
    QColor outlineVariant;
    QColor inverseSurface;
    QColor inverseOnSurface;
    QColor primary;
    QColor onPrimary;
    QColor primaryContainer;
    QColor onPrimaryContainer;
    QColor primaryFixed;
    QColor primaryFixedDim;
    QColor onPrimaryFixed;
    QColor onPrimaryFixedVariant;
    QColor inversePrimary;
    QColor secondary;
    QColor onSecondary;
    QColor secondaryContainer;
    QColor onSecondaryContainer;
    QColor secondaryFixed;
    QColor secondaryFixedDim;
    QColor onSecondaryFixed;
    QColor onSecondaryFixedVariant;
    QColor tertiary;
    QColor onTertiary;
    QColor tertiaryContainer;
    QColor onTertiaryContainer;
    QColor tertiaryFixed;
    QColor tertiaryFixedDim;
    QColor onTertiaryFixed;
    QColor onTertiaryFixedVariant;
    QColor error;
    QColor onError;
    QColor errorContainer;
    QColor onErrorContainer;

    // Extended material
    QColor success;
    QColor onSuccess;
    QColor successContainer;
    QColor onSuccessContainer;

    // Terminal colours
    QColor term0;
    QColor term1;
    QColor term2;
    QColor term3;
    QColor term4;
    QColor term5;
    QColor term6;
    QColor term7;
    QColor term8;
    QColor term9;
    QColor term10;
    QColor term11;
    QColor term12;
    QColor term13;
    QColor term14;
    QColor term15;

    // Named colours
    QColor rosewater;
    QColor flamingo;
    QColor pink;
    QColor mauve;
    QColor red;
    QColor maroon;
    QColor peach;
    QColor yellow;
    QColor green;
    QColor teal;
    QColor sky;
    QColor sapphire;
    QColor blue;
    QColor lavender;

    // KDE colours (for Qt theming)
    QColor klink;
    QColor klinkSelection;
    QColor kvisited;
    QColor kvisitedSelection;
    QColor knegative;
    QColor knegativeSelection;
    QColor kneutral;
    QColor kneutralSelection;
    QColor kpositive;
    QColor kpositiveSelection;
};

enum class Mode : quint8 {
    Light,
    Dark,
};

struct SchemeData {
    QString name;
    QString flavour;
    Mode mode;
    SchemeColours colours;
};

struct NamedColour {
    QStringView name;
    QColor colour;
};

QList<NamedColour> namedColours(const SchemeColours& colours);
QString modeName(Mode mode);

[[nodiscard]] QJsonObject toJson(const SchemeData& data);
[[nodiscard]] std::optional<SchemeData> fromJson(const QJsonObject& json);

QFuture<SchemeData> defaultScheme();
bool saveState(const SchemeData& data);
[[nodiscard]] std::optional<SchemeData> loadState();

class Scheme;

class Flavour {
public:
    [[nodiscard]] const QString& name() const;
    [[nodiscard]] QList<Mode> modes() const;
    [[nodiscard]] Mode defaultMode() const;
    [[nodiscard]] QFuture<SchemeData> data(Mode mode) const;

private:
    friend class Scheme;

    Flavour(std::shared_ptr<const Scheme> scheme, QString name);

    std::shared_ptr<const Scheme> m_scheme;
    QString m_name;
};

class Scheme : public std::enable_shared_from_this<Scheme> {
public:
    Q_DISABLE_COPY_MOVE(Scheme)

    virtual ~Scheme() = default;

    [[nodiscard]] static QFuture<QList<std::shared_ptr<Scheme>>> all();
    [[nodiscard]] static QFuture<std::shared_ptr<Scheme>> forName(const QString& name);
    [[nodiscard]] static std::shared_ptr<Scheme> forColour(QColor colour);
    [[nodiscard]] static QFuture<std::shared_ptr<Scheme>> forMedia(const QString& path);

    [[nodiscard]] const QString& name() const;
    [[nodiscard]] QList<Flavour> flavours() const;
    [[nodiscard]] Flavour defaultFlavour() const;
    [[nodiscard]] std::optional<Flavour> flavour(const QString& name) const;

protected:
    Scheme(QString name, QStringList flavours);

private:
    friend class Flavour;

    [[nodiscard]] virtual QList<Mode> modes(const QString& flavour) const = 0;
    [[nodiscard]] virtual QFuture<SchemeData> load(const QString& flavour, Mode mode) const = 0;

    const QString m_name;
    const QStringList m_flavours;
};

} // namespace caelestia::scheme
