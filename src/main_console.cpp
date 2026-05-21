#include "core/Card.hpp"
#include "core/Deck.hpp"
#include "core/GameManager.hpp"
#include "core/Hand.hpp"

#include <iostream>
#include <sstream>
#include <string>

using namespace dvcode;

static void printHand(const std::string& label, const Hand& hand, bool hideUnrevealed) {
    std::cout << label << ": ";
    for (std::size_t i = 0; i < hand.size(); ++i) {
        if (i > 0) {
            std::cout << " | ";
        }
        const Card& c = hand.at(i);
        if (hideUnrevealed && !c.isRevealed()) {
            std::cout << '[' << i << "?]";
        } else {
            std::cout << '[' << i << ']' << c.toString();
        }
    }
    std::cout << "  (hidden=" << hand.hiddenCount() << ")\n";
}

static void printPhase(const GameManager& gm) {
    const char* phaseName = "?";
    switch (gm.phase()) {
        case GamePhase::NotStarted:
            phaseName = "NotStarted";
            break;
        case GamePhase::TurnDrawChoice:
            phaseName = "DrawChoice";
            break;
        case GamePhase::TurnGuess:
            phaseName = "Guess";
            break;
        case GamePhase::TurnAfterCorrectGuess:
            phaseName = "AfterCorrect";
            break;
        case GamePhase::TurnPlacePendingHidden:
            phaseName = "PlaceHidden";
            break;
        case GamePhase::TurnPlacePendingRevealed:
            phaseName = "PlaceRevealed";
            break;
        case GamePhase::TurnRevealPenalty:
            phaseName = "Penalty";
            break;
        case GamePhase::GameOver:
            phaseName = "GameOver";
            break;
    }
    std::cout << "Phase=" << phaseName << "  Current=P" << static_cast<int>(gm.currentPlayer())
              << "  Deck=" << gm.deck().size();
    if (gm.hasPendingDraw()) {
        std::cout << "  Pending=" << gm.pendingDraw().toString();
    }
    std::cout << '\n';
}

static int runSmokeTest() {
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
    printHand("Initial deal (4)", player, false);

    Card drawn{7, CardColor::White};
    player.insertSorted(drawn);
    printHand("After insert 7W", player, false);

    Card joker = Card::makeJoker();
    player.insertAt(0, joker);
    printHand("Joker at front", player, false);

    Hand guessHand;
    guessHand.insertSorted(Card{3, CardColor::Black, false});
    guessHand.insertAt(1, Card::makeJoker(false));
    std::cout << "Guess 3B on joker slot: "
              << (guessHand.isGuessCorrect(1, 3, CardColor::Black) ? "OK" : "FAIL") << '\n';

    const auto ord1 = Card::compareStrict(Card{5, CardColor::Black}, Card{5, CardColor::White});
    const auto ord2 = Card::compareStrict(Card{3, CardColor::White}, Card{5, CardColor::Black});
    std::cout << "Compare 5B vs 5W: "
              << (ord1 == std::strong_ordering::less ? "5B < 5W OK" : "FAIL") << '\n';
    std::cout << "Compare 3W vs 5B: "
              << (ord2 == std::strong_ordering::less ? "3W < 5B OK" : "FAIL") << '\n';

    std::cout << "\nAll smoke checks done.\n";
    return 0;
}

static int runGameSim() {
    std::cout << "=== GameManager simulation ===\n\n";
    GameManager gm;
    gm.startNewGame();
    printPhase(gm);

    if (gm.canDraw()) {
        const Card drawn = gm.draw();
        std::cout << "P0 drew: " << drawn.toString() << '\n';
    }

    const GuessOutcome g = gm.guess(PlayerId::Player1, 0, 0, CardColor::Black);
    std::cout << "P0 guessed P1[0]=0B -> " << (g == GuessOutcome::Correct ? "correct" : "wrong")
              << '\n';
    printPhase(gm);

    if (gm.phase() == GamePhase::TurnAfterCorrectGuess) {
        gm.stopGuessingAndEndTurn();
        std::cout << "P0 stopped guessing.\n";
    }
    if (gm.canPlacePendingHidden()) {
        gm.placePendingHidden(0);
        std::cout << "P0 placed pending at 0.\n";
    } else if (gm.canPlacePendingRevealed()) {
        gm.placePendingRevealed(0);
        std::cout << "P0 placed revealed pending at 0.\n";
    } else if (gm.canRevealPenalty()) {
        gm.applyPenaltyReveal(0);
        std::cout << "P0 penalty reveal at 0.\n";
    }

    printPhase(gm);
    std::cout << "Simulation step done.\n";
    return 0;
}

static CardColor parseColor(char ch) {
    if (ch == 'B' || ch == 'b') {
        return CardColor::Black;
    }
    if (ch == 'W' || ch == 'w') {
        return CardColor::White;
    }
    throw std::invalid_argument("color");
}

static int runInteractiveGame() {
    std::cout << "=== Da Vinci Code / 双人控制台对战 ===\n";
    std::cout << "命令: draw | skip | guess <对手0|1> <位> <值><B|W> | cont | stop | "
                 "place <位> | penalty <位> | show | quit\n\n";

    GameManager gm;
    gm.startNewGame();

    while (gm.phase() != GamePhase::GameOver) {
        printPhase(gm);
        for (int p = 0; p < 2; ++p) {
            const bool hide = true;
            printHand("P" + std::to_string(p), gm.hand(static_cast<PlayerId>(p)), hide);
        }

        std::cout << "\nP" << static_cast<int>(gm.currentPlayer()) << "> ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd == "quit" || cmd == "q") {
            break;
        }
        if (cmd == "show") {
            continue;
        }
        try {
            if (cmd == "draw" && gm.canDraw()) {
                std::cout << "摸到: " << gm.draw().toString() << "（暂不入牌）\n";
            } else if (cmd == "skip" && gm.canSkipDraw()) {
                gm.skipDraw();
                std::cout << "跳过摸牌，进入猜牌。\n";
            } else if (cmd == "guess" && gm.canGuess()) {
                int target = 0;
                std::size_t idx = 0;
                int val = 0;
                std::string colorToken;
                iss >> target >> idx >> val >> colorToken;
                if (colorToken.empty()) {
                    throw std::invalid_argument("need color B/W");
                }
                const auto outcome =
                    gm.guess(static_cast<PlayerId>(target), idx, val,
                             parseColor(colorToken[0]));
                std::cout << (outcome == GuessOutcome::Correct ? "猜对！\n" : "猜错。\n");
            } else if (cmd == "cont" && gm.canContinueGuessing()) {
                gm.continueGuessing();
                std::cout << "继续猜牌。\n";
            } else if (cmd == "stop" && gm.canStopGuessing()) {
                gm.stopGuessingAndEndTurn();
                std::cout << "结束猜牌阶段。\n";
            } else if (cmd == "place" && gm.canPlacePendingHidden()) {
                std::size_t idx = 0;
                iss >> idx;
                gm.placePendingHidden(idx);
                std::cout << "已将 pending 暗牌插入位 " << idx << "，回合结束。\n";
            } else if (cmd == "place" && gm.canPlacePendingRevealed()) {
                std::size_t idx = 0;
                iss >> idx;
                gm.placePendingRevealed(idx);
                std::cout << "已将 pending 亮牌插入位 " << idx << "，请 penalty 翻开己方一张。\n";
            } else if (cmd == "penalty" && gm.canRevealPenalty()) {
                std::size_t idx = 0;
                iss >> idx;
                gm.applyPenaltyReveal(idx);
                std::cout << "惩罚翻开完成，回合结束。\n";
            } else {
                std::cout << "当前阶段不可执行该命令。\n";
            }
        } catch (const std::exception& ex) {
            std::cout << "错误: " << ex.what() << '\n';
        }
    }

    if (gm.winner()) {
        std::cout << "\n游戏结束，胜者: P" << static_cast<int>(*gm.winner()) << '\n';
    }
    return 0;
}

int main(int argc, char* argv[]) {
    const bool gameMode = argc > 1 && std::string(argv[1]) == "--game";
    const bool simMode = argc > 1 && std::string(argv[1]) == "--sim";
    if (gameMode) {
        return runInteractiveGame();
    }
    if (simMode) {
        return runGameSim();
    }
    return runSmokeTest();
}
