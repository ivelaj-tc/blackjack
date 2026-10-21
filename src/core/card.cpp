#include "core/card.h"

namespace bj {
namespace {

const char* const kSuitsUnicode[] = {"♠", "♥", "♦", "♣"};
const char* const kSuitsAscii[] = {"S", "H", "D", "C"};
const char* const kRanks[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};

}  // namespace

std::string Card::rankStr() const { return kRanks[rank]; }

std::string Card::suitStr(bool ascii) const { return ascii ? kSuitsAscii[suit] : kSuitsUnicode[suit]; }

void Hand::tally(int& sum, int& highAces) const {
    sum = 0;
    highAces = 0;
    for (const Card& c : cards) {
        sum += c.value();
        if (c.isAce()) ++highAces;
    }
    while (sum > 21 && highAces > 0) {
        sum -= 10;
        --highAces;
    }
}

int Hand::total() const {
    int sum = 0;
    int highAces = 0;
    tally(sum, highAces);
    return sum;
}

bool Hand::isSoft() const {
    int sum = 0;
    int highAces = 0;
    tally(sum, highAces);
    return highAces > 0;
}

}  // namespace bj
