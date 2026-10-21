#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "core/card.h"

namespace bj {

// A multi-deck shoe that also keeps the Hi-Lo running count of every card it has dealt.
class Shoe {
public:
    explicit Shoe(int decks);                      // shuffled with seed::forShoe()
    Shoe(int decks, std::uint32_t seed);           // shuffled with a specific seed

    Card draw();

    // Reshuffles once roughly 75% of the shoe has been dealt. Returns true if it did.
    bool reshuffleIfLow();

    int runningCount() const { return runningCount_; }
    std::size_t cardsSeen() const;
    double decksRemaining() const;
    double trueCount() const;

private:
    std::size_t totalCards() const;
    void shuffle();

    int decks_;
    int runningCount_ = 0;
    std::vector<Card> cards_;
    std::mt19937 rng_;
};

}  // namespace bj
