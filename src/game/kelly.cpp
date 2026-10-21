#include "game/kelly.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "config/settings.h"
#include "core/rules.h"
#include "core/shoe.h"
#include "ui/format.h"
#include "ui/style.h"

namespace bj {
namespace {

const double kHandVariance = 1.32;  // variance of one hand's result, in squared bet units

}  // namespace

// Kelly criterion for a small-edge bet: stake = edge / variance of the bankroll. Betting only a
// fraction of that gives up a little growth in exchange for much lower risk of ruin.
KellyAdvice kellyBet(const Settings& settings, double trueCount, int chips, double strategyPenalty) {
    KellyAdvice a;
    a.penalty = strategyPenalty;
    a.edge = -houseEdge(settings.rules, settings.decks) + kEdgePerTrueCount * trueCount - strategyPenalty;
    a.fullKelly = a.edge > 0 ? a.edge / kHandVariance : 0.0;
    a.rawBet = a.fullKelly * (settings.kellyPercent / 100.0) * chips;
    a.bet = std::max(settings.minBet, std::min(chips, static_cast<int>(std::lround(a.rawBet))));
    return a;
}

KellyAdvice kellyAdvice(const Shoe& shoe, const Settings& settings, int chips, double strategyPenalty) {
    return kellyBet(settings, shoe.trueCount(), chips, strategyPenalty);
}

void printKelly(const Shoe& shoe, const Settings& settings, int chips, double strategyPenalty) {
    KellyAdvice a = kellyAdvice(shoe, settings, chips, strategyPenalty);
    std::cout << style::bold("Kelly:") << " estimated edge " << style::bySign(percent(a.edge, true), a.edge);
    if (a.penalty >= 0.00005) {
        std::cout << " (after " << percent(a.penalty, false) << " lost to strategy slips)";
    }
    if (a.edge <= 0) {
        std::cout << " -- no edge, so Kelly bets nothing.\n"
                  << "       Suggested bet: " << style::bold(std::to_string(a.bet)) << " (the minimum).\n";
        return;
    }
    std::cout << ", full Kelly = " << percent(a.fullKelly, false) << " of your " << chips << " chips\n"
              << "       " << settings.kellyPercent << "% Kelly -> suggested bet: "
              << style::good(std::to_string(a.bet));
    if (std::lround(a.rawBet) < settings.minBet) {
        std::cout << " (the minimum; the Kelly stake is under " << settings.minBet
                  << (settings.minBet == 1 ? " chip)" : " chips)");
    }
    std::cout << "\n";
}

}  // namespace bj
