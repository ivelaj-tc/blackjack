#pragma once

namespace bj {

class Shoe;
struct CoachStats;
struct Settings;

struct RoundResult {
    int net = 0;                 // change in chips, including any insurance (positive = win)
    bool blackjack = false;      // the player was dealt a natural blackjack
    bool paidBlackjack = false;  // ...and the dealer didn't have one, so it was paid
    bool busted = false;         // every hand went over 21, so there was no showdown
    bool surrendered = false;
};

// Plays one round. `chips` is the bankroll before the round and `bet` the opening wager. With the coach on in the
// settings, each decision is compared with basic strategy and counted in `coachStats`.
RoundResult playRound(Shoe& shoe, const Settings& settings, int chips, int bet, CoachStats& coachStats);

}  // namespace bj
