#pragma once

#include <cstdint>
#include <vector>

#include "progress/leaderboard.h"
#include "progress/stats.h"

namespace bj {

enum DrillKind { kSpeedDrill = 0, kTrueCountDrill = 1, kBetSizingDrill = 2, kNumDrills = 3 };

// How the player is doing in one drill.
struct DrillRecord {
    int bestLevel = 0;  // the highest level passed (0 = none yet)
    int attempts = 0;
    int passes = 0;
};

// Everything that is remembered between runs besides the settings.
struct Profile {
    LifetimeStats lifetime;
    DrillRecord drills[kNumDrills];
    std::uint64_t achievements = 0;  // one bit per achievement, see achievements.h
    std::vector<LeaderboardEntry> leaderboard;
};

}  // namespace bj
