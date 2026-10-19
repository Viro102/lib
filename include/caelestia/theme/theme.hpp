#pragma once

#include <qfuture.h>
#include <qstring.h>

#include "caelestia/scheme/scheme.hpp"

class TestTheme;

namespace caelestia::theme {

class Theme {
public:
    explicit Theme(scheme::Scheme scheme);

    [[nodiscard]] QFuture<void> apply() const;

private:
    friend class ::TestTheme;

    scheme::Scheme m_scheme;
    QString m_iconTheme;

    [[nodiscard]] QString sequences() const;
    [[nodiscard]] QString render(const QString& templatePath) const;
    [[nodiscard]] QString renderTemplate(const QString& name) const;
    void writeTemplate(const QString& name, const QString& path) const;

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
