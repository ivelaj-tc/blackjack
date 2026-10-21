#include "ui/table.h"

#include <iostream>

#include "config/settings.h"
#include "ui/style.h"

namespace bj {
namespace {

// ---- compact style: "[A♠] [K♥] (21)", or white card faces when color is on

std::string renderCard(const Card& c, bool ascii) {
    std::string face = c.str(ascii);
    if (!style::enabled()) return "[" + face + "]";
    return style::paint(" " + face + " ", c.isRed() ? "1;31;47" : "1;30;47");
}

std::string renderHoleCard() { return style::enabled() ? style::paint(" ?? ", "1;97;44") : "[??]"; }

std::string describe(const std::vector<Card>& cards, bool ascii) {
    std::string out;
    for (const Card& c : cards) out += renderCard(c, ascii) + " ";
    return out;
}

// "(17)", highlighted, and red once the hand has gone over 21.
std::string renderTotal(const Hand& hand) {
    std::string text = "(" + std::to_string(hand.total()) + ")";
    return hand.isBust() ? style::bad(text) : style::bold(text);
}

// ---- multi-line card art: a 7x5 card, joined side by side for a hand

const std::size_t kCardRows = 5;

std::string paintFace(const std::string& line, const Card& c) {
    return style::paint(line, c.isRed() ? "1;31;47" : "1;30;47");  // red or black ink on a white card
}

std::string paintBack(const std::string& line) { return style::paint(line, "1;97;44"); }

std::vector<std::string> faceArt(const Card& c, bool ascii) {
    std::string rank = c.rankStr();
    std::string gap(5 - rank.size(), ' ');
    std::string side = ascii ? "|" : "│";
    std::vector<std::string> rows = {
        ascii ? "+-----+" : "┌─────┐",
        side + rank + gap + side,
        side + "  " + c.suitStr(ascii) + "  " + side,
        side + gap + rank + side,
        ascii ? "+-----+" : "└─────┘",
    };
    for (std::string& row : rows) row = paintFace(row, c);
    return rows;
}

std::vector<std::string> backArt(bool ascii) {
    std::string side = ascii ? "|" : "│";
    std::string fill = ascii ? "/////" : "░░░░░";
    std::vector<std::string> rows = {
        ascii ? "+-----+" : "┌─────┐", side + fill + side, side + fill + side, side + fill + side,
        ascii ? "+-----+" : "└─────┘",
    };
    for (std::string& row : rows) row = paintBack(row);
    return rows;
}

// The card rows of a hand, indented, one string per row. `hideSecond` shows the second card face down.
void appendHandArt(std::vector<std::string>& lines, const std::vector<Card>& cards, bool hideSecond, bool ascii) {
    if (cards.empty()) return;
    std::vector<std::vector<std::string>> arts;
    for (std::size_t i = 0; i < cards.size(); ++i) {
        arts.push_back(hideSecond && i == 1 ? backArt(ascii) : faceArt(cards[i], ascii));
    }
    for (std::size_t row = 0; row < kCardRows; ++row) {
        std::string line = "  ";
        for (std::size_t i = 0; i < arts.size(); ++i) line += (i == 0 ? "" : " ") + arts[i][row];
        lines.push_back(line);
    }
}

}  // namespace

std::string compactCard(const Card& card, bool ascii) { return renderCard(card, ascii); }

TableDisplay::TableDisplay(const Settings& settings)
    : settings_(settings), art_(cardArtEnabled(settings)), animating_(animationEnabled(settings)) {}

void TableDisplay::beginAnimation() { lastHeight_ = 0; }

std::vector<std::string> TableDisplay::render(const std::vector<PlayerHand>& hands, const Hand& dealer, bool hideHole,
                                              int active, const std::string& status) const {
    const bool ascii = settings_.ascii;
    const bool concealed = hideHole && dealer.cards.size() >= 2;  // hole card face down
    std::vector<std::string> lines;
    if (!status.empty()) lines.push_back(status);
    lines.push_back("");

    // Dealer
    std::string dealerTotal = !hideHole && !dealer.cards.empty() ? renderTotal(dealer) : "";
    if (art_) {
        lines.push_back("  " + style::bold("Dealer:") + (dealerTotal.empty() ? "" : " " + dealerTotal));
        appendHandArt(lines, dealer.cards, concealed, ascii);
    } else if (concealed) {
        lines.push_back("  " + style::bold("Dealer:") + " " + renderCard(dealer.cards[0], ascii) + " " +
                        renderHoleCard());
    } else {
        lines.push_back("  " + style::bold("Dealer:") + " " + describe(dealer.cards, ascii) + dealerTotal);
    }

    // Player
    for (std::size_t i = 0; i < hands.size(); ++i) {
        const PlayerHand& ph = hands[i];
        std::string total = ph.hand.cards.empty() ? "" : renderTotal(ph.hand);
        std::string label = hands.size() == 1 ? style::bold("You:") : style::bold("Hand " + std::to_string(i + 1) + ":");
        std::string marker = hands.size() > 1 && static_cast<int>(i) == active ? "  " + style::warn("<--") : "";
        std::string bet = hands.size() > 1 ? "  bet " + std::to_string(ph.bet) : "";

        if (art_) {
            lines.push_back("  " + label + (total.empty() ? "" : " " + total) + bet + marker);
            appendHandArt(lines, ph.hand.cards, false, ascii);
        } else if (hands.size() == 1) {
            lines.push_back("  " + label + "    " + describe(ph.hand.cards, ascii) + total);
        } else {
            lines.push_back("  " + label + " " + describe(ph.hand.cards, ascii) + total + bet + marker);
        }
    }

    lines.push_back("");
    return lines;
}

void TableDisplay::draw(const std::vector<std::string>& lines, bool replace) {
    const bool inPlace = animating_ && replace && lastHeight_ > 0;
    if (inPlace) std::cout << "\x1b[" << lastHeight_ << "A\r";  // back to the first line of the previous frame
    for (const std::string& line : lines) {
        std::cout << line;
        if (inPlace) std::cout << "\x1b[K";  // clear whatever the previous frame left on this line
        std::cout << "\n";
    }
    if (inPlace) std::cout << "\x1b[J";  // and anything below, in case this frame is shorter
    std::cout.flush();
    lastHeight_ = animating_ ? lines.size() : 0;
}

void TableDisplay::preview(const std::vector<PlayerHand>& hands, const Hand& dealer, bool hideHole,
                           const std::string& status) {
    if (!animating_) return;
    draw(render(hands, dealer, hideHole, -1, status), true);
}

void TableDisplay::show(const std::vector<PlayerHand>& hands, const Hand& dealer, bool hideHole, int active,
                        const std::string& status, bool replace) {
    draw(render(hands, dealer, hideHole, active, status), replace);
}

}  // namespace bj
