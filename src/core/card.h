#pragma once

#include <string>
#include <vector>

namespace bj {

struct Card {
    int rank;  // 0 = Ace ... 12 = King
    int suit;  // 0 = spades, 1 = hearts, 2 = diamonds, 3 = clubs

    bool isAce() const { return rank == 0; }
    bool isRed() const { return suit == 1 || suit == 2; }

    int value() const {
        if (isAce()) return 11;  // aces start high; Hand::total() demotes them
        if (rank >= 9) return 10;
        return rank + 1;
    }

    // Hi-Lo counting system: 2-6 are +1, 7-9 are 0, 10/J/Q/K/A are -1.
    int hiLo() const {
        if (rank >= 1 && rank <= 5) return 1;
        if (isAce() || rank >= 9) return -1;
        return 0;
    }

    std::string rankStr() const;             // "A", "2" ... "10", "J", "Q", "K"
    std::string suitStr(bool ascii) const;   // one column wide: a suit symbol, or S/H/D/C
    std::string str(bool ascii) const { return rankStr() + suitStr(ascii); }
};

struct Hand {
    std::vector<Card> cards;

    int total() const;
    bool isSoft() const;  // an ace is currently counted as 11
    bool isBlackjack() const { return cards.size() == 2 && total() == 21; }
    bool isBust() const { return total() > 21; }
    bool canSplit() const { return cards.size() == 2 && cards[0].value() == cards[1].value(); }

private:
    void tally(int& sum, int& highAces) const;  // sum counts demoted aces as 1; highAces still count as 11
};

}  // namespace bj
