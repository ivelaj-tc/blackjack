#pragma once

namespace bj {

struct Settings;
struct Profile;

// The training drills menu: speed count (flash cards), true-count conversion and bet sizing, each with five levels.
// Passing a level is recorded in the profile, and may unlock achievements.
void runDrillMenu(const Settings& settings, Profile& profile);

}  // namespace bj
