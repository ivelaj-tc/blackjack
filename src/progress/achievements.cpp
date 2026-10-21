#include "progress/achievements.h"

#include <iostream>

#include "ui/style.h"

namespace bj {
namespace {

const Achievement kAchievements[kNumAchievements] = {
    {"natural", "Natural", "Be dealt a blackjack."},
    {"hot_streak", "Hot streak", "Give 10 correct running counts in a row."},
    {"deck_master", "Deck master", "Pass the speed count at level 3, a whole deck with one card hidden."},
    {"quick_hands", "Quick hands", "Pass the speed count at level 5."},
    {"math_whiz", "Math whiz", "Pass true-count conversion at level 3."},
    {"right_size", "Right size", "Pass bet sizing at level 3."},
    {"by_the_book", "By the book", "Make 25 basic-strategy plays in a row with the coach on."},
    {"sharp", "Sharp", "Finish a session with at least 50 coached plays and 95% of them correct."},
    {"marathon", "Marathon", "Play 100 rounds in one session."},
    {"comeback", "Comeback", "End a session in profit after falling to half your starting chips."},
    {"high_roller", "High roller", "Win 500 chips or more in one session."},
    {"challenger", "Challenger", "Finish a challenge."},
    {"top_of_the_board", "Top of the board", "Beat an earlier score on a challenge and take first place."},
    {"veteran", "Veteran", "Play 500 rounds in total."},
};

bool satisfied(int index, const Profile& p, const AchievementEvent& e) {
    const LifetimeStats& life = p.lifetime;
    const SessionStats* s = e.session;
    switch (index) {
        case kNatural: return life.blackjacks > 0;
        case kHotStreak: return life.bestCountStreak >= 10;
        case kDeckMaster: return p.drills[kSpeedDrill].bestLevel >= 3;
        case kQuickHands: return p.drills[kSpeedDrill].bestLevel >= 5;
        case kMathWhiz: return p.drills[kTrueCountDrill].bestLevel >= 3;
        case kRightSize: return p.drills[kBetSizingDrill].bestLevel >= 3;
        case kByTheBook: return life.bestCoachStreak >= 25;
        case kSharp: return s && s->coach.decisions >= 50 && s->coach.correct * 100 >= s->coach.decisions * 95;
        case kMarathon: return s && s->rounds >= 100;
        case kComeback: return s && s->net() > 0 && s->low() * 2 <= s->start();
        case kHighRoller: return s && s->net() >= 500;
        case kChallenger: return e.challengeFinished;
        case kTopOfTheBoard: return e.challengeFinished && e.challengeRank == 1 && e.challengeEntriesBefore >= 1;
        case kVeteran: return life.rounds >= 500;
        default: return false;
    }
}

}  // namespace

const Achievement& achievement(int index) { return kAchievements[index]; }

bool isUnlocked(const Profile& profile, int index) { return (profile.achievements >> index) & 1u; }

std::vector<int> evaluateAchievements(Profile& profile, const AchievementEvent& event) {
    std::vector<int> unlocked;
    for (int i = 0; i < kNumAchievements; ++i) {
        if (isUnlocked(profile, i) || !satisfied(i, profile, event)) continue;
        profile.achievements |= std::uint64_t{1} << i;
        unlocked.push_back(i);
    }
    return unlocked;
}

void announceAchievements(const std::vector<int>& newlyUnlocked, bool ascii) {
    for (int i : newlyUnlocked) {
        std::cout << "\n" << style::good(std::string(ascii ? "*" : "★") + " Achievement unlocked: ")
                  << style::bold(kAchievements[i].name) << " - " << kAchievements[i].description << "\n";
    }
}

void printAchievements(const Profile& profile) {
    int unlocked = 0;
    for (int i = 0; i < kNumAchievements; ++i) unlocked += isUnlocked(profile, i);
    std::cout << "\n" << style::bold("Achievements") << "  (" << unlocked << "/" << kNumAchievements << ")\n";
    for (int i = 0; i < kNumAchievements; ++i) {
        bool got = isUnlocked(profile, i);
        std::string line = std::string(got ? "[x] " : "[ ] ") + kAchievements[i].name + " - " + kAchievements[i].description;
        std::cout << "  " << (got ? style::good(line) : style::dim(line)) << "\n";
    }
}

}  // namespace bj
