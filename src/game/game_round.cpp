#include "game/game_round.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "config/settings.h"
#include "core/card.h"
#include "core/rules.h"
#include "core/shoe.h"
#include "core/strategy.h"
#include "platform/audio.h"
#include "training/coach.h"
#include "ui/format.h"
#include "ui/input.h"
#include "ui/pacing.h"
#include "ui/style.h"
#include "ui/table.h"

namespace bj {
namespace {

// Every card that leaves the shoe goes through here, so each one gets its own card sound.
Card drawCard(Shoe& shoe) {
    audio::play(audio::Sound::Card);
    return shoe.draw();
}

int totalBets(const std::vector<PlayerHand>& hands) {
    int sum = 0;
    for (const PlayerHand& ph : hands) sum += ph.bet;
    return sum;
}

// `handLabel` names the hand being played when there are several ("Hand 2: "), since the table isn't redrawn
// between hands.
char promptAction(const std::string& handLabel, bool canHit, bool canDouble, bool canSplit, bool canSurrender) {
    std::string prompt = handLabel;
    std::string allowed;
    if (canHit) {
        prompt += style::tag("H") + "it  ";
        allowed += 'h';
    }
    prompt += style::tag("S") + "tand";
    allowed += 's';
    if (canDouble) {
        prompt += "  " + style::tag("D") + "ouble";
        allowed += 'd';
    }
    if (canSplit) {
        prompt += "  s" + style::tag("P") + "lit";
        allowed += 'p';
    }
    if (canSurrender) {
        prompt += "  su" + style::tag("R") + "render";
        allowed += 'r';
    }
    return promptChoice(prompt + ": ", allowed);
}

bool dealerMustHit(const Hand& dealer, const Rules& rules) {
    int total = dealer.total();
    return total < 17 || (rules.dealerHitsSoft17 && total == 17 && dealer.isSoft());
}

}  // namespace

RoundResult playRound(Shoe& shoe, const Settings& settings, int chips, int bet, CoachStats& coachStats) {
    const Rules& rules = settings.rules;
    if (settings.coach != CoachMode::Off) ++coachStats.rounds;
    TableDisplay display(settings);
    RoundResult result;

    Hand dealer;
    std::vector<PlayerHand> hands(1);
    hands[0].bet = bet;

    audio::play(audio::Sound::ChipsPlace);
    pauseBetweenCards(settings);

    // The opening deal: player, dealer, player, dealer. Each card is heard, and with the animation on also seen,
    // as it lands. The last card's frame is the table itself.
    display.beginAnimation();
    hands[0].hand.cards.push_back(drawCard(shoe));
    display.preview(hands, dealer, true);
    pauseBetweenCards(settings);
    dealer.cards.push_back(drawCard(shoe));
    display.preview(hands, dealer, true);
    pauseBetweenCards(settings);
    hands[0].hand.cards.push_back(drawCard(shoe));
    display.preview(hands, dealer, true);
    pauseBetweenCards(settings);
    dealer.cards.push_back(drawCard(shoe));
    display.show(hands, dealer, true, -1, "", true);

    // Insurance: a side bet of up to half the wager, paying 2:1 if the dealer has blackjack.
    int insurance = 0;
    if (dealer.cards[0].isAce() && bet / 2 >= 1 && chips - bet >= bet / 2) {
        // The count the player could know: the dealer's face-down card isn't part of it yet.
        double visibleCount = (shoe.runningCount() - dealer.cards[1].hiLo()) / std::max(shoe.decksRemaining(), 0.5);
        bool shouldInsure = strategy::shouldInsure(visibleCount);
        coach::showInsuranceHint(settings.coach, shouldInsure, visibleCount);
        char answer = promptChoice(style::warn("Dealer shows an Ace.") + " Buy insurance for " +
                                       std::to_string(bet / 2) + "? [y/N] ",
                                   "yn", 'n');
        coach::gradeInsurance(settings.coach, coachStats, answer == 'y', shouldInsure, visibleCount);
        if (answer == 'y') {
            insurance = bet / 2;
            audio::play(audio::Sound::ChipsPlace);
        }
    }

    int net = 0;
    const bool playerBJ = hands[0].hand.isBlackjack();
    const bool dealerBJ = dealer.isBlackjack();

    if (playerBJ || dealerBJ) display.show(hands, dealer, false);

    if (insurance > 0) {
        if (dealerBJ) {
            std::cout << style::good("Insurance pays 2:1 (" + signedInt(2 * insurance) + ").") << "\n";
            net += 2 * insurance;
        } else {
            std::cout << style::bad("Dealer does not have blackjack. Insurance lost (-" + std::to_string(insurance) +
                                    ").")
                      << "\n";
            net -= insurance;
        }
    }

    // Naturals are settled immediately.
    if (playerBJ || dealerBJ) {
        result.blackjack = playerBJ;
        result.paidBlackjack = playerBJ && !dealerBJ;
        if (playerBJ && dealerBJ) {
            std::cout << style::warn("Both have blackjack. Push.") << "\n";
        } else if (playerBJ) {
            int win = blackjackWin(rules, bet);
            std::cout << style::good("Blackjack! You win " + std::to_string(win) + ".") << "\n";
            net += win;
        } else {
            std::cout << style::bad("Dealer has blackjack. You lose " + std::to_string(bet) + ".") << "\n";
            net -= bet;
        }
        result.net = net;
        return result;
    }

    // Player's turn: work through each hand, which may grow as hands are split.
    for (std::size_t i = 0; i < hands.size(); ++i) {
        while (true) {
            PlayerHand& ph = hands[i];  // re-fetched each pass: splitting inserts into the vector
            if (ph.hand.total() >= 21) break;

            int available = chips - totalBets(hands) - insurance;
            bool twoCards = ph.hand.cards.size() == 2;
            bool canSplit = twoCards && ph.hand.canSplit() && available >= ph.bet &&
                            static_cast<int>(hands.size()) < kMaxHands;

            if (ph.locked) {
                // Split aces take one card and stand, unless the rules let a second ace be split again.
                if (!(rules.resplitAces && canSplit && ph.hand.cards[0].isAce())) break;
            }

            bool canHit = !ph.locked;
            bool canDouble = !ph.locked && twoCards && available >= ph.bet;
            bool canSurrender = rules.lateSurrender && hands.size() == 1 && twoCards && !ph.locked;

            std::string handLabel = hands.size() > 1 ? style::bold("Hand " + std::to_string(i + 1)) + ": " : "";
            strategy::Options options;
            options.canHit = canHit;
            options.canDouble = canDouble;
            options.canSplit = canSplit;
            options.canSurrender = canSurrender;
            strategy::Play advised = strategy::basicPlay(ph.hand, dealer.cards[0], rules, settings.decks, options);
            std::string situation = strategy::describeSituation(ph.hand, dealer.cards[0], canSplit);
            coach::showHint(settings.coach, advised, situation);

            char action = promptAction(handLabel, canHit, canDouble, canSplit, canSurrender);
            coach::grade(settings.coach, coachStats, strategy::playFromKey(action), advised, situation);
            if (action == 's') break;

            if (action == 'r') {
                // The dealer's hole card is shown, but no more cards are played.
                int loss = (ph.bet + 1) / 2;
                display.show(hands, dealer, false, -1, style::dim("Dealer reveals the hole card."));
                std::cout << style::bad("You surrender. You lose " + std::to_string(loss) + ".") << "\n";
                result.net = net - loss;
                result.surrendered = true;
                return result;
            }

            if (action == 'h') {
                ph.hand.cards.push_back(drawCard(shoe));
                if (ph.hand.isBust()) audio::play(audio::Sound::PlayerBusts);
            } else if (action == 'd') {
                ph.bet *= 2;
                audio::play(audio::Sound::ChipsPlace);
                std::string status = style::accent("Doubling down. Bet is now " + std::to_string(ph.bet) + ".");
                ph.hand.cards.push_back(drawCard(shoe));
                if (ph.hand.isBust()) audio::play(audio::Sound::PlayerBusts);
                display.show(hands, dealer, true, static_cast<int>(i), status);
                break;
            } else {  // split
                audio::play(audio::Sound::ChipsPlace);
                PlayerHand second;
                second.bet = ph.bet;
                second.hand.cards.push_back(ph.hand.cards.back());
                ph.hand.cards.pop_back();
                if (ph.hand.cards[0].isAce()) {
                    ph.locked = true;
                    second.locked = true;
                }
                hands.insert(hands.begin() + static_cast<std::ptrdiff_t>(i) + 1, second);  // invalidates ph

                std::string status = style::accent("Splitting.");
                display.beginAnimation();
                hands[i].hand.cards.push_back(drawCard(shoe));
                display.preview(hands, dealer, true, status);
                pauseBetweenCards(settings);
                hands[i + 1].hand.cards.push_back(drawCard(shoe));
                display.show(hands, dealer, true, static_cast<int>(i), status, true);
                continue;
            }
            display.show(hands, dealer, true, static_cast<int>(i));
        }
    }

    // Dealer's turn: always reveals the hole card, but only draws if a hand is still live.
    bool anyLive = false;
    for (const PlayerHand& ph : hands) anyLive = anyLive || !ph.hand.isBust();
    result.busted = !anyLive;

    display.show(hands, dealer, false, -1, style::dim("Dealer reveals the hole card."));
    if (anyLive) {
        while (dealerMustHit(dealer, rules)) {
            pauseBetweenCards(settings);
            dealer.cards.push_back(drawCard(shoe));
            display.show(hands, dealer, false, -1, style::dim("Dealer hits."), true);
        }
    }

    // Settle each hand.
    int handsNet = 0;
    for (std::size_t i = 0; i < hands.size(); ++i) {
        const PlayerHand& ph = hands[i];
        std::string prefix = hands.size() > 1 ? "Hand " + std::to_string(i + 1) + ": " : "";
        int mine = ph.hand.total();
        int theirs = dealer.total();

        const std::string stake = std::to_string(ph.bet);

        if (ph.hand.isBust()) {
            std::cout << prefix << style::bad("Bust. You lose " + stake + ".") << "\n";
            handsNet -= ph.bet;
        } else if (dealer.isBust()) {
            std::cout << prefix << style::good("Dealer busts. You win " + stake + ".") << "\n";
            handsNet += ph.bet;
        } else if (mine > theirs) {
            std::cout << prefix << style::good("You win " + stake + ".") << "\n";
            handsNet += ph.bet;
        } else if (mine < theirs) {
            std::cout << prefix << style::bad("Dealer wins. You lose " + stake + ".") << "\n";
            handsNet -= ph.bet;
        } else {
            std::cout << prefix << style::warn("Push. Bet returned.") << "\n";
        }
    }
    net += handsNet;

    if (hands.size() > 1 || insurance > 0) {
        std::cout << "Net this round: " << style::bySign(signedInt(net), net) << "\n";
    }
    result.net = net;
    return result;
}

}  // namespace bj
