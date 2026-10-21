#include "core/shoe.h"

#include <algorithm>

#include "core/seed.h"

namespace bj {
namespace {

const int kCardsPerDeck = 52;
const std::size_t kMinCardsBeforeReshuffle = 20;

}  // namespace

Shoe::Shoe(int decks) : Shoe(decks, seed::forShoe()) {}

Shoe::Shoe(int decks, std::uint32_t seed) : decks_(decks), rng_(seed) { shuffle(); }

Card Shoe::draw() {
    if (cards_.empty()) shuffle();
    Card c = cards_.back();
    cards_.pop_back();
    runningCount_ += c.hiLo();
    return c;
}

bool Shoe::reshuffleIfLow() {
    std::size_t cut = std::max<std::size_t>(totalCards() / 4, kMinCardsBeforeReshuffle);
    if (cards_.size() >= cut) return false;
    shuffle();
    return true;
}

std::size_t Shoe::cardsSeen() const { return totalCards() - cards_.size(); }

double Shoe::decksRemaining() const { return static_cast<double>(cards_.size()) / kCardsPerDeck; }

double Shoe::trueCount() const { return runningCount_ / std::max(decksRemaining(), 0.5); }

std::size_t Shoe::totalCards() const { return static_cast<std::size_t>(decks_) * kCardsPerDeck; }

void Shoe::shuffle() {
    cards_.clear();
    for (int d = 0; d < decks_; ++d)
        for (int s = 0; s < 4; ++s)
            for (int r = 0; r < 13; ++r) cards_.push_back(Card{r, s});
    std::shuffle(cards_.begin(), cards_.end(), rng_);
    runningCount_ = 0;
}

}  // namespace bj
