#include <qstandardpaths.h>

#include <array>

#include "caelestia/theme/theme.hpp"
#include "util.hpp"

namespace caelestia::theme {

using Qt::StringLiterals::operator""_s;

namespace {

constexpr std::array k_browsers = { u"chromium", u"brave", u"google-chrome-stable" };

} // namespace

void Theme::applyChromium() const {
    QStringList installed;
    for (const auto* const browser : k_browsers) {
        if (!QStandardPaths::findExecutable(QString::fromUtf16(browser)).isEmpty())
            installed << QString::fromUtf16(browser);
    }
    if (installed.isEmpty())
        return;

    const auto helper = QStringLiteral(CAELESTIA_CHROMIUM_HELPER_BIN);
    if (run(u"sudo"_s, { u"-n"_s, helper, hex(m_scheme.colours.surface) }) != 0) {
        qCWarning(lcTheme) << "Failed to write chromium policy via" << helper;
        return;
    }

    for (const auto& browser : installed)
        run(browser, { u"--refresh-platform-policy"_s, u"--no-startup-window"_s });
}

} // namespace caelestia::theme
