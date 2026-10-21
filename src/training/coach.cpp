#include "training/coach.h"

#include <algorithm>
#include <iostream>

#include "ui/format.h"
#include "ui/style.h"

namespace bj {
namespace coach {
namespace {

void record(CoachStats& stats, bool ok) {
    ++stats.decisions;
    if (ok) {
        ++stats.correct;
        stats.bestStreak = std::max(stats.bestStreak, ++stats.streak);
    } else {
        stats.streak = 0;
    }
}

void say(bool ok, const std::string& text) {
    std::cout << (ok ? style::good(text) : style::bad(text)) << "\n";
}

}  // namespace

void showHint(CoachMode mode, strategy::Play recommended, const std::string& situation) {
    if (mode != CoachMode::Hints) return;
    std::cout << style::accent("Coach: ") << strategy::playName(recommended) << " (" << situation << ").\n";
}

bool grade(CoachMode mode, CoachStats& stats, strategy::Play chosen, strategy::Play recommended,
           const std::string& situation) {
    if (mode == CoachMode::Off) return true;
    bool ok = chosen == recommended;
    record(stats, ok);
    if (ok) {
        say(true, "Coach: correct.");
    } else {
        say(false, std::string("Coach: basic strategy says ") + strategy::playName(recommended) + " (" + situation +
                       "), not " + strategy::playName(chosen) + ".");
    }
    return ok;
}

void showInsuranceHint(CoachMode mode, bool shouldInsure, double trueCount) {
    if (mode != CoachMode::Hints) return;
    std::cout << style::accent("Coach: ") << (shouldInsure ? "take insurance" : "decline insurance") << " (true count "
              << tenths(trueCount, true) << (shouldInsure ? ", which is +3 or more" : "; insure only at +3 or more")
              << ").\n";
}

bool gradeInsurance(CoachMode mode, CoachStats& stats, bool tookInsurance, bool shouldInsure, double trueCount) {
    if (mode == CoachMode::Off) return true;
    bool ok = tookInsurance == shouldInsure;
    record(stats, ok);
    if (ok) {
        say(true, "Coach: correct.");
    } else {
        say(false, std::string("Coach: basic strategy says ") + (shouldInsure ? "take" : "decline") +
                       " insurance (true count " + tenths(trueCount, true) + "), not " +
                       (tookInsurance ? "take" : "decline") + " it.");
    }
    return ok;
}

double strategyPenalty(const CoachStats& lifetime, const CoachStats& session) {
    int decisions = lifetime.decisions + session.decisions;
    int rounds = lifetime.rounds + session.rounds;
    if (decisions < kMinDecisionsForPenalty || rounds <= 0) return 0.0;
    int mistakes = decisions - (lifetime.correct + session.correct);
    return mistakes * kMistakeCost / rounds;
}

}  // namespace coach
}  // namespace bj
