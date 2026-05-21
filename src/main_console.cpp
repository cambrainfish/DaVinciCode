#include "core/Card.hpp"
#include "core/Deck.hpp"
#include "core/Hand.hpp"

#include <iostream>
#include <string>

using namespace dvcode;

static void printHand(const std::string& label, const Hand& hand) {
    std::cout << label << ": ";
    for (std::size_t i = 0; i < hand.size(); ++i) {
        if (i > 0) {
            std::cout << " | ";
        }
        std::cout << hand.at(i).toString();
    }
    std::cout << "  (valid=" << (hand.isValidOrder() ? "yes" : "no") << ")\n";
}

int main() {
    std::cout << "=== Da Vinci Code / Core smoke test ===\n\n";

    Deck deck;
    deck.shuffle();
    std::cout << "Deck size after build: " << deck.size() << " (expect "
              << Deck::kStandardDeckSize << ")\n";

    Hand player;
    for (Card c : deck.deal(Deck::kInitialDeal)) {
        if (c.isJoker()) {
            player.insertAt(player.size(), c);
        } else {
            player.insertSorted(c);
        }
    }
    printHand("Initial deal (4)", player);

    Card drawn{7, CardColor::White};
    player.insertSorted(drawn);
    printHand("After insert 7W", player);

    Card joker = Card::makeJoker();
    player.insertAt(0, joker);
    printHand("Joker at front", player);

    player.insertAt(player.size(), Card::makeJoker());
    printHand("Two jokers", player);

    const auto ord1 = Card::compareStrict(Card{5, CardColor::Black}, Card{5, CardColor::White});
    const auto ord2 = Card::compareStrict(Card{3, CardColor::White}, Card{5, CardColor::Black});
    std::cout << "\nCompare 5B vs 5W: "
              << (ord1 == std::strong_ordering::less ? "5B < 5W OK" : "FAIL") << '\n';
    std::cout << "Compare 3W vs 5B: "
              << (ord2 == std::strong_ordering::less ? "3W < 5B OK" : "FAIL") << '\n';

    std::cout << "\nAll smoke checks done.\n";
    return 0;
}
