#include "caelestia/scheme/smart.hpp"

namespace caelestia::scheme {

using Qt::StringLiterals::operator""_s;

namespace {

constexpr double k_neutralColourfulness = 10;
constexpr double k_contentColourfulness = 20;
constexpr double k_lightTone = 60;

QString smartFlavour(const media::Analysis& analysis) {
    if (analysis.colourfulness < k_neutralColourfulness)
        return u"neutral"_s;
    if (analysis.colourfulness < k_contentColourfulness)
        return u"content"_s;
    return u"tonalspot"_s;
}

Mode smartMode(const media::Analysis& analysis) {
    return analysis.tone > k_lightTone ? Mode::Light : Mode::Dark;
}

} // namespace

SmartChoice smartChoice(const media::Analysis& analysis) {
    return { .flavour = smartFlavour(analysis), .mode = smartMode(analysis) };
}

} // namespace caelestia::scheme
