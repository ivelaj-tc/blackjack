#pragma once

#include <string>
#include <vector>

#include "game/game_round.h"
#include "training/coach.h"
#include "training/counting.h"

namespace bj {

struct Settings;

// What happened during one sitting at the table.
struct SessionStats {
    int rounds = 0;
    int wins = 0;
    int losses = 0;
    int pushes = 0;
    int blackjacks = 0;
    int biggestWin = 0;   // chips, as a positive number (0 if none)
    int biggestLoss = 0;  // chips, as a positive magnitude (0 if none)
    std::vector<int> bankroll;  // chips at the start and after every round
    CoachStats coach;           // basic-strategy accuracy for decisions made with the coach on

    void begin(int chips) { bankroll.assign(1, chips); }
    void record(const RoundResult& result, int chipsAfter);

    int start() const { return bankroll.front(); }
    int end() const { return bankroll.back(); }
    int net() const { return end() - start(); }
    int peak() const;
    int low() const;
};

// Totals across every saved session.
struct LifetimeStats {
    long long sessions = 0;
    long long rounds = 0;
    long long wins = 0;
    long long losses = 0;
    long long pushes = 0;
    long long blackjacks = 0;
    int biggestWin = 0;
    int biggestLoss = 0;
    long long totalNet = 0;
    int bestSessionNet = 0;  // meaningful once sessions > 0
    int peakBankroll = 0;    // meaningful once sessions > 0
    long long countAsked = 0;
    long long countCorrect = 0;
    int bestCountStreak = 0;
    long long coachDecisions = 0;
    long long coachCorrect = 0;
    long long coachRounds = 0;
    int bestCoachStreak = 0;
};

// The lifetime coach totals in the form the strategy penalty wants (streak isn't needed there).
CoachStats lifetimeCoach(const LifetimeStats& lifetime);

// Personal records a session broke. The first session only sets the baseline, so it never breaks one.
struct Records {
    bool bestSession = false;
    bool peakBankroll = false;
};

// Folds a finished session into the lifetime totals and reports which records it broke.
// A session with no rounds only contributes its counting-practice and coaching results.
Records addSession(LifetimeStats& lifetime, const SessionStats& session, const CountStats& counting);

void printSessionSummary(const SessionStats& session, const Records& records, const Settings& settings);
void printLifetimeStats(const LifetimeStats& lifetime);

// A one-line chart of `values`, scaled between their minimum and maximum and shortened to at most 40 points.
std::string sparkline(const std::vector<int>& values, bool ascii);

}  // namespace bj
