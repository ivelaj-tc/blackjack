#include "config/settings.h"

#include <algorithm>

#include "platform/terminal.h"
#include "ui/style.h"

namespace bj {
namespace {

bool resolve(Toggle toggle, bool automatic) {
    return toggle == Toggle::On || (toggle == Toggle::Default && automatic);
}

}  // namespace

bool cardArtEnabled(const Settings& settings) { return resolve(settings.cardArt, isInteractive()); }

bool animationEnabled(const Settings& settings) {
    return style::enabled() && resolve(settings.animation, isInteractive());
}

void applyColors(const Settings& settings) {
    bool on = false;
    if (settings.colors == Toggle::On) {
        on = !style::colorForbidden();  // NO_COLOR still wins over a saved "on"
    } else if (settings.colors == Toggle::Default) {
        on = style::supportsColor();
    }
    style::setEnabled(on);
}

void sanitize(Settings& settings) {
    settings.decks = std::max(kMinDecks, std::min(kMaxDecks, settings.decks));
    settings.kellyPercent = std::max(kMinKellyPercent, std::min(kMaxKellyPercent, settings.kellyPercent));
    settings.startingChips = std::max(kMinStartingChips, std::min(kMaxStartingChips, settings.startingChips));
    settings.minBet = std::max(1, std::min(settings.startingChips, settings.minBet));
}

}  // namespace bj
