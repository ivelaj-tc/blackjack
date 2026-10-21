#include "game/challenge.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#include "config/settings.h"
#include "core/seed.h"
#include "game/session.h"
#include "platform/terminal.h"
#include "progress/achievements.h"
#include "progress/leaderboard.h"
#include "progress/profile.h"
#include "ui/input.h"
#include "ui/style.h"

namespace bj {
namespace challenge {
namespace {

std::string askInitials() {
    if (!isInteractive()) return "YOU";
    try {
        return sanitizeInitials(readLine("Initials for the leaderboard (up to 3 letters, Enter = YOU): "));
    } catch (const InputClosed&) {
        return "YOU";  // the score still counts; the next prompt will end the program
    }
}

}  // namespace

int todayDate() {
    const char* override = std::getenv("BLACKJACK_TODAY");
    if (override != nullptr) {
        int value = 0;
        std::string text = override;
        if (text.size() == 8 && parseInt(text, value) && value >= 10000101 && value <= 99991231) return value;
    }
    std::time_t now = std::time(nullptr);
    std::tm local = *std::localtime(&now);
    return (local.tm_year + 1900) * 10000 + (local.tm_mon + 1) * 100 + local.tm_mday;
}

std::uint32_t currentSeed() {
    return seed::hasFixed() ? seed::fixedValue() : static_cast<std::uint32_t>(todayDate());
}

void run(const Settings& userSettings, Profile& profile) {
    const std::uint32_t shoeSeed = currentSeed();

    // The challenge is the same for everyone: only how the game looks comes from the player's own settings.
    Settings settings = userSettings;
    settings.decks = 6;
    settings.rules = Rules();
    settings.startingChips = 1000;
    settings.minBet = 10;
    settings.coach = CoachMode::Off;

    SessionOptions options;
    options.challenge = true;
    options.maxRounds = kRounds;
    options.useSeed = true;
    options.seed = shoeSeed;
    options.note = "Challenge on shoe " + std::to_string(shoeSeed) + ": " + std::to_string(kRounds) +
                   " rounds, no coach or Kelly advice. Everyone who plays this shoe gets the same cards.";

    SessionResult result = playSession(settings, options, profile);
    if (result.stats.rounds == 0) {
        std::cout << "No rounds played, so there is nothing to score.\n";
        return;
    }
    if (result.end == SessionEnd::Quit) {
        std::cout << "You left before the challenge was over, so this run isn't scored.\n";
        return;
    }

    LeaderboardEntry entry;
    entry.seed = shoeSeed;
    entry.score = result.chips;
    entry.rounds = result.stats.rounds;
    entry.date = todayDate();
    entry.initials = askInitials();

    std::size_t before = countForSeed(profile.leaderboard, shoeSeed);
    int rank = addEntry(profile.leaderboard, entry);

    std::cout << "\n" << style::bold("Challenge score: " + std::to_string(entry.score) + " chips");
    if (rank > 0) {
        std::cout << "  -- #" << rank << " of " << countForSeed(profile.leaderboard, shoeSeed) << " on this shoe";
    } else {
        std::cout << "  -- not high enough for the leaderboard";
    }
    std::cout << "\n";
    printLeaderboard(profile.leaderboard, shoeSeed, rank);

    AchievementEvent event;
    event.challengeFinished = true;
    event.challengeRank = rank;
    event.challengeEntriesBefore = before;
    announceAchievements(evaluateAchievements(profile, event), settings.ascii);
}

}  // namespace challenge
}  // namespace bj
