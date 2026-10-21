#include "progress/save.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <map>
#include <sstream>

#include "config/settings.h"
#include "platform/terminal.h"
#include "progress/achievements.h"
#include "progress/profile.h"
#include "progress/stats.h"

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace bj {
namespace {

const char* const kHeader = "blackjack-save 1";

using Values = std::map<std::string, std::string>;

bool envSet(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0';
}

bool parseLongLong(const std::string& text, long long& out) {
    try {
        std::size_t used = 0;
        long long value = std::stoll(text, &used);
        if (used != text.size()) return false;
        out = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// The readers below leave `out` alone when the key is absent, malformed or out of range.
void readInt(const Values& values, const char* key, int lo, int hi, int& out) {
    auto it = values.find(key);
    long long n = 0;
    if (it != values.end() && parseLongLong(it->second, n) && n >= lo && n <= hi) out = static_cast<int>(n);
}

void readCount(const Values& values, const char* key, long long& out) {
    auto it = values.find(key);
    long long n = 0;
    if (it != values.end() && parseLongLong(it->second, n) && n >= 0) out = n;
}

void readBool(const Values& values, const char* key, bool& out) {
    auto it = values.find(key);
    if (it != values.end() && (it->second == "0" || it->second == "1")) out = it->second == "1";
}

const char* toggleName(Toggle toggle) {
    switch (toggle) {
        case Toggle::On: return "on";
        case Toggle::Off: return "off";
        default: return "auto";
    }
}

void readToggle(const Values& values, const char* key, Toggle& out) {
    auto it = values.find(key);
    if (it == values.end()) return;
    if (it->second == "on") out = Toggle::On;
    else if (it->second == "off") out = Toggle::Off;
    else if (it->second == "auto") out = Toggle::Default;
}

// The save file is only ever read or replaced if it is a regular file or doesn't exist yet. Reading a pipe would
// hang the game at startup, and renaming a temporary file over a device or directory would destroy it (think
// BLACKJACK_SAVE=/dev/null as a way to switch saving off).
bool isPlainFileOrMissing(const std::string& path) {
#ifdef _WIN32
    (void)path;
    return true;
#else
    struct stat info;
    return stat(path.c_str(), &info) != 0 || S_ISREG(info.st_mode);
#endif
}

const char* coachName(CoachMode mode) {
    switch (mode) {
        case CoachMode::Grade: return "grade";
        case CoachMode::Hints: return "hints";
        default: return "off";
    }
}

void readCoach(const Values& values, CoachMode& out) {
    auto it = values.find("coach");
    if (it == values.end()) return;
    if (it->second == "off") out = CoachMode::Off;
    else if (it->second == "grade") out = CoachMode::Grade;
    else if (it->second == "hints") out = CoachMode::Hints;
}

const char* const kDrillNames[kNumDrills] = {"speed", "true_count", "bet_sizing"};

// One leaderboard line: seed|initials|score|rounds|date
bool parseEntry(const std::string& text, LeaderboardEntry& out) {
    std::vector<std::string> parts;
    std::stringstream stream(text);
    for (std::string part; std::getline(stream, part, '|');) parts.push_back(part);
    if (parts.size() != 5) return false;
    long long seed = 0, score = 0, rounds = 0, date = 0;
    if (!parseLongLong(parts[0], seed) || !parseLongLong(parts[2], score) || !parseLongLong(parts[3], rounds) ||
        !parseLongLong(parts[4], date)) {
        return false;
    }
    if (seed < 0 || seed > 4294967295LL || score < -1000000000 || score > 1000000000 || rounds < 0 ||
        rounds > 1000000 || date < 0 || date > 99999999) {
        return false;
    }
    out.seed = static_cast<std::uint32_t>(seed);
    out.initials = sanitizeInitials(parts[1]);
    out.score = static_cast<int>(score);
    out.rounds = static_cast<int>(rounds);
    out.date = static_cast<int>(date);
    return true;
}

}  // namespace

std::string savePath() {
    if (envSet("BLACKJACK_SAVE")) return std::getenv("BLACKJACK_SAVE");
    if (envSet("NO_SAVE") || !isInteractive() || !envSet("HOME")) return "";
    return std::string(std::getenv("HOME")) + "/.blackjack_save";
}

void loadProgress(Settings& settings, Profile& profile) {
    LifetimeStats& lifetime = profile.lifetime;
    std::string path = savePath();
    if (path.empty() || !isPlainFileOrMissing(path)) return;

    // Tolerate Windows line endings in a hand-edited file.
    auto readLine = [](std::istream& in, std::string& line) {
        if (!std::getline(in, line)) return false;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return true;
    };

    std::ifstream file(path);
    std::string line;
    if (!file || !readLine(file, line) || line != kHeader) return;

    Values values;
    while (readLine(file, line)) {
        std::size_t eq = line.find('=');
        if (eq != std::string::npos) values[line.substr(0, eq)] = line.substr(eq + 1);
    }

    readInt(values, "decks", kMinDecks, kMaxDecks, settings.decks);
    readBool(values, "ascii", settings.ascii);
    readInt(values, "kelly_percent", kMinKellyPercent, kMaxKellyPercent, settings.kellyPercent);
    readInt(values, "starting_chips", kMinStartingChips, kMaxStartingChips, settings.startingChips);
    readInt(values, "min_bet", 1, kMaxStartingChips, settings.minBet);
    readBool(values, "dealer_hits_soft_17", settings.rules.dealerHitsSoft17);
    readBool(values, "blackjack_6_5", settings.rules.blackjackSixToFive);
    readBool(values, "late_surrender", settings.rules.lateSurrender);
    readBool(values, "resplit_aces", settings.rules.resplitAces);
    readToggle(values, "colors", settings.colors);
    readToggle(values, "music", settings.music);
    readToggle(values, "effects", settings.effects);
    readToggle(values, "card_art", settings.cardArt);
    readToggle(values, "animation", settings.animation);
    readCoach(values, settings.coach);
    sanitize(settings);

    readCount(values, "stat.sessions", lifetime.sessions);
    readCount(values, "stat.rounds", lifetime.rounds);
    readCount(values, "stat.wins", lifetime.wins);
    readCount(values, "stat.losses", lifetime.losses);
    readCount(values, "stat.pushes", lifetime.pushes);
    readCount(values, "stat.blackjacks", lifetime.blackjacks);
    readInt(values, "stat.biggest_win", 0, 1000000000, lifetime.biggestWin);
    readInt(values, "stat.biggest_loss", 0, 1000000000, lifetime.biggestLoss);
    readInt(values, "stat.best_session_net", -1000000000, 1000000000, lifetime.bestSessionNet);
    readInt(values, "stat.peak_bankroll", 0, 1000000000, lifetime.peakBankroll);
    readCount(values, "stat.count_asked", lifetime.countAsked);
    readCount(values, "stat.count_correct", lifetime.countCorrect);
    readInt(values, "stat.best_count_streak", 0, 1000000000, lifetime.bestCountStreak);
    long long coachDecisions = lifetime.coachDecisions, coachCorrect = lifetime.coachCorrect;
    readCount(values, "stat.coach_decisions", coachDecisions);
    readCount(values, "stat.coach_correct", coachCorrect);
    readCount(values, "stat.coach_rounds", lifetime.coachRounds);
    readInt(values, "stat.best_coach_streak", 0, 1000000000, lifetime.bestCoachStreak);
    lifetime.coachDecisions = coachDecisions;
    lifetime.coachCorrect = std::min(coachCorrect, coachDecisions);   // can't have more right than made
    auto net = values.find("stat.total_net");
    long long totalNet = 0;
    if (net != values.end() && parseLongLong(net->second, totalNet)) lifetime.totalNet = totalNet;

    for (int k = 0; k < kNumDrills; ++k) {
        std::string base = std::string("drill.") + kDrillNames[k];
        readInt(values, (base + ".best_level").c_str(), 0, 5, profile.drills[k].bestLevel);
        readInt(values, (base + ".attempts").c_str(), 0, 1000000000, profile.drills[k].attempts);
        readInt(values, (base + ".passes").c_str(), 0, 1000000000, profile.drills[k].passes);
        profile.drills[k].passes = std::min(profile.drills[k].passes, profile.drills[k].attempts);  // can't pass more than tried
    }
    for (int i = 0; i < kNumAchievements; ++i) {
        auto it = values.find(std::string("ach.") + achievement(i).id);
        if (it != values.end() && it->second == "1") profile.achievements |= std::uint64_t{1} << i;
    }
    profile.leaderboard.clear();
    for (std::size_t i = 0; i < kMaxLeaderboardEntries; ++i) {
        auto it = values.find("lb." + std::to_string(i));
        LeaderboardEntry entry;
        if (it != values.end() && parseEntry(it->second, entry)) profile.leaderboard.push_back(entry);
    }
    std::stable_sort(profile.leaderboard.begin(), profile.leaderboard.end(),
                     [](const LeaderboardEntry& a, const LeaderboardEntry& b) { return a.score > b.score; });
}

void saveProgress(const Settings& settings, const Profile& profile) {
    const LifetimeStats& lifetime = profile.lifetime;
    std::string path = savePath();
    if (path.empty() || !isPlainFileOrMissing(path)) return;

    // Write beside the real file and rename, so an interrupted save can't leave a half-written file behind.
    std::string temp = path + ".tmp";
    {
        std::ofstream file(temp, std::ios::trunc);
        if (!file) return;
        const Rules& r = settings.rules;
        file << kHeader << "\n"
             << "decks=" << settings.decks << "\n"
             << "ascii=" << settings.ascii << "\n"
             << "kelly_percent=" << settings.kellyPercent << "\n"
             << "starting_chips=" << settings.startingChips << "\n"
             << "min_bet=" << settings.minBet << "\n"
             << "dealer_hits_soft_17=" << r.dealerHitsSoft17 << "\n"
             << "blackjack_6_5=" << r.blackjackSixToFive << "\n"
             << "late_surrender=" << r.lateSurrender << "\n"
             << "resplit_aces=" << r.resplitAces << "\n"
             << "colors=" << toggleName(settings.colors) << "\n"
             << "music=" << toggleName(settings.music) << "\n"
             << "effects=" << toggleName(settings.effects) << "\n"
             << "card_art=" << toggleName(settings.cardArt) << "\n"
             << "animation=" << toggleName(settings.animation) << "\n"
             << "coach=" << coachName(settings.coach) << "\n"
             << "stat.sessions=" << lifetime.sessions << "\n"
             << "stat.rounds=" << lifetime.rounds << "\n"
             << "stat.wins=" << lifetime.wins << "\n"
             << "stat.losses=" << lifetime.losses << "\n"
             << "stat.pushes=" << lifetime.pushes << "\n"
             << "stat.blackjacks=" << lifetime.blackjacks << "\n"
             << "stat.biggest_win=" << lifetime.biggestWin << "\n"
             << "stat.biggest_loss=" << lifetime.biggestLoss << "\n"
             << "stat.total_net=" << lifetime.totalNet << "\n"
             << "stat.best_session_net=" << lifetime.bestSessionNet << "\n"
             << "stat.peak_bankroll=" << lifetime.peakBankroll << "\n"
             << "stat.count_asked=" << lifetime.countAsked << "\n"
             << "stat.count_correct=" << lifetime.countCorrect << "\n"
             << "stat.best_count_streak=" << lifetime.bestCountStreak << "\n"
             << "stat.coach_decisions=" << lifetime.coachDecisions << "\n"
             << "stat.coach_correct=" << lifetime.coachCorrect << "\n"
             << "stat.coach_rounds=" << lifetime.coachRounds << "\n"
             << "stat.best_coach_streak=" << lifetime.bestCoachStreak << "\n";
        for (int k = 0; k < kNumDrills; ++k) {
            std::string base = std::string("drill.") + kDrillNames[k];
            file << base << ".best_level=" << profile.drills[k].bestLevel << "\n"
                 << base << ".attempts=" << profile.drills[k].attempts << "\n"
                 << base << ".passes=" << profile.drills[k].passes << "\n";
        }
        for (int i = 0; i < kNumAchievements; ++i) {
            if (isUnlocked(profile, i)) file << "ach." << achievement(i).id << "=1\n";
        }
        for (std::size_t i = 0; i < profile.leaderboard.size() && i < kMaxLeaderboardEntries; ++i) {
            const LeaderboardEntry& e = profile.leaderboard[i];
            file << "lb." << i << "=" << e.seed << "|" << e.initials << "|" << e.score << "|" << e.rounds << "|"
                 << e.date << "\n";
        }
        if (!file) return;
    }
    if (std::rename(temp.c_str(), path.c_str()) != 0) std::remove(temp.c_str());
}

}  // namespace bj
