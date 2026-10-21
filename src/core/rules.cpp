#include "core/rules.h"

namespace bj {

std::string blackjackPayout(const Rules& rules) { return rules.blackjackSixToFive ? "6:5" : "3:2"; }

int blackjackWin(const Rules& rules, int bet) { return rules.blackjackSixToFive ? bet * 6 / 5 : bet * 3 / 2; }

double houseEdge(const Rules& rules, int decks) {
    // Standard rules: dealer stands on 17, double after split, 3:2 blackjack. The edge grows with the deck count.
    double edge = 0.0045 - 0.003 / decks;

    // Commonly quoted effects of each variation on the house edge.
    if (rules.dealerHitsSoft17) edge += 0.0022;
    if (rules.blackjackSixToFive) edge += 0.0139;
    if (rules.lateSurrender) edge -= 0.0007;
    if (rules.resplitAces) edge -= 0.0008;
    return edge;
}

std::string describeRules(const Rules& rules) {
    std::string text = std::string("Dealer ") + (rules.dealerHitsSoft17 ? "hits soft 17" : "stands on 17") +
                       ". Blackjack pays " + blackjackPayout(rules) + ". Double on any two cards, split up to " +
                       std::to_string(kMaxHands) + " hands.";
    if (rules.lateSurrender) text += " Late surrender is allowed.";
    if (rules.resplitAces) text += " Split aces can be split again.";
    return text;
}

}  // namespace bj
