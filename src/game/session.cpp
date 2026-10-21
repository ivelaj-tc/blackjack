#include "game/session.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "config/settings.h"
#include "core/rules.h"
#include "core/seed.h"
#include "core/shoe.h"
#include "game/game_round.h"
#include "game/kelly.h"
#include "platform/audio.h"
#include "progress/achievements.h"
#include "progress/profile.h"
#include "progress/stats.h"
#include "training/coach.h"
#include "training/counting.h"
#include "ui/format.h"
#include "ui/input.h"
#include "ui/style.h"

namespace bj {
namespace {

std::string lowered(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

int announce(int bet, const std::string& how) {
    std::cout << style::dim("Betting " + std::to_string(bet) + " (" + how + ").") << "\n";
    return bet;
}

// Returns 0 if the player wants to quit. Besides a number of chips, the bet prompt takes shortcuts:
// Enter repeats the last bet, m bets the minimum, a goes all-in, kk places the Kelly bet, and k just shows the advice.
// `allowAdvice` is false in a challenge, where k and kk are switched off.
int promptBet(int chips, int lastBet, const Shoe& shoe, const Settings& settings, double strategyPenalty,
              bool allowAdvice) {
    while (true) {
        std::string line = readLine(style::warn("Chips: " + std::to_string(chips)) + "  |  Bet (" +
                                    std::to_string(settings.minBet) + "-" + std::to_string(chips) + ", " +
                                    (allowAdvice ? style::accent("k") + " = Kelly advice, " : "") +
                                    style::accent("q") + " to quit): ");
        if (isQuit(line)) return 0;

        std::string word = lowered(line);
        if (word == "k" || word == "kk") {
            if (!allowAdvice) {
                std::cout << "Kelly advice isn't available in a challenge.\n";
                continue;
            }
            if (word == "kk") return announce(kellyAdvice(shoe, settings, chips, strategyPenalty).bet, "Kelly");
            printCountLine(shoe);
            printKelly(shoe, settings, chips, strategyPenalty);
            continue;
        }
        if (word == "m") return announce(settings.minBet, "minimum");
        if (word == "a") return announce(chips, "all-in");
        if (line.empty()) {
            if (lastBet > 0) return announce(std::max(settings.minBet, std::min(lastBet, chips)), "same as last time");
            std::cout << "No previous bet yet. Enter an amount, or m for the minimum.\n";
            continue;
        }

        int bet = 0;
        if (parseInt(line, bet) && bet >= settings.minBet && bet <= chips) return bet;
        std::cout << "Please enter a whole number between " << settings.minBet << " and " << chips
                  << (allowAdvice ? ", or k for Kelly advice.\n" : ".\n");
    }
}

// How a round ended, as sound: a win, a paid blackjack or a push gets one sound. A loss gets the failure sound plus one
// for how you lost: the dealer taking the chips when you surrendered, or the dealer winning at the showdown. When every
// hand busted there was no showdown, and the bust sound already played at the moment each hand went over 21.
void playOutcomeSounds(const RoundResult& round) {
    if (round.net > 0) {
        audio::play(round.paidBlackjack ? audio::Sound::Blackjack : audio::Sound::Win);
    } else if (round.net == 0) {
        audio::play(audio::Sound::Push);
    } else {
        if (round.surrendered) {
            audio::play(audio::Sound::TakesChips);
        } else if (!round.busted) {
            audio::play(audio::Sound::DealerWins);
        }
        audio::play(audio::Sound::Failure);
    }
}

}  // namespace

SessionResult playSession(const Settings& settings, const SessionOptions& options, Profile& profile) {
    Shoe shoe(settings.decks, options.useSeed ? options.seed : seed::forShoe());
    CountStats counts;
    SessionStats session;
    int chips = settings.startingChips;
    int lastBet = 0;
    session.begin(chips);

    audio::play(audio::Sound::Shuffle);
    std::cout << "\nYou sit down with " << style::warn(std::to_string(chips) + " chips") << " at a " << settings.decks
              << "-deck table.\n"
              << style::dim(describeRules(settings.rules)) << "\n"
              << style::dim("Estimated house edge with basic strategy: " +
                            percent(houseEdge(settings.rules, settings.decks),
                                    houseEdge(settings.rules, settings.decks) < 0) +
                            ".")
              << "\n";
    if (!options.note.empty()) std::cout << style::dim(options.note) << "\n";
    if (settings.coach != CoachMode::Off) {
        std::cout << style::dim(std::string("Coach is on (") +
                                (settings.coach == CoachMode::Hints ? "hints and grading" : "grading only") +
                                "): your plays are compared with basic strategy for this table.")
                  << "\n";
    }
    if (seed::hasFixed() && !options.useSeed) {
        std::cout << style::dim("Fixed shuffle: seed " + std::to_string(seed::fixedValue()) + ".") << "\n";
    }
    if (!options.challenge) {
        std::cout << style::dim("Type k at the bet prompt for a " + std::to_string(settings.kellyPercent) +
                                "% Kelly bet suggestion.")
                  << "\n"
                  << style::dim("Bet shortcuts: Enter = same as last time, m = minimum, a = all-in, kk = Kelly bet.")
                  << "\n";
    } else {
        std::cout << style::dim("Bet shortcuts: Enter = same as last time, m = minimum, a = all-in.") << "\n";
    }
    if (options.counting) {
        std::cout << style::bold("Counting practice:") << " before each bet, give the Hi-Lo running count of every "
                  << "card dealt\n"
                  << "since the last shuffle.  2-6 = " << style::good("+1") << "   7-9 = 0   10-A = "
                  << style::bad("-1") << "\n";
    }
    std::cout << "\n";

    SessionEnd end = SessionEnd::OutOfChips;  // what ends the loop below unless something else does
    bool inputClosed = false;
    try {
        while (chips >= settings.minBet) {
            if (options.maxRounds > 0 && session.rounds >= options.maxRounds) {
                end = SessionEnd::RoundLimit;
                break;
            }
            if (shoe.reshuffleIfLow()) {
                audio::play(audio::Sound::Shuffle);
                std::cout << "\n" << style::accent("(Shuffling a fresh shoe -- the count resets to 0.)") << "\n";
            }
            double penalty = options.challenge ? 0.0
                                               : coach::strategyPenalty(lifetimeCoach(profile.lifetime), session.coach);
            if (options.counting && !quizCount(shoe, counts, settings, chips, penalty)) {
                end = SessionEnd::Quit;
                break;
            }

            int bet = promptBet(chips, lastBet, shoe, settings, penalty, !options.challenge);
            if (bet == 0) {
                end = SessionEnd::Quit;
                break;
            }
            lastBet = bet;

            RoundResult round = playRound(shoe, settings, chips, bet, session.coach);
            chips += round.net;
            session.record(round, chips);
            playOutcomeSounds(round);
            std::cout << "\n" << style::dim("--------------------------------") << "\n";
        }
    } catch (const InputClosed&) {
        inputClosed = true;  // leave the table below, then let the program end
    }

    if (inputClosed) {
        // no farewell line: the player is already gone
    } else if (end == SessionEnd::RoundLimit) {
        std::cout << "That's all " << options.maxRounds << " rounds: you finish with "
                  << style::bold(std::to_string(chips) + " chips") << ".\n";
    } else if (chips <= 0) {
        std::cout << style::bad("You're out of chips. Game over!") << "\n";
    } else if (chips < settings.minBet) {
        std::cout << style::bad("You have " + std::to_string(chips) + " chips left, less than the " +
                                std::to_string(settings.minBet) + "-chip minimum bet. Game over!")
                  << "\n";
    } else {
        int delta = chips - settings.startingChips;
        std::cout << "You leave the table with " << style::bold(std::to_string(chips) + " chips") << " ("
                  << style::bySign(signedInt(delta), delta) << ").\n";
    }

    Records records = addSession(profile.lifetime, session, counts);
    printSessionSummary(session, records, settings);
    if (options.counting) printCountStats(counts);

    AchievementEvent event;
    event.session = &session;
    announceAchievements(evaluateAchievements(profile, event), settings.ascii);

    if (inputClosed) throw InputClosed();

    SessionResult result;
    result.stats = session;
    result.end = end;
    result.chips = chips;
    return result;
}

}  // namespace bj
