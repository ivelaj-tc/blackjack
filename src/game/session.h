#pragma once

#include <cstdint>
#include <string>

#include "progress/stats.h"

namespace bj {

struct Settings;
struct Profile;

struct SessionOptions {
    bool counting = false;       // quiz the running count before every bet
    bool challenge = false;      // no coach or Kelly advice; the leaderboard rules apply
    int maxRounds = 0;           // stop after this many rounds (0 = no limit)
    bool useSeed = false;        // shuffle with `seed` instead of the --seed option or a random one
    std::uint32_t seed = 0;
    std::string note;            // an extra line shown when sitting down
};

enum class SessionEnd { Quit, OutOfChips, RoundLimit };

struct SessionResult {
    SessionStats stats;
    SessionEnd end = SessionEnd::Quit;
    int chips = 0;  // chips when the session ended
};

// Runs one sitting at the table: bet, play rounds, and leave (or go broke). A summary is shown at the end, the session
// is added to the profile's lifetime statistics, and any achievements it earned are announced. If the input ends in the
// middle, the session is wrapped up the same way and InputClosed is thrown afterwards.
SessionResult playSession(const Settings& settings, const SessionOptions& options, Profile& profile);

}  // namespace bj
