#pragma once

#include <string>

#include "core/card.h"
#include "core/rules.h"

namespace bj {
namespace strategy {

enum class Play { Hit, Stand, Double, Split, Surrender };

// What the player may do at this decision.
struct Options {
    bool canHit = true;        // false for split aces, which take one card and stand
    bool canDouble = true;
    bool canSplit = true;
    bool canSurrender = false;
};

// The play basic strategy recommends for `hand` against the dealer's up card, limited to what is allowed (so
// "double if allowed, otherwise hit" comes back as Double or Hit accordingly). The chart is the usual multi-deck one
// with double after split; with one or two decks only 11 against an Ace differs, and that is handled here.
Play basicPlay(const Hand& hand, const Card& dealerUp, const Rules& rules, int decks, const Options& options);

// "hard 16 vs 10", "soft 18 vs 9" or "pair of 8s vs 6". `pairCounts` says whether splitting is possible, since a
// pair that can't be split is described by its total.
std::string describeSituation(const Hand& hand, const Card& dealerUp, bool pairCounts);

const char* playName(Play play);  // "Hit", "Stand", "Double down", "Split", "Surrender"
char playKey(Play play);          // the key that makes this play at the prompt: h, s, d, p or r
Play playFromKey(char key);

// Hi-Lo insurance index: insurance only pays off in the long run once the true count reaches +3.
const double kInsureAtTrueCount = 3.0;
bool shouldInsure(double trueCount);

}  // namespace strategy
}  // namespace bj
