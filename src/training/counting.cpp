#include "training/counting.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include "core/shoe.h"
#include "game/kelly.h"
#include "ui/format.h"
#include "ui/input.h"
#include "ui/style.h"

namespace bj {

void printCountLine(const Shoe& shoe) {
    double trueCount = shoe.trueCount();
    double shown = std::fabs(trueCount) < 0.05 ? 0.0 : trueCount;  // matches how tenths() rounds it
    std::cout << "Running " << style::bySign(signedInt(shoe.runningCount()), shoe.runningCount())
              << "  |  decks left " << tenths(shoe.decksRemaining(), false) << "  |  true count "
              << style::bySign(tenths(trueCount, true), shown) << "\n";
}

bool quizCount(const Shoe& shoe, CountStats& stats, const Settings& settings, int chips, double strategyPenalty) {
    if (shoe.cardsSeen() == 0) {
        std::cout << "\n" << style::accent("Fresh shoe: the running count is 0.") << "\n";
        printKelly(shoe, settings, chips, strategyPenalty);
        return true;
    }

    int guess = 0;
    while (true) {
        std::string line = readLine("\n" + style::bold("What's the running count (Hi-Lo)?") + " [q to quit] ");
        if (isQuit(line)) return false;
        if (parseInt(line, guess)) break;
        std::cout << "Please enter a whole number such as -3, 0 or +2.\n";
    }

    int actual = shoe.runningCount();
    ++stats.asked;
    if (guess == actual) {
        ++stats.correct;
        stats.bestStreak = std::max(stats.bestStreak, ++stats.streak);
        std::cout << style::good("Correct! (streak: " + std::to_string(stats.streak) + ")") << "\n";
    } else {
        stats.streak = 0;
        std::cout << style::bad("Not quite -- you said " + signedInt(guess) + ", the running count is " +
                                signedInt(actual) + ".")
                  << "\n";
    }

    printCountLine(shoe);
    printKelly(shoe, settings, chips, strategyPenalty);
    return true;
}

void printCountStats(const CountStats& stats) {
    if (stats.asked == 0) return;
    std::cout << style::bold("Counting practice:") << " " << stats.correct << "/" << stats.asked << " correct ("
              << stats.correct * 100 / stats.asked << "%), best streak " << stats.bestStreak << ".\n";
}

}  // namespace bj
