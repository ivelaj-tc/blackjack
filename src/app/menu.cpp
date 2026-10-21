#include "app/menu.h"

#include <iostream>
#include <string>

#include "config/settings.h"
#include "core/rules.h"
#include "core/seed.h"
#include "game/challenge.h"
#include "game/session.h"
#include "platform/audio.h"
#include "progress/achievements.h"
#include "progress/leaderboard.h"
#include "progress/profile.h"
#include "progress/save.h"
#include "progress/stats.h"
#include "training/drills.h"
#include "ui/format.h"
#include "ui/input.h"
#include "ui/style.h"

namespace bj {
namespace {

// A boxed title when color is on (terminals that do color also do Unicode); a plain line otherwise.
void printBanner() {
    if (!style::enabled()) {
        std::cout << "=== BLACKJACK ===\n";
        return;
    }
    std::cout << "\n"
              << "  " << style::accent("╔═══════════════════╗") << "\n"
              << "  " << style::accent("║") << "     " << style::bold("BLACKJACK") << "     " << style::accent("║")
              << "\n"
              << "  " << style::accent("╚═══════════════════╝") << "\n"
              << "         " << style::bold("♠") << " " << style::bad("♥") << " " << style::bad("♦") << " "
              << style::bold("♣") << "\n";
}

// Prompts for an integer in [lo, hi]. A blank or invalid answer leaves `value` unchanged.
void editInt(const std::string& label, int lo, int hi, int& value) {
    std::string line = readLine(label + " (" + std::to_string(lo) + "-" + std::to_string(hi) + ", blank to cancel): ");
    if (line.empty()) return;
    int n = 0;
    if (parseInt(line, n) && n >= lo && n <= hi) {
        value = n;
    } else {
        std::cout << "Please enter a whole number between " << lo << " and " << hi << ".\n";
    }
}

// "on", "off", or a dimmed note when no player or sound files were found.
std::string soundState(bool on) {
    if (!audio::available()) return style::dim("unavailable");
    return on ? style::good("on") : "off";
}

std::string onOff(bool on) { return on ? style::good("on") : "off"; }

std::string coachText(CoachMode mode) {
    switch (mode) {
        case CoachMode::Grade: return style::good("grading only");
        case CoachMode::Hints: return style::good("hints and grading");
        default: return "off";
    }
}

// Flipping what is currently in effect turns an automatic setting into an explicit choice.
Toggle flipped(bool currentlyOn) { return currentlyOn ? Toggle::Off : Toggle::On; }

void rulesMenu(Settings& settings) {
    Rules& rules = settings.rules;
    while (true) {
        double edge = houseEdge(rules, settings.decks);
        std::cout << "\n"
                  << style::bold("Table rules") << "  (estimated house edge " << percent(edge, edge < 0)
                  << " with basic strategy)\n"
                  << "  " << style::tag("1") << " Dealer on soft 17: " << (rules.dealerHitsSoft17 ? "hits" : "stands")
                  << "\n"
                  << "  " << style::tag("2") << " Blackjack pays: " << blackjackPayout(rules) << "\n"
                  << "  " << style::tag("3") << " Late surrender: " << onOff(rules.lateSurrender) << "\n"
                  << "  " << style::tag("4") << " Resplit aces: " << onOff(rules.resplitAces) << "\n"
                  << "  " << style::tag("B") << " Back\n";
        char choice = promptChoice(style::accent("> "), "1234b");
        if (choice == 'b') return;

        if (choice == '1') rules.dealerHitsSoft17 = !rules.dealerHitsSoft17;
        else if (choice == '2') rules.blackjackSixToFive = !rules.blackjackSixToFive;
        else if (choice == '3') rules.lateSurrender = !rules.lateSurrender;
        else rules.resplitAces = !rules.resplitAces;
    }
}

void settingsMenu(Settings& settings) {
    while (true) {
        double edge = houseEdge(settings.rules, settings.decks);
        std::cout << "\n"
                  << style::bold("Settings") << "\n"
                  << "  " << style::tag("1") << " Decks in the shoe: " << settings.decks << "\n"
                  << "  " << style::tag("2")
                  << " Suit symbols: " << (settings.ascii ? "ASCII (S H D C)" : "Unicode (♠ ♥ ♦ ♣)") << "\n"
                  << "  " << style::tag("3") << " Kelly fraction: " << settings.kellyPercent << "% of full Kelly\n"
                  << "  " << style::tag("4") << " Starting chips: " << settings.startingChips << "\n"
                  << "  " << style::tag("5") << " Minimum bet: " << settings.minBet << "\n"
                  << "  " << style::tag("6") << " Colors: " << onOff(style::enabled()) << "\n"
                  << "  " << style::tag("7") << " Music: " << soundState(audio::musicEnabled()) << "\n"
                  << "  " << style::tag("8") << " Sound effects: " << soundState(audio::effectsEnabled()) << "\n"
                  << "  " << style::tag("A") << " Card art: " << onOff(cardArtEnabled(settings)) << "\n"
                  << "  " << style::tag("D") << " Dealing animation: " << onOff(animationEnabled(settings)) << "\n"
                  << "  " << style::tag("C") << " Basic-strategy coach: " << coachText(settings.coach) << "\n"
                  << "  " << style::tag("R") << " Table rules  " << style::dim("(house edge " + percent(edge, edge < 0) + ")")
                  << "\n"
                  << "  " << style::tag("B") << " Back\n";
        char choice = promptChoice(style::accent("> "), "12345678adcrb");
        if (choice == 'b') return;

        if (choice == '1') {
            editInt("Number of decks", kMinDecks, kMaxDecks, settings.decks);
        } else if (choice == '2') {
            settings.ascii = !settings.ascii;
        } else if (choice == '3') {
            editInt("Percent of full Kelly to bet", kMinKellyPercent, kMaxKellyPercent, settings.kellyPercent);
        } else if (choice == '4') {
            editInt("Starting chips", kMinStartingChips, kMaxStartingChips, settings.startingChips);
            if (settings.minBet > settings.startingChips) {  // keep at least one bet affordable
                settings.minBet = settings.startingChips;
                std::cout << "Minimum bet lowered to " << settings.minBet << " to fit the starting chips.\n";
            }
        } else if (choice == '5') {
            editInt("Minimum bet", 1, settings.startingChips, settings.minBet);
        } else if (choice == '6') {
            settings.colors = flipped(style::enabled());
            applyColors(settings);
        } else if (choice == '7' || choice == '8') {
            if (!audio::available()) {
                std::cout << "Sound isn't available: it needs a player (afplay on macOS) and the assets/ folder.\n";
            } else if (choice == '7') {
                settings.music = flipped(audio::musicEnabled());
                audio::setMusicEnabled(settings.music == Toggle::On);
            } else {
                settings.effects = flipped(audio::effectsEnabled());
                audio::setEffectsEnabled(settings.effects == Toggle::On);
            }
        } else if (choice == 'a') {
            settings.cardArt = flipped(cardArtEnabled(settings));
        } else if (choice == 'd') {
            settings.animation = flipped(animationEnabled(settings));
            if (settings.animation == Toggle::On && !animationEnabled(settings)) {
                std::cout << "The dealing animation needs colors on (Settings -> Colors).\n";
            }
        } else if (choice == 'c') {
            settings.coach = settings.coach == CoachMode::Off     ? CoachMode::Grade
                             : settings.coach == CoachMode::Grade ? CoachMode::Hints
                                                                  : CoachMode::Off;
        } else {
            rulesMenu(settings);
        }
    }
}

void statsMenu(Profile& profile, const Settings& settings) {
    while (true) {
        printLifetimeStats(profile.lifetime);
        std::cout << "\n  " << style::tag("R") << " Reset statistics   " << style::tag("B") << " Back\n";
        char choice = promptChoice(style::accent("> "), "rb");
        if (choice == 'b') return;

        if (promptChoice("Erase all lifetime statistics? [y/N] ", "yn", 'n') == 'y') {
            profile.lifetime = LifetimeStats();
            saveProgress(settings, profile);
            std::cout << "Statistics reset.\n";
        }
    }
}

}  // namespace

void runMainMenu(Settings& settings, Profile& profile) {
    printBanner();
    if (seed::hasFixed()) {
        std::cout << style::dim("Fixed shuffle: seed " + std::to_string(seed::fixedValue()) +
                                ". Games and drills repeat exactly for the same plays.")
                  << "\n";
    }

    while (true) {
        std::cout << "\n  " << style::tag("1") << " Play blackjack\n"
                  << "  " << style::tag("2") << " Card counting practice (Hi-Lo)\n"
                  << "  " << style::tag("3") << " Settings\n"
                  << "  " << style::tag("4") << " Statistics\n"
                  << "  " << style::tag("5") << " Training drills\n"
                  << "  " << style::tag("6") << " Challenge " << style::dim("(shoe " + std::to_string(challenge::currentSeed()) + ")")
                  << "\n"
                  << "  " << style::tag("7") << " Leaderboard\n"
                  << "  " << style::tag("8") << " Achievements\n"
                  << "  " << style::tag("Q") << " Quit\n";
        char choice = promptChoice(style::accent("> "), "12345678q");
        if (choice == '1' || choice == '2') {
            SessionOptions options;
            options.counting = choice == '2';
            playSession(settings, options, profile);
            saveProgress(settings, profile);
        } else if (choice == '3') {
            settingsMenu(settings);
            sanitize(settings);
            saveProgress(settings, profile);
        } else if (choice == '4') {
            statsMenu(profile, settings);
        } else if (choice == '5') {
            runDrillMenu(settings, profile);
            saveProgress(settings, profile);
        } else if (choice == '6') {
            challenge::run(settings, profile);
            saveProgress(settings, profile);
        } else if (choice == '7') {
            printLeaderboard(profile.leaderboard, challenge::currentSeed());
        } else if (choice == '8') {
            printAchievements(profile);
        } else {
            break;
        }
    }

    std::cout << "Thanks for playing!\n";
}

}  // namespace bj
