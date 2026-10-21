#pragma once

#include <string>

namespace bj {

const int kMaxHands = 4;  // a round can be split into at most this many hands

// Table rules the player can change. The defaults are the usual multi-deck rules.
struct Rules {
    bool dealerHitsSoft17 = false;    // otherwise the dealer stands on every 17
    bool blackjackSixToFive = false;  // otherwise blackjack pays 3:2
    bool lateSurrender = false;       // give up half the bet on the first decision
    bool resplitAces = false;         // a split ace that draws another ace may be split again
};

// "3:2" or "6:5"
std::string blackjackPayout(const Rules& rules);

// Chips won by a natural blackjack on `bet`, rounded down.
int blackjackWin(const Rules& rules, int bet);

// Rough house edge against basic strategy as a fraction of the bet (0.004 = 0.4%); negative favors the player.
// Starts from the standard rules for the deck count and applies rule-of-thumb adjustments.
double houseEdge(const Rules& rules, int decks);

// "Dealer stands on 17. Blackjack pays 3:2. Double on any two cards, split up to 4 hands." plus any extras.
std::string describeRules(const Rules& rules);

}  // namespace bj
