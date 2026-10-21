#pragma once

namespace bj {

class Shoe;
struct Settings;

// How well the player has tracked the running count this session.
struct CountStats {
    int asked = 0;
    int correct = 0;
    int streak = 0;
    int bestStreak = 0;
};

// "Running +3  |  decks left 4.5  |  true count +0.7"
void printCountLine(const Shoe& shoe);

// Asks for the running count before each bet, then shows the Kelly advice for it.
// Returns false if the player quit.
// `strategyPenalty` is passed on to the Kelly advice (see kelly.h).
bool quizCount(const Shoe& shoe, CountStats& stats, const Settings& settings, int chips, double strategyPenalty = 0.0);

void printCountStats(const CountStats& stats);

}  // namespace bj
