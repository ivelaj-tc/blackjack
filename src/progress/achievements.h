#pragma once

#include <vector>

#include "progress/profile.h"

namespace bj {

enum AchievementId {
    kNatural,
    kHotStreak,
    kDeckMaster,
    kQuickHands,
    kMathWhiz,
    kRightSize,
    kByTheBook,
    kSharp,
    kMarathon,
    kComeback,
    kHighRoller,
    kChallenger,
    kTopOfTheBoard,
    kVeteran,
    kNumAchievements
};

struct Achievement {
    const char* id;           // stable name used in the save file
    const char* name;
    const char* description;
};

const Achievement& achievement(int index);

bool isUnlocked(const Profile& profile, int index);

// What just happened. The lifetime totals and drill records in the profile are expected to be up to date already;
// the fields here carry the things only the moment itself knows. Leave out what doesn't apply.
struct AchievementEvent {
    const SessionStats* session = nullptr;  // a session that just ended
    bool challengeFinished = false;
    int challengeRank = 0;                  // rank on the seed's leaderboard (1 = first)
    std::size_t challengeEntriesBefore = 0; // entries for that seed before this one was added
};

// Unlocks every achievement the profile and event now satisfy, and returns the ones newly unlocked.
std::vector<int> evaluateAchievements(Profile& profile, const AchievementEvent& event);

// "Achievement unlocked: Hot streak - Give 10 correct running counts in a row."
void announceAchievements(const std::vector<int>& newlyUnlocked, bool ascii);

void printAchievements(const Profile& profile);

}  // namespace bj
