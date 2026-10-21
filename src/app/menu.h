#pragma once

namespace bj {

struct Settings;
struct Profile;

// Shows the main menu until the player quits. Settings and the profile (statistics, drill records, achievements,
// leaderboard) are saved whenever they change.
void runMainMenu(Settings& settings, Profile& profile);

}  // namespace bj
