#include "training/drills.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "config/settings.h"
#include "core/card.h"
#include "core/rules.h"
#include "core/seed.h"
#include "game/kelly.h"
#include "platform/terminal.h"
#include "progress/achievements.h"
#include "progress/profile.h"
#include "ui/format.h"
#include "ui/input.h"
#include "ui/style.h"
#include "ui/table.h"

namespace bj {
namespace {

using Clock = std::chrono::steady_clock;

enum class Outcome { Passed, Failed, Cancelled };

const int kLevels = 5;
const int kQuestions = 10;     // questions per true-count or bet-sizing run
const int kPassMark = 8;       // correct answers needed to pass one of those

double secondsSince(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

std::string number(double value) {  // 2.5 -> "2.5", 3 -> "3", 2.25 -> "2.25"
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", value);
    return buf;
}

std::string seconds(double value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f", value);
    return buf;
}

int uniform(std::mt19937& gen, int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(gen); }

template <typename T>
const T& pick(std::mt19937& gen, const std::vector<T>& items) {
    return items[static_cast<std::size_t>(uniform(gen, 0, static_cast<int>(items.size()) - 1))];
}

void say(bool good, const std::string& text) { std::cout << (good ? style::good(text) : style::bad(text)) << "\n"; }

// ------------------------------------------------------------------ speed count (flash cards)

struct SpeedLevel {
    int cards;
    double secondsPerCard;
    bool hiddenCard;  // a whole deck with one card face down: the cards shown must add up to minus the hidden one
};

const SpeedLevel kSpeedLevels[kLevels] = {
    {10, 2.0, false}, {20, 1.5, false}, {51, 1.2, true}, {51, 0.8, true}, {51, 0.5, true},
};

std::string speedDescription(int level) {
    const SpeedLevel& l = kSpeedLevels[level - 1];
    return l.hiddenCard ? "a deck with one card hidden (51 cards), " + seconds(l.secondsPerCard) + " s each"
                        : std::to_string(l.cards) + " cards, " + seconds(l.secondsPerCard) + " s each";
}

void flashCard(const Card& card, int index, int total, bool ascii) {
    std::string line = "  Card " + std::to_string(index) + "/" + std::to_string(total) + "   " + compactCard(card, ascii);
    if (style::enabled()) {
        std::cout << "\r" << line << "   \x1b[K" << std::flush;  // redraw in place
    } else {
        std::cout << line << "\n" << std::flush;
    }
}

Outcome speedDrill(int level, const Settings& settings) {
    const SpeedLevel& cfg = kSpeedLevels[level - 1];
    std::mt19937 gen(seed::forDrill());

    std::vector<Card> deck;
    for (int suit = 0; suit < 4; ++suit)
        for (int rank = 0; rank < 13; ++rank) deck.push_back(Card{rank, suit});
    std::shuffle(deck.begin(), deck.end(), gen);
    Card hidden = deck.back();
    deck.resize(static_cast<std::size_t>(cfg.cards));

    std::cout << "\n" << style::bold("Speed count, level " + std::to_string(level)) << ": " << speedDescription(level)
              << ".\nKeep a running Hi-Lo count (2-6 = +1, 7-9 = 0, 10-A = -1). ";
    if (cfg.hiddenCard) std::cout << "A whole deck counts to 0, so the cards you see add up to minus the hidden card.";
    std::cout << "\n";
    if (isQuit(readLine("Press Enter to start, or q to go back: "))) return Outcome::Cancelled;

    if (style::enabled()) std::cout << "\n";
    int count = 0;
    int index = 0;
    for (const Card& card : deck) {
        count += card.hiLo();
        flashCard(card, ++index, cfg.cards, settings.ascii);
        if (isInteractive()) std::this_thread::sleep_for(std::chrono::duration<double>(cfg.secondsPerCard));
    }
    if (style::enabled()) std::cout << "\r\x1b[K";
    std::cout << "\n";

    Clock::time_point asked = Clock::now();
    int guess = 0;
    while (true) {
        std::string line = readLine(style::bold("What is the running count?") + " ");
        if (parseInt(line, guess)) break;
        std::cout << "Please enter a whole number such as -3, 0 or +2.\n";
    }
    double took = secondsSince(asked);

    bool ok = guess == count;
    if (ok) {
        say(true, "Correct! The count was " + signedInt(count) + ". (answered in " + seconds(took) + " s)");
    } else {
        say(false, "Not quite -- you said " + signedInt(guess) + ", the count was " + signedInt(count) + ".");
    }
    if (cfg.hiddenCard) {
        std::cout << "The hidden card was " << compactCard(hidden, settings.ascii) << " (Hi-Lo " << signedInt(hidden.hiLo())
                  << "), so the cards shown always add up to " << signedInt(-hidden.hiLo()) << ".\n";
    }
    return ok ? Outcome::Passed : Outcome::Failed;
}

// ------------------------------------------------------------------ true-count conversion

struct TrueCountLevel {
    std::vector<double> decks;  // decks remaining to pick from
    int runningMin;
    int runningMax;
    double limit;  // seconds allowed per answer
};

std::vector<TrueCountLevel> trueCountLevels() {
    return {
        {{1, 2, 4}, -8, 12, 30},
        {{1, 2, 3, 4, 5, 6}, -12, 18, 20},
        {{1.5, 2.5, 3.5, 4.5, 5.5}, -15, 20, 15},
        {{1.25, 1.75, 2.25, 2.75, 3.25, 3.75, 4.25, 5.25}, -20, 25, 12},
        {{0.75, 1.25, 1.75, 2.25, 2.75, 3.25, 3.75, 4.25, 4.75, 5.25, 5.75}, -25, 30, 8},
    };
}

std::string trueCountDescription(int level) {
    static const char* const text[kLevels] = {
        "whole decks (1, 2 or 4), 30 s per answer", "whole decks from 1 to 6, 20 s",
        "half decks (2.5, 3.5, ...), 15 s",         "quarter decks (2.25, 3.75, ...), 12 s",
        "any quarter deck down to 0.75, 8 s",
    };
    return text[level - 1];
}

Outcome trueCountDrill(int level, const Settings&) {
    const TrueCountLevel cfg = trueCountLevels()[static_cast<std::size_t>(level - 1)];
    std::mt19937 gen(seed::forDrill());

    std::cout << "\n" << style::bold("True-count conversion, level " + std::to_string(level)) << ": "
              << trueCountDescription(level) << ".\nTrue count = running count / decks remaining. Answer with the nearest "
              << "whole number (anything within 0.5 counts).\n";
    if (isQuit(readLine("Press Enter to start, or q to go back: "))) return Outcome::Cancelled;

    int right = 0;
    for (int q = 1; q <= kQuestions; ++q) {
        int running = uniform(gen, cfg.runningMin, cfg.runningMax);
        double decks = pick(gen, cfg.decks);
        double exact = running / decks;

        std::cout << "\nQuestion " << q << "/" << kQuestions << ": running count " << style::bold(signedInt(running))
                  << ", " << style::bold(number(decks)) << " decks left.\n";
        double answer = 0;
        Clock::time_point asked = Clock::now();
        while (true) {
            std::string line = readLine("True count (" + number(cfg.limit) + " s)? ");
            if (parseDouble(line, answer)) break;
            std::cout << "Please enter a number such as 3 or -1.5.\n";
        }
        double took = secondsSince(asked);

        std::string working = signedInt(running) + " / " + number(decks) + " = " + tenths(exact, true);
        if (took > cfg.limit) {
            say(false, "Too slow (" + seconds(took) + " s): " + working);
        } else if (std::fabs(answer - exact) <= 0.5) {
            say(true, "Correct (" + seconds(took) + " s): " + working);
            ++right;
        } else {
            say(false, "Not quite: " + working);
        }
    }
    std::cout << "\nScore: " << right << "/" << kQuestions << "\n";
    return right >= kPassMark ? Outcome::Passed : Outcome::Failed;
}

// ------------------------------------------------------------------ bet sizing

struct BetLevel {
    int trueCountStep;               // true counts come in multiples of 1/trueCountStep
    int bankrollStep;                // bankrolls are multiples of this
    int bankrollMin;
    int bankrollMax;
    std::vector<int> kellyPercents;
    double tolerance;                // how far off, as a fraction of the right bet, still counts
    double limit;                    // seconds allowed per answer
};

std::vector<BetLevel> betLevels() {
    return {
        {1, 1000, 1000, 5000, {50}, 0.25, 40},
        {2, 500, 1000, 5000, {50}, 0.15, 30},
        {10, 100, 1000, 10000, {25, 50, 75}, 0.10, 25},
        {10, 100, 1000, 10000, {25, 50, 75}, 0.08, 20},
        {10, 50, 1000, 10000, {25, 50, 75}, 0.05, 15},
    };
}

std::string betDescription(int level) {
    static const char* const text[kLevels] = {
        "whole true counts, half Kelly, 25% leeway, 40 s",
        "half-count steps, half Kelly, 15% leeway, 30 s",
        "any true count, 25-75% Kelly, 10% leeway, 25 s",
        "as level 3 with 8% leeway, 20 s",
        "as level 3 with 5% leeway, 15 s",
    };
    return text[level - 1];
}

Outcome betSizingDrill(int level, const Settings& settings) {
    const BetLevel cfg = betLevels()[static_cast<std::size_t>(level - 1)];
    std::mt19937 gen(seed::forDrill());
    const double edgeAtZero = houseEdge(settings.rules, settings.decks);
    const double breakEven = edgeAtZero / kEdgePerTrueCount;  // the true count where the edge turns positive

    std::cout << "\n" << style::bold("Bet sizing, level " + std::to_string(level)) << ": " << betDescription(level)
              << ".\nBet = bankroll x Kelly% x (edge / 1.32), where edge = 0.5% x true count - house edge ("
              << percent(edgeAtZero, false) << " at this table), so the edge turns positive above a true count of "
              << tenths(breakEven, false) << ".\n";
    if (isQuit(readLine("Press Enter to start, or q to go back: "))) return Outcome::Cancelled;

    int right = 0;
    for (int q = 1; q <= kQuestions; ++q) {
        // true counts a little above break-even, where there is something to bet
        int steps = cfg.trueCountStep;
        int lo = static_cast<int>(std::ceil((breakEven + 0.5) * steps));
        int hi = static_cast<int>(std::floor((breakEven + 8.0) * steps));
        double tc = uniform(gen, lo, std::max(lo, hi)) / static_cast<double>(steps);
        int bankroll = uniform(gen, cfg.bankrollMin / cfg.bankrollStep, cfg.bankrollMax / cfg.bankrollStep) * cfg.bankrollStep;

        Settings table = settings;
        table.kellyPercent = pick(gen, cfg.kellyPercents);
        table.minBet = 1;  // keep the minimum from hiding the arithmetic
        KellyAdvice a = kellyBet(table, tc, bankroll);

        std::cout << "\nQuestion " << q << "/" << kQuestions << ": true count " << style::bold(tenths(tc, true))
                  << ", bankroll " << style::bold(std::to_string(bankroll)) << ", " << table.kellyPercent
                  << "% Kelly.\n";
        int answer = 0;
        Clock::time_point asked = Clock::now();
        while (true) {
            std::string line = readLine("How many chips do you bet (" + number(cfg.limit) + " s)? ");
            if (parseInt(line, answer)) break;
            std::cout << "Please enter a whole number of chips.\n";
        }
        double took = secondsSince(asked);

        std::string working = "edge " + percent(a.edge, true) + " -> full Kelly " + percent(a.fullKelly, false) + " -> " +
                              std::to_string(table.kellyPercent) + "% of " + std::to_string(bankroll) + " = " +
                              tenths(a.rawBet, false) + " -> bet " + std::to_string(a.bet);
        double allowed = std::max(1.0, cfg.tolerance * a.bet);
        if (took > cfg.limit) {
            say(false, "Too slow (" + seconds(took) + " s): " + working);
        } else if (std::fabs(answer - a.bet) <= allowed) {
            say(true, "Correct (" + seconds(took) + " s): " + working);
            ++right;
        } else {
            say(false, "Not quite: " + working);
        }
    }
    std::cout << "\nScore: " << right << "/" << kQuestions << "\n";
    return right >= kPassMark ? Outcome::Passed : Outcome::Failed;
}

// ------------------------------------------------------------------ menu

struct DrillInfo {
    const char* name;
    std::string (*describe)(int level);
    Outcome (*run)(int level, const Settings& settings);
};

const DrillInfo kDrills[kNumDrills] = {
    {"Speed count (flash cards)", speedDescription, speedDrill},
    {"True-count conversion", trueCountDescription, trueCountDrill},
    {"Bet sizing", betDescription, betSizingDrill},
};

// Returns the chosen level, or 0 to go back.
int chooseLevel(const DrillInfo& drill, int bestLevel) {
    std::cout << "\n" << style::bold(drill.name) << "\n";
    for (int level = 1; level <= kLevels; ++level) {
        std::cout << "  " << style::tag(std::to_string(level)) << " " << drill.describe(level)
                  << (level <= bestLevel ? style::good("  (passed)") : "") << "\n";
    }
    int dflt = std::min(kLevels, bestLevel + 1);
    while (true) {
        std::string line = readLine("Level (1-" + std::to_string(kLevels) + ", Enter = " + std::to_string(dflt) +
                                    ", q to go back): ");
        if (isQuit(line)) return 0;
        if (line.empty()) return dflt;
        int level = 0;
        if (parseInt(line, level) && level >= 1 && level <= kLevels) return level;
        std::cout << "Please enter a level from 1 to " << kLevels << ".\n";
    }
}

}  // namespace

void runDrillMenu(const Settings& settings, Profile& profile) {
    while (true) {
        std::cout << "\n" << style::bold("Training drills") << "\n";
        for (int k = 0; k < kNumDrills; ++k) {
            const DrillRecord& r = profile.drills[k];
            std::cout << "  " << style::tag(std::to_string(k + 1)) << " " << kDrills[k].name
                      << style::dim("   best level passed: " + std::to_string(r.bestLevel) + ", " +
                                    std::to_string(r.passes) + "/" + std::to_string(r.attempts) + " runs passed")
                      << "\n";
        }
        std::cout << "  " << style::tag("B") << " Back\n";
        char choice = promptChoice(style::accent("> "), "123b");
        if (choice == 'b') return;

        int kind = choice - '1';
        int level = chooseLevel(kDrills[kind], profile.drills[kind].bestLevel);
        if (level == 0) continue;

        Outcome outcome = kDrills[kind].run(level, settings);
        if (outcome == Outcome::Cancelled) continue;

        DrillRecord& record = profile.drills[kind];
        ++record.attempts;
        if (outcome == Outcome::Passed) {
            ++record.passes;
            record.bestLevel = std::max(record.bestLevel, level);
            say(true, "Level " + std::to_string(level) + " passed!");
        } else {
            std::cout << "Not passed this time. Try again, or pick another level.\n";
        }
        announceAchievements(evaluateAchievements(profile, AchievementEvent()), settings.ascii);
    }
}

}  // namespace bj
