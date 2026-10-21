#pragma once

#include <string>

#include "config/settings.h"
#include "core/strategy.h"

namespace bj {

// How well the player is following basic strategy. Only decisions made with the coach on are counted.
struct CoachStats {
    int decisions = 0;
    int correct = 0;
    int streak = 0;
    int bestStreak = 0;
    int rounds = 0;  // rounds played with the coach on
};

namespace coach {

// Prints "Coach: Double down (hard 11 vs 6)." before the player chooses. Only in Hints mode.
void showHint(CoachMode mode, strategy::Play recommended, const std::string& situation);

// Compares the player's choice with basic strategy, counts it, and tells the player how it went (Grade and Hints modes).
// Returns whether the choice matched.
bool grade(CoachMode mode, CoachStats& stats, strategy::Play chosen, strategy::Play recommended,
           const std::string& situation);

// The same for the insurance decision, which basic strategy answers with the Hi-Lo true count.
void showInsuranceHint(CoachMode mode, bool shouldInsure, double trueCount);
bool gradeInsurance(CoachMode mode, CoachStats& stats, bool tookInsurance, bool shouldInsure, double trueCount);

// The edge lost to strategy mistakes, as a fraction of a bet per round, estimated from the decisions graded so far
// (lifetime plus the current session). It is 0 until there are enough decisions to say anything.
double strategyPenalty(const CoachStats& lifetime, const CoachStats& session);

// Rough average cost of one basic-strategy mistake, as a fraction of the bet.
const double kMistakeCost = 0.07;
const int kMinDecisionsForPenalty = 20;

}  // namespace coach
}  // namespace bj
