#include "progress/stats.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "config/settings.h"
#include "ui/format.h"
#include "ui/style.h"

namespace bj {
namespace {

const std::size_t kMaxSparkPoints = 40;

std::string winRate(long long wins, long long rounds) {
    return rounds == 0 ? "0%" : std::to_string(wins * 100 / rounds) + "%";
}

}  // namespace

void SessionStats::record(const RoundResult& result, int chipsAfter) {
    ++rounds;
    if (result.net > 0) {
        ++wins;
        biggestWin = std::max(biggestWin, result.net);
    } else if (result.net < 0) {
        ++losses;
        biggestLoss = std::max(biggestLoss, -result.net);
    } else {
        ++pushes;
    }
    if (result.blackjack) ++blackjacks;
    bankroll.push_back(chipsAfter);
}

int SessionStats::peak() const { return *std::max_element(bankroll.begin(), bankroll.end()); }
int SessionStats::low() const { return *std::min_element(bankroll.begin(), bankroll.end()); }

CoachStats lifetimeCoach(const LifetimeStats& lifetime) {
    CoachStats stats;
    stats.decisions = static_cast<int>(lifetime.coachDecisions);
    stats.correct = static_cast<int>(lifetime.coachCorrect);
    stats.rounds = static_cast<int>(lifetime.coachRounds);
    stats.bestStreak = lifetime.bestCoachStreak;
    return stats;
}

Records addSession(LifetimeStats& lifetime, const SessionStats& session, const CountStats& counting) {
    lifetime.countAsked += counting.asked;
    lifetime.countCorrect += counting.correct;
    lifetime.bestCountStreak = std::max(lifetime.bestCountStreak, counting.bestStreak);
    lifetime.coachDecisions += session.coach.decisions;
    lifetime.coachCorrect += session.coach.correct;
    lifetime.coachRounds += session.coach.rounds;
    lifetime.bestCoachStreak = std::max(lifetime.bestCoachStreak, session.coach.bestStreak);

    Records records;
    if (session.rounds == 0) return records;

    if (lifetime.sessions > 0) {
        records.bestSession = session.net() > lifetime.bestSessionNet;
        records.peakBankroll = session.peak() > lifetime.peakBankroll;
        lifetime.bestSessionNet = std::max(lifetime.bestSessionNet, session.net());
        lifetime.peakBankroll = std::max(lifetime.peakBankroll, session.peak());
    } else {
        lifetime.bestSessionNet = session.net();
        lifetime.peakBankroll = session.peak();
    }

    ++lifetime.sessions;
    lifetime.rounds += session.rounds;
    lifetime.wins += session.wins;
    lifetime.losses += session.losses;
    lifetime.pushes += session.pushes;
    lifetime.blackjacks += session.blackjacks;
    lifetime.biggestWin = std::max(lifetime.biggestWin, session.biggestWin);
    lifetime.biggestLoss = std::max(lifetime.biggestLoss, session.biggestLoss);
    lifetime.totalNet += session.net();
    return records;
}

std::string sparkline(const std::vector<int>& values, bool ascii) {
    static const char* const unicodeLevels[8] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    static const char* const asciiLevels[8] = {"_", ".", "-", "~", "=", "+", "*", "#"};
    const char* const* levels = ascii ? asciiLevels : unicodeLevels;
    if (values.empty()) return "";

    std::vector<int> points;
    if (values.size() <= kMaxSparkPoints) {
        points = values;
    } else {
        for (std::size_t i = 0; i < kMaxSparkPoints; ++i) {
            points.push_back(values[i * (values.size() - 1) / (kMaxSparkPoints - 1)]);
        }
    }

    int lo = *std::min_element(points.begin(), points.end());
    int hi = *std::max_element(points.begin(), points.end());
    std::string out;
    for (int v : points) {
        int level = hi == lo ? 3 : static_cast<int>(std::lround(static_cast<double>(v - lo) * 7 / (hi - lo)));
        out += levels[level];
    }
    return out;
}

void printSessionSummary(const SessionStats& s, const Records& records, const Settings& settings) {
    if (s.rounds == 0) return;

    std::cout << "\n" << style::bold("Session summary") << "\n"
              << "  Rounds: " << s.rounds << "   " << style::good("Wins " + std::to_string(s.wins)) << " | "
              << style::bad("Losses " + std::to_string(s.losses)) << " | "
              << style::warn("Pushes " + std::to_string(s.pushes)) << "   (win rate "
              << winRate(s.wins, s.rounds) << ")\n"
              << "  Blackjacks: " << s.blackjacks << "\n"
              << (s.coach.decisions > 0 ? "  Strategy: " + std::to_string(s.coach.correct) + "/" +
                                              std::to_string(s.coach.decisions) + " plays matched basic strategy (" +
                                              winRate(s.coach.correct, s.coach.decisions) + "), best streak " +
                                              std::to_string(s.coach.bestStreak) + "\n"
                                        : "")
              << "  Biggest win: " << (s.biggestWin > 0 ? style::good(signedInt(s.biggestWin)) : "-")
              << "   Biggest loss: " << (s.biggestLoss > 0 ? style::bad(signedInt(-s.biggestLoss)) : "-") << "\n"
              << "  Net: " << style::bySign(signedInt(s.net()), s.net()) << "   (" << s.start()
              << (settings.ascii ? " -> " : " → ") << s.end() << ")   Peak " << s.peak() << "   Low " << s.low()
              << "\n"
              << "  Bankroll: " << style::accent(sparkline(s.bankroll, settings.ascii)) << "\n";

    if (records.bestSession) std::cout << "  " << style::good("New personal best: your best session so far!") << "\n";
    if (records.peakBankroll) std::cout << "  " << style::good("New personal best: your highest bankroll!") << "\n";
}

void printLifetimeStats(const LifetimeStats& t) {
    std::cout << "\n" << style::bold("Lifetime statistics") << "\n";
    if (t.sessions == 0 && t.countAsked == 0 && t.coachDecisions == 0) {
        std::cout << "  Nothing recorded yet. Play a session and it will show up here.\n";
        return;
    }
    if (t.sessions > 0) {
        std::cout << "  Sessions: " << t.sessions << "   Rounds: " << t.rounds << "\n"
                  << "  " << style::good("Wins " + std::to_string(t.wins)) << " | "
                  << style::bad("Losses " + std::to_string(t.losses)) << " | "
                  << style::warn("Pushes " + std::to_string(t.pushes)) << "   (win rate "
                  << winRate(t.wins, t.rounds) << ")\n"
                  << "  Blackjacks: " << t.blackjacks << "\n"
                  << "  Biggest win: " << (t.biggestWin > 0 ? style::good(signedInt(t.biggestWin)) : "-")
                  << "   Biggest loss: " << (t.biggestLoss > 0 ? style::bad(signedInt(-t.biggestLoss)) : "-") << "\n"
                  << "  Total net: " << style::bySign(signedInt(static_cast<int>(t.totalNet)), t.totalNet)
                  << "   Best session: " << style::bySign(signedInt(t.bestSessionNet), t.bestSessionNet)
                  << "   Peak bankroll: " << t.peakBankroll << "\n";
    }
    if (t.coachDecisions > 0) {
        std::cout << "  Basic strategy: " << t.coachCorrect << "/" << t.coachDecisions << " plays matched ("
                  << winRate(t.coachCorrect, t.coachDecisions) << "), best streak " << t.bestCoachStreak << "\n";
    }
    if (t.countAsked > 0) {
        std::cout << "  Counting practice: " << t.countCorrect << "/" << t.countAsked << " correct ("
                  << winRate(t.countCorrect, t.countAsked) << "), best streak " << t.bestCountStreak << "\n";
    }
}

}  // namespace bj
