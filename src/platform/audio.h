#pragma once

#include "config/toggle.h"

namespace bj {
namespace audio {

enum class Sound {
    Card,        // a card is dealt or drawn
    Shuffle,     // sitting down, and every reshuffle of the shoe
    Win,         // a round won
    Blackjack,   // a natural blackjack is paid
    Push,        // a round tied
    DealerWins,  // the dealer beat you at the showdown
    TakesChips,  // you surrender: the dealer simply takes half the bet
    ChipsPlace,  // a bet, double, split or insurance wager goes down
    Failure,     // you lost a round (played together with DealerWins or TakesChips, or after a bust)
    PlayerBusts, // one of your hands went over 21
};

// Finds the audio player and the assets folder, and starts the background music if it should be on.
// Toggle::Default means on only when stdin and stdout are terminals; NO_SOUND turns everything off regardless.
// Call once at startup.
void init(Toggle music = Toggle::Default, Toggle effects = Toggle::Default);

// True if a player and at least one sound file were found.
bool available();

bool musicEnabled();
void setMusicEnabled(bool on);  // starts or stops the looping background track

bool effectsEnabled();
void setEffectsEnabled(bool on);

// Plays a sound effect without waiting for it to finish. Does nothing when effects are off or unavailable.
void play(Sound sound);

}  // namespace audio
}  // namespace bj
