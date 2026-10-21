#pragma once

#include "config/toggle.h"
#include "core/rules.h"

namespace bj {

const int kMinDecks = 1;
const int kMaxDecks = 8;
const int kDefaultDecks = 6;

// Kelly stakes are around 1% of the bankroll or less, so a larger default stack keeps them meaningful in whole chips.
const int kMinStartingChips = 10;
const int kMaxStartingChips = 1000000;
const int kDefaultStartingChips = 1000;

const int kDefaultMinBet = 1;  // the smallest opening bet; it can be set from 1 up to the starting chips

const int kMinKellyPercent = 5;
const int kMaxKellyPercent = 100;
const int kDefaultKellyPercent = 50;  // "half Kelly"

// How much the basic-strategy coach says. Off grades nothing; Grade tells you after each play whether it matched
// basic strategy; Hints also names the recommended play before you choose.
enum class CoachMode { Off, Grade, Hints };

// Options the player can change from the settings menu. These are saved between runs.
struct Settings {
    int decks = kDefaultDecks;
    bool ascii = false;
    int kellyPercent = kDefaultKellyPercent;  // share of the full Kelly stake to actually bet
    int startingChips = kDefaultStartingChips;
    int minBet = kDefaultMinBet;  // never more than startingChips
    Rules rules;
    CoachMode coach = CoachMode::Off;

    // Each follows an automatic default until the player picks on or off.
    Toggle colors = Toggle::Default;     // on in a color-capable terminal
    Toggle music = Toggle::Default;      // on in a terminal, when a player and the sound files are found
    Toggle effects = Toggle::Default;    // same
    Toggle cardArt = Toggle::Default;    // multi-line cards in a terminal, compact [A♠] cards otherwise
    Toggle animation = Toggle::Default;  // cards appear one at a time in a terminal
};

// Whether the multi-line card art is in use right now.
bool cardArtEnabled(const Settings& settings);

// Whether the dealing animation is in use right now. It redraws the table in place with cursor movement, so it also
// needs terminal escape codes (colors) to be on.
bool animationEnabled(const Settings& settings);

// Makes the colors setting take effect.
void applyColors(const Settings& settings);

// Pulls every value back into its valid range (for example after loading a hand-edited save file).
void sanitize(Settings& settings);

}  // namespace bj
