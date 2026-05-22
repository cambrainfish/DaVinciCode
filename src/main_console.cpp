#include "core/Card.hpp"
#include "core/Deck.hpp"
#include "core/Hand.hpp"
#include "ui/ConsoleGame.hpp"

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

static int runSmokeTest() {
    std::cout << "=== Da Vinci Code / Core smoke test ===\n\n";

    Deck deck;
    deck.shuffle();
    std::cout << "Deck total: " << deck.size() << " (expect " << Deck::kStandardDeckSize
              << "), black=" << deck.size(CardColor::Black) << " white="
              << deck.size(CardColor::White) << '\n';

    Hand player;
    for (Card c : deck.deal(Deck::kInitialDeal)) {
        if (c.isJoker()) {
            player.insertAt(player.size(), c);
        } else {
            player.insertSorted(c);
        }
    }
    printHand("Initial deal (4)", player);

    Hand guessHand;
    guessHand.insertSorted(Card{3, CardColor::Black, false});
    guessHand.insertAt(1, Card::makeJoker(false));
    std::cout << "Guess 3B on joker: "
              << (guessHand.isGuessCorrect(1, 3, CardColor::Black) ? "OK" : "FAIL") << '\n';

    const auto ord1 = Card::compareStrict(Card{5, CardColor::Black}, Card{5, CardColor::White});
    std::cout << "5B < 5W: " << (ord1 == std::strong_ordering::less ? "OK" : "FAIL") << '\n';
    std::cout << "\nSmoke test done.\n";
    return 0;
}

static int runGameSim() {
    std::cout << "=== GameManager simulation ===\n";
    GameManager gm;
    gm.startNewGame();
    if (gm.canDraw()) {
        std::cout << "P0 drew from black: " << gm.draw(CardColor::Black).toString() << '\n';
    }
    const auto g = gm.guess(PlayerId::Player1, 0, 0, CardColor::Black);
    std::cout << "guess -> " << (g == GuessOutcome::Correct ? "correct" : "wrong") << '\n';
    std::cout << "Simulation done.\n";
    return 0;
}

int main(int argc, char* argv[]) {
    std::string mode = "play";
    if (argc > 1) {
        mode = argv[1];
    }

    if (mode == "--smoke") {
        return runSmokeTest();
    }
    if (mode == "--sim") {
        return runGameSim();
    }
    if (mode == "--play" || mode == "play") {
        ui::ConsoleGame game;
        game.run();
        return 0;
    }

    std::cout << "用法:\n";
    std::cout << "  DaVinciCode.exe           控制台对战（默认）\n";
    std::cout << "  DaVinciCode.exe --smoke   核心冒烟测试\n";
    std::cout << "  DaVinciCode.exe --sim     状态机快速模拟\n";
    return 0;
}
