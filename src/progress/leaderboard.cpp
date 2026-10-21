#include "progress/leaderboard.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>

#include "ui/style.h"

namespace bj {
namespace {

const std::size_t kRowsShown = 10;

std::string row(int rank, const LeaderboardEntry& e, bool withSeed, bool highlight) {
    char buf[96];
    std::snprintf(buf, sizeof(buf), "  %2d. %-3s %7d  %3d rounds", rank, e.initials.c_str(), e.score, e.rounds);
    std::string text = buf;
    if (withSeed) text += "  seed " + std::to_string(e.seed);
    text += "  " + dateText(e.date);
    return highlight ? style::good(text + "  <-- you") : text;
}

}  // namespace

std::string sanitizeInitials(const std::string& text) {
    std::string out;
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c)) && out.size() < 3) {
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }
    return out.empty() ? "YOU" : out;
}

int addEntry(std::vector<LeaderboardEntry>& entries, const LeaderboardEntry& entry) {
    // Insert after every entry that scores at least as much, so ties keep the older entry on top.
    auto it = std::find_if(entries.begin(), entries.end(),
                           [&](const LeaderboardEntry& other) { return other.score < entry.score; });
    std::size_t index = static_cast<std::size_t>(it - entries.begin());
    entries.insert(it, entry);
    if (entries.size() > kMaxLeaderboardEntries) entries.pop_back();
    if (index >= entries.size()) return 0;  // pushed off the end

    int rank = 1;
    for (std::size_t i = 0; i < index; ++i) {
        if (entries[i].seed == entry.seed) ++rank;
    }
    return rank;
}

std::size_t countForSeed(const std::vector<LeaderboardEntry>& entries, std::uint32_t seed) {
    return static_cast<std::size_t>(
        std::count_if(entries.begin(), entries.end(), [&](const LeaderboardEntry& e) { return e.seed == seed; }));
}

std::string dateText(int yyyymmdd) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", yyyymmdd / 10000, yyyymmdd / 100 % 100, yyyymmdd % 100);
    return buf;
}

void printLeaderboard(const std::vector<LeaderboardEntry>& entries, std::uint32_t seed, int highlightRank) {
    std::cout << "\n" << style::bold("Leaderboard") << "  (shoe " << seed << ")\n";
    int rank = 0;
    bool any = false;
    for (const LeaderboardEntry& e : entries) {
        if (e.seed != seed) continue;
        ++rank;
        if (static_cast<std::size_t>(rank) > kRowsShown) break;
        std::cout << row(rank, e, false, rank == highlightRank) << "\n";
        any = true;
    }
    if (!any) std::cout << "  No scores yet for this shoe.\n";

    std::cout << "\n" << style::bold("Best of all time") << "\n";
    if (entries.empty()) std::cout << "  Nothing yet. Finish a challenge to get on the board.\n";
    for (std::size_t i = 0; i < entries.size() && i < kRowsShown; ++i) {
        std::cout << row(static_cast<int>(i) + 1, entries[i], true, false) << "\n";
    }
}

}  // namespace bj
