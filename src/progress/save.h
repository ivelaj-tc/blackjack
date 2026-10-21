#pragma once

#include <string>

namespace bj {

struct Settings;
struct Profile;

// Where progress is stored, or "" when saving is off. BLACKJACK_SAVE=/path always picks that file. Otherwise saving
// is on only in an interactive terminal, in ~/.blackjack_save, and NO_SAVE turns it off.
std::string savePath();

// Fills in settings and the profile (statistics, drill records, achievements, leaderboard) from the save file. Anything missing, malformed or out of range is
// ignored and keeps its default, so a damaged file can never stop the game from starting.
void loadProgress(Settings& settings, Profile& profile);

// Writes settings and the profile. Failures (read-only disk, no home directory) are silently ignored.
void saveProgress(const Settings& settings, const Profile& profile);

}  // namespace bj
