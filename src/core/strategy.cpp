#include "core/strategy.h"

namespace bj {
namespace strategy {
namespace {

// A cell of the strategy chart. Surrender and splitting are decided before the hard and soft charts.
enum class Cell { Hit, Stand, DoubleOrHit, DoubleOrStand };

int dealerValue(const Card& up) { return up.value(); }  // 2-10, and 11 for an Ace

bool between(int d, int lo, int hi) { return d >= lo && d <= hi; }

// Late surrender (only ever asked on the first two cards of an unsplit hand).
bool surrenders(const Hand& hand, int d, const Rules& rules) {
    if (hand.isSoft()) return false;
    const bool h17 = rules.dealerHitsSoft17;
    int total = hand.total();
    bool pairOfEights = hand.canSplit() && hand.cards[0].value() == 8;
    if (pairOfEights) return h17 && d == 11;  // otherwise 8,8 is split against everything
    if (total == 17) return h17 && d == 11;
    if (total == 16) return d == 9 || d == 10 || d == 11;
    if (total == 15) return d == 10 || (h17 && d == 11);
    return false;
}

// Does basic strategy split this pair? `v` is the card value (11 for aces) and double after split is allowed.
bool pairSplits(int v, int d) {
    switch (v) {
        case 11: return true;
        case 10: return false;
        case 9: return between(d, 2, 6) || d == 8 || d == 9;
        case 8: return true;
        case 7: return between(d, 2, 7);
        case 6: return between(d, 2, 6);
        case 5: return false;  // played as a hard 10
        case 4: return between(d, 5, 6);
        case 3: return between(d, 2, 7);
        case 2: return between(d, 2, 7);
        default: return false;
    }
}

Cell hardCell(int total, int d, int decks, const Rules& rules) {
    if (total <= 8) return Cell::Hit;
    if (total == 9) return between(d, 3, 6) ? Cell::DoubleOrHit : Cell::Hit;
    if (total == 10) return between(d, 2, 9) ? Cell::DoubleOrHit : Cell::Hit;
    if (total == 11) {
        if (d <= 10) return Cell::DoubleOrHit;
        return rules.dealerHitsSoft17 || decks <= 2 ? Cell::DoubleOrHit : Cell::Hit;  // 11 vs Ace
    }
    if (total == 12) return between(d, 4, 6) ? Cell::Stand : Cell::Hit;
    if (total <= 16) return between(d, 2, 6) ? Cell::Stand : Cell::Hit;
    return Cell::Stand;
}

Cell softCell(int total, int d, const Rules& rules) {
    const bool h17 = rules.dealerHitsSoft17;
    if (total <= 12) return Cell::Hit;                                              // A,A when it can't be split
    if (total <= 14) return between(d, 5, 6) ? Cell::DoubleOrHit : Cell::Hit;      // A,2 and A,3
    if (total <= 16) return between(d, 4, 6) ? Cell::DoubleOrHit : Cell::Hit;      // A,4 and A,5
    if (total == 17) return between(d, 3, 6) ? Cell::DoubleOrHit : Cell::Hit;      // A,6
    if (total == 18) {                                                              // A,7
        if (between(d, 3, 6) || (h17 && d == 2)) return Cell::DoubleOrStand;
        return d >= 9 ? Cell::Hit : Cell::Stand;
    }
    if (total == 19) return h17 && d == 6 ? Cell::DoubleOrStand : Cell::Stand;      // A,8
    return Cell::Stand;
}

}  // namespace

Play basicPlay(const Hand& hand, const Card& dealerUp, const Rules& rules, int decks, const Options& options) {
    const int d = dealerValue(dealerUp);
    if (hand.total() >= 21) return Play::Stand;

    if (options.canSurrender && hand.cards.size() == 2 && surrenders(hand, d, rules)) return Play::Surrender;
    if (options.canSplit && hand.canSplit() && pairSplits(hand.cards[0].value(), d)) return Play::Split;

    Cell cell = hand.isSoft() ? softCell(hand.total(), d, rules) : hardCell(hand.total(), d, decks, rules);
    Play play = Play::Stand;
    switch (cell) {
        case Cell::Hit: play = Play::Hit; break;
        case Cell::Stand: play = Play::Stand; break;
        case Cell::DoubleOrHit: play = options.canDouble ? Play::Double : Play::Hit; break;
        case Cell::DoubleOrStand: play = options.canDouble ? Play::Double : Play::Stand; break;
    }
    if (play == Play::Hit && !options.canHit) return Play::Stand;
    return play;
}

std::string describeSituation(const Hand& hand, const Card& dealerUp, bool pairCounts) {
    std::string dealer = dealerUp.isAce() ? "A" : std::to_string(dealerUp.value());
    if (pairCounts && hand.canSplit()) {
        int v = hand.cards[0].value();
        return "pair of " + (v == 11 ? std::string("aces") : std::to_string(v) + "s") + " vs " + dealer;
    }
    return std::string(hand.isSoft() ? "soft " : "hard ") + std::to_string(hand.total()) + " vs " + dealer;
}

const char* playName(Play play) {
    switch (play) {
        case Play::Hit: return "Hit";
        case Play::Stand: return "Stand";
        case Play::Double: return "Double down";
        case Play::Split: return "Split";
        default: return "Surrender";
    }
}

char playKey(Play play) {
    switch (play) {
        case Play::Hit: return 'h';
        case Play::Stand: return 's';
        case Play::Double: return 'd';
        case Play::Split: return 'p';
        default: return 'r';
    }
}

Play playFromKey(char key) {
    switch (key) {
        case 'h': return Play::Hit;
        case 'd': return Play::Double;
        case 'p': return Play::Split;
        case 'r': return Play::Surrender;
        default: return Play::Stand;
    }
}

bool shouldInsure(double trueCount) { return trueCount >= kInsureAtTrueCount; }

}  // namespace strategy
}  // namespace bj
