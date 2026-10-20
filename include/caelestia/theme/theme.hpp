#pragma once

#include <qfuture.h>
#include <qstring.h>

#include <optional>

#include "caelestia/scheme/scheme.hpp"

#ifdef CAELESTIA_TESTING
class TestTheme;
#endif

namespace caelestia::theme {

class Theme {
public:
    explicit Theme(scheme::SchemeData scheme);

    [[nodiscard]] QFuture<void> apply() const;

private:
#ifdef CAELESTIA_TESTING
    friend class ::TestTheme;
#endif

    scheme::SchemeData m_scheme;
    QString m_iconTheme;

    [[nodiscard]] QString sequences() const;
    [[nodiscard]] QString render(const QString& templatePath) const;
    [[nodiscard]] QString renderTemplate(const QString& name) const;
    void writeTemplate(const QString& name, const QString& path) const;

    [[nodiscard]] static bool saveState(const scheme::SchemeData& scheme);
    [[nodiscard]] static std::optional<scheme::SchemeData> loadState();

    void applySync() const;
    void applyTargets() const;
    void applyCustom() const;
    void runPostHook(const QString& hook) const;

    void applyTerminal() const;
    void applyHypr() const;
    void applyDiscord() const;
    void applySpicetify() const;
    void applyPandora() const;
    void applyFuzzel() const;
    void applyBtop() const;
    void applyNvtop() const;
    void applyHtop() const;
    void applyGtk() const;
    void applyQt() const;
    void applyWarp() const;
    void applyChromium() const;
    void applyZed() const;
    void applyCava() const;
};

} // namespace caelestia::theme
