#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bj {

// One finished challenge. Everyone playing the same seed gets the same shuffle, so scores for a seed are comparable.
struct LeaderboardEntry {
    std::uint32_t seed = 0;
    std::string initials = "YOU";  // up to three letters or digits
    int score = 0;                 // chips at the end of the challenge
    int rounds = 0;
    int date = 0;                  // YYYYMMDD
};

const std::size_t kMaxLeaderboardEntries = 50;

// Upper-cases and keeps up to three letters or digits; anything else gives "YOU".
std::string sanitizeInitials(const std::string& text);

// Adds an entry, keeping the list sorted by score (an earlier entry stays above a later one with the same score) and at
// most kMaxLeaderboardEntries long. Returns its 1-based rank among the entries for the same seed, or 0 if it didn't
// make the list.
int addEntry(std::vector<LeaderboardEntry>& entries, const LeaderboardEntry& entry);

// How many entries are on the list for this seed.
std::size_t countForSeed(const std::vector<LeaderboardEntry>& entries, std::uint32_t seed);

// "2026-10-18" from 20261018.
std::string dateText(int yyyymmdd);

// The best scores for `seed`, then the best of all time. `highlightRank` marks that row of the seed's list.
void printLeaderboard(const std::vector<LeaderboardEntry>& entries, std::uint32_t seed, int highlightRank = 0);

}  // namespace bj
