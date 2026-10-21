#pragma once

namespace bj {

class Shoe;
struct Settings;

struct KellyAdvice {
    double edge;       // estimated player edge per unit wagered, including the strategy penalty
    double penalty;    // the part of the edge given up to strategy mistakes (0 when the coach has no data)
    double fullKelly;  // optimal share of the bankroll to bet (0 when there is no edge)
    double rawBet;     // fractional-Kelly stake in chips, before rounding
    int bet;           // suggested bet: rawBet rounded and kept within [minBet, chips]
};

// Sizes a fractional-Kelly bet for `chips` at a given true count. `strategyPenalty` (a fraction of a bet) is the edge
// lost to the player's own basic-strategy mistakes; leave it 0 to assume perfect play.
KellyAdvice kellyBet(const Settings& settings, double trueCount, int chips, double strategyPenalty = 0.0);

// The same for the current true count of a shoe.
KellyAdvice kellyAdvice(const Shoe& shoe, const Settings& settings, int chips, double strategyPenalty = 0.0);

void printKelly(const Shoe& shoe, const Settings& settings, int chips, double strategyPenalty = 0.0);

// The Hi-Lo rule of thumb used by the estimate: each +1 of true count is worth about +0.5% edge.
const double kEdgePerTrueCount = 0.005;

}  // namespace bj
