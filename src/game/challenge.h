#pragma once

#include <cstdint>

namespace bj {

struct Settings;
struct Profile;

namespace challenge {

const int kRounds = 25;

// Today's date as YYYYMMDD. The BLACKJACK_TODAY environment variable (also YYYYMMDD) overrides it, which is handy for
// testing and for replaying an old day's shoe.
int todayDate();

// The shuffle everyone plays in the challenge: the --seed option when given, otherwise today's date.
std::uint32_t currentSeed();

// Plays the challenge: kRounds rounds on a fixed shoe with standard rules, a 1,000-chip stack, a 10-chip minimum and no
// coach or Kelly advice. Finishing (or going broke) scores the chips left onto the leaderboard; leaving early does not.
void run(const Settings& settings, Profile& profile);

}  // namespace challenge
}  // namespace bj
