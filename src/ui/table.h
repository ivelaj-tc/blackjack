#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "core/card.h"

namespace bj {

struct Settings;

// A single card as one line of text: "[10♥]", or a white card face when color is on.
std::string compactCard(const Card& card, bool ascii);

// One of the player's hands in a round. A round starts with one and can grow through splits.
struct PlayerHand {
    Hand hand;
    int bet = 0;
    bool locked = false;  // split aces receive one card each and must stand
};

// Draws the dealer and the player's hands, either as compact one-line hands ("[A♠] [K♥] (21)") or as multi-line card
// art, depending on the settings.
//
// With the dealing animation on, frames can be drawn over the previous one in place using cursor movement, so cards
// seem to land one at a time. That only works while nothing else has been printed since the last frame, so callers
// use `replace` only for back-to-back frames. Without the animation nothing special is printed and every call simply
// prints a new table.
class TableDisplay {
public:
    explicit TableDisplay(const Settings& settings);

    // Forget the previous frame, so the next one is printed fresh below whatever came before.
    void beginAnimation();

    // An in-between frame of an animation. Printed only when animating, replacing the previous frame if there is one.
    void preview(const std::vector<PlayerHand>& hands, const Hand& dealer, bool hideHole,
                 const std::string& status = "");

    // Prints the table; `status` goes on the line above it. When animating and `replace` is set, the previous frame is
    // overwritten instead. `active` marks the hand being played when there are several.
    void show(const std::vector<PlayerHand>& hands, const Hand& dealer, bool hideHole, int active = -1,
              const std::string& status = "", bool replace = false);

private:
    std::vector<std::string> render(const std::vector<PlayerHand>& hands, const Hand& dealer, bool hideHole,
                                    int active, const std::string& status) const;
    void draw(const std::vector<std::string>& lines, bool replace);

    const Settings& settings_;
    bool art_;
    bool animating_;
    std::size_t lastHeight_ = 0;  // lines of the last frame, which can be overwritten in place
};

}  // namespace bj
