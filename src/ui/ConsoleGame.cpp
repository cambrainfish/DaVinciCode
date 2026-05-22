#include "ui/ConsoleGame.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace dvcode::ui {

namespace {

void clearInputLine() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

}  // namespace

void ConsoleGame::printLine(char ch) const {
    std::cout << std::string(56, ch) << '\n';
}

std::string ConsoleGame::phaseName(GamePhase phase) {
    switch (phase) {
        case GamePhase::NotStarted:
            return "未开始";
        case GamePhase::TurnDrawRequired:
            return "须先摸牌";
        case GamePhase::TurnGuess:
            return "猜测电脑暗牌";
        case GamePhase::TurnAfterCorrectGuess:
            return "猜对：继续或结束";
        case GamePhase::TurnPlacePendingHidden:
            return "将摸到的牌暗插入手牌";
        case GamePhase::TurnPlacePendingRevealed:
            return "猜错：亮牌自动插入手牌";
        case GamePhase::GameOver:
            return "游戏结束";
    }
    return "?";
}

std::string ConsoleGame::colorName(CardColor color) {
    return color == CardColor::Black ? "黑" : "白";
}

void ConsoleGame::printHandRow(const char* label, PlayerId owner, bool hideOpponent) const {
    const auto& hand = game_.hand(owner);
    std::cout << label;
    for (std::size_t i = 0; i < hand.size(); ++i) {
        if (i > 0) {
            std::cout << " | ";
        }
        const Card& c = hand.at(i);
        const bool hide = hideOpponent && owner == kAi && !c.isRevealed();
        std::cout << '[' << i << ']';
        if (hide) {
            std::cout << "??";
        } else {
            std::cout << c.toString();
        }
    }
    std::cout << "  (暗牌 " << hand.hiddenCount() << " 张)\n";
}

void ConsoleGame::printBoard() const {
    printLine();
    const auto& deck = game_.deck();
    std::cout << "黑堆 " << deck.size(CardColor::Black) << " 张 | 白堆 "
              << deck.size(CardColor::White) << " 张 | 合计 " << deck.size() << "  |  阶段: "
              << phaseName(game_.phase()) << '\n';
    if (game_.hasPendingDraw() && game_.currentPlayer() == kHuman) {
        std::cout << "本回合摸到的牌（待处理）: " << game_.pendingDraw().toString() << '\n';
    }
    printHandRow("【电脑 P1】", kAi, true);
    printHandRow("【你的 P0】", kHuman, false);
    printLine('-');
}

void ConsoleGame::printPhaseHint() const {
    switch (game_.phase()) {
        case GamePhase::TurnDrawRequired:
            std::cout << "提示: 先选择从黑堆或白堆摸牌，再从该堆随机抽一张，然后猜牌。\n";
            break;
        case GamePhase::TurnGuess:
            std::cout << "提示: 选择电脑暗牌位置，再输入数值(0-11)与颜色。\n";
            break;
        case GamePhase::TurnAfterCorrectGuess:
            std::cout << "提示: 可继续猜；结束猜牌后若有待插入牌须指定插入位置。\n";
            break;
        case GamePhase::TurnPlacePendingHidden:
            std::cout << "提示: 新摸到的非 Joker 牌会自动按数值顺序插入手牌。\n";
            break;
        case GamePhase::TurnPlacePendingRevealed:
            std::cout << "提示: 猜错后摸到的牌会自动亮出并按数值顺序插入手牌。\n";
            break;
        default:
            break;
    }
}

void ConsoleGame::printHelp() const {
    std::cout << "\n命令: h=帮助  q=退出\n";
}

int ConsoleGame::readInt(const char* prompt, int minVal, int maxVal) const {
    for (;;) {
        std::cout << prompt << " [" << minVal << '-' << maxVal << "]: ";
        int v = 0;
        if (std::cin >> v && v >= minVal && v <= maxVal) {
            clearInputLine();
            return v;
        }
        std::cout << "输入无效，请重试。\n";
        clearInputLine();
    }
}

CardColor ConsoleGame::chooseDrawPile(bool forHuman) const {
    const auto& deck = game_.deck();
    const bool blackOk = game_.canDrawFrom(CardColor::Black);
    const bool whiteOk = game_.canDrawFrom(CardColor::White);

    if (blackOk && !whiteOk) {
        if (forHuman) {
            std::cout << "仅剩黑堆可摸。\n";
        }
        return CardColor::Black;
    }
    if (whiteOk && !blackOk) {
        if (forHuman) {
            std::cout << "仅剩白堆可摸。\n";
        }
        return CardColor::White;
    }
    if (!blackOk && !whiteOk) {
        throw std::logic_error("no pile available to draw");
    }

    if (!forHuman) {
        static std::mt19937 rng{std::random_device{}()};
        return (rng() % 2 == 0) ? CardColor::Black : CardColor::White;
    }

    std::cout << "\n选择摸牌牌堆:\n";
    std::cout << "  1. 黑堆 (剩余 " << deck.size(CardColor::Black) << " 张)\n";
    std::cout << "  2. 白堆 (剩余 " << deck.size(CardColor::White) << " 张)\n";
    const int choice = readInt("请选择", 1, 2);
    return choice == 1 ? CardColor::Black : CardColor::White;
}

void ConsoleGame::handleMandatoryDraw() {
    const CardColor pile = chooseDrawPile(true);
    const Card c = game_.draw(pile);
    std::cout << "\n【从" << colorName(pile) << "堆摸到】 " << c.toString()
              << "（暂不入手，须先猜牌）\n";
}

void ConsoleGame::handleGuess() {
    const auto& opp = game_.hand(kAi);
    std::cout << "\n电脑可猜的暗牌位置: ";
    bool any = false;
    for (std::size_t i = 0; i < opp.size(); ++i) {
        if (opp.isHidden(i)) {
            std::cout << i << ' ';
            any = true;
        }
    }
    std::cout << '\n';
    if (!any) {
        std::cout << "没有可猜的暗牌。\n";
        return;
    }

    const int idx = readInt("对手牌位", 0, static_cast<int>(opp.size()) - 1);
    if (!opp.isHidden(static_cast<std::size_t>(idx))) {
        std::cout << "该位置已翻开。\n";
        return;
    }

    const int val = readInt("猜测数值", Card::kMinValue, Card::kMaxValue);
    std::cout << "颜色: 1=黑(B)  2=白(W)\n";
    const int colorChoice = readInt("颜色", 1, 2);
    const auto color = colorChoice == 1 ? CardColor::Black : CardColor::White;

    try {
        const auto outcome = game_.guess(kAi, static_cast<std::size_t>(idx), val, color);
        if (outcome == GuessOutcome::Correct) {
            std::cout << "猜对了！电脑该位置已翻开。\n";
        } else {
            std::cout << "猜错了。";
            if (game_.hasPendingDraw()) {
                std::cout << " 接下来须将摸到的牌亮出并插入。";
            }
            std::cout << '\n';
        }
    } catch (const std::exception& ex) {
        std::cout << "猜牌失败: " << ex.what() << '\n';
    }
}

void ConsoleGame::handleAfterCorrect() {
    std::cout << "\n  1. 继续猜\n";
    std::cout << "  2. 结束猜牌\n";
    const int choice = readInt("请选择", 1, 2);
    if (choice == 1) {
        game_.continueGuessing();
        std::cout << "继续猜牌。\n";
    } else {
        game_.stopGuessingAndEndTurn();
        std::cout << "结束猜牌。";
        if (game_.canPlacePendingHidden()) {
            std::cout << " 请将待插入牌放入手牌。";
        }
        std::cout << '\n';
    }
}

void ConsoleGame::handlePlaceHidden() {
    try {
        game_.placePendingHidden(0);
        std::cout << "已按规则自动将新牌插入手牌，回合结束。\n";
    } catch (const std::exception& ex) {
        std::cout << "插入失败: " << ex.what() << '\n';
    }
}

void ConsoleGame::handlePlaceRevealed() {
    try {
        game_.placePendingRevealed(0);
        std::cout << "已按规则自动将惩罚牌亮出并插入手牌，回合结束。\n";
    } catch (const std::exception& ex) {
        std::cout << "插入失败: " << ex.what() << '\n';
    }
}

void ConsoleGame::humanTurn() {
    printBoard();
    printPhaseHint();

    if (game_.mustDrawBeforeGuess()) {
        handleMandatoryDraw();
        printBoard();
        printPhaseHint();
    }

    for (;;) {
        if (game_.phase() == GamePhase::TurnGuess) {
            handleGuess();
            return;
        }
        if (game_.phase() == GamePhase::TurnAfterCorrectGuess) {
            handleAfterCorrect();
            return;
        }
        if (game_.phase() == GamePhase::TurnPlacePendingHidden) {
            handlePlaceHidden();
            return;
        }
        if (game_.phase() == GamePhase::TurnPlacePendingRevealed) {
            handlePlaceRevealed();
            return;
        }
        return;
    }
}

void ConsoleGame::aiTurn() {
    printBoard();
    std::cout << "\n>>> 电脑回合...\n";

    static std::mt19937 rng{std::random_device{}()};

    try {
        if (game_.mustDrawBeforeGuess()) {
            const CardColor pile = chooseDrawPile(false);
            const Card c = game_.draw(pile);
            std::cout << "电脑从" << colorName(pile) << "堆摸到: " << c.toString()
                      << "，接着猜牌。\n";
        } else if (game_.canGuess()) {
            const auto& humanHand = game_.hand(kHuman);
            std::vector<std::size_t> hidden;
            for (std::size_t i = 0; i < humanHand.size(); ++i) {
                if (humanHand.isHidden(i)) {
                    hidden.push_back(i);
                }
            }
            if (!hidden.empty()) {
                std::uniform_int_distribution<std::size_t> distIdx(0, hidden.size() - 1);
                std::uniform_int_distribution<int> distVal(Card::kMinValue, Card::kMaxValue);
                const std::size_t idx = hidden[distIdx(rng)];
                const int val = distVal(rng);
                const auto color =
                    (rng() % 2 == 0) ? CardColor::Black : CardColor::White;
                const auto outcome = game_.guess(kHuman, idx, val, color);
                std::cout << "电脑猜你位置 " << idx << " 为 " << val
                          << colorName(color) << " → "
                          << (outcome == GuessOutcome::Correct ? "猜对" : "猜错") << '\n';
            }
        } else if (game_.phase() == GamePhase::TurnAfterCorrectGuess) {
            if (rng() % 2 == 0 && game_.canContinueGuessing()) {
                game_.continueGuessing();
                std::cout << "电脑选择继续猜。\n";
                aiTurn();
                return;
            }
            game_.stopGuessingAndEndTurn();
            std::cout << "电脑结束猜牌。\n";
        } else if (game_.canPlacePendingHidden() || game_.canPlacePendingRevealed()) {
            if (game_.canPlacePendingHidden()) {
                game_.placePendingHidden(0);
                std::cout << "电脑已按规则自动插入新摸到的牌。\n";
            } else {
                game_.placePendingRevealed(0);
                std::cout << "电脑已按规则自动处理猜错惩罚牌。\n";
            }
        }
    } catch (const std::exception& ex) {
        std::cout << "电脑回合异常: " << ex.what() << '\n';
    }

    if (game_.phase() == GamePhase::TurnAfterCorrectGuess) {
        aiTurn();
    }
}

void ConsoleGame::run() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
    std::cout << "========================================================\n";
    std::cout << "        达芬奇密码 - 控制台对战 (你 vs 电脑)\n";
    std::cout << "========================================================\n";
    printHelp();

    game_.startNewGame();

    while (game_.phase() != GamePhase::GameOver) {
        if (game_.currentPlayer() == kHuman) {
            std::cout << "\n######## 你的回合 ########\n";
            humanTurn();
        } else {
            aiTurn();
        }
    }

    printBoard();
    if (game_.winner()) {
        if (*game_.winner() == kHuman) {
            std::cout << "\n恭喜，你赢了！\n";
        } else {
            std::cout << "\n电脑获胜，再接再厉。\n";
        }
    }
}

}  // namespace dvcode::ui
