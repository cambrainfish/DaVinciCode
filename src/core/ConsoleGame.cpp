#include "ConsoleGame.hpp"

#include <iostream>
#include <string>
#include <limits>
#include <cstdlib>
#include <ctime>

namespace dvcode::ui {

    void ConsoleGame::run() {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        printLine();
        std::cout << "        欢迎来到 达芬奇密码 (Da Vinci Code) 控制台版!\n";
        printLine();

        game_.startNewGame();

        while (game_.phase() != GamePhase::GameOver) {
            printBoard();
            printPhaseHint();

            if (game_.currentPlayer() == kHuman) {
                humanTurn();
            }
            else {
                aiTurn();
            }
        }

        printBoard();
        printLine('*');
        std::cout << "                游戏结束！\n";
        // 判定胜负
        if (game_.hand(kHuman).hiddenCount() + game_.hand(kHuman).size() - game_.hand(kHuman).hiddenCount() == 0) {
            // 如果某一方牌全开，说明输了。通过剩下的暗牌数等逻辑可以直接判断
        }
        // 这里采用简单规则：谁的手牌没全被翻开谁就赢
        if (game_.hand(kHuman).hiddenCount() > 0) {
            std::cout << "         🎉 恭喜你，你战胜了 AI，赢得了胜利！\n";
        }
        else {
            std::cout << "         ❌ 遗憾！AI 成功破译了你的所有密码。\n";
        }
        printLine('*');
    }

    void ConsoleGame::printLine(char ch) const {
        std::cout << std::string(60, ch) << '\n';
    }

    void ConsoleGame::printBoard() const {
        printLine('-');
        // 打印电脑手牌（对人类隐藏暗牌）
        printHandRow("AI  的手牌", kAi, true);
        // 打印人类手牌（全部可见）
        printHandRow("你的手牌", kHuman, false);

        // 打印牌堆剩余情况
        std::cout << "[牌堆剩余] ⬛ 黑牌: " << game_.deck().size(CardColor::Black)
            << " 张 | ⬜ 白牌: " << game_.deck().size(CardColor::White) << " 张\n";
        printLine('-');
    }

    void ConsoleGame::printHandRow(const char* label, PlayerId owner, bool hideOpponent) const {
        std::cout << label << ": ";
        const Hand& h = game_.hand(owner);
        for (std::size_t i = 0; i < h.size(); ++i) {
            if (i > 0) std::cout << "  ";
            const Card& c = h.at(i);

            std::string colorStr = (c.color() == CardColor::Black) ? "B" : "W";

            if (hideOpponent && c.isHidden() && game_.phase() != GamePhase::GameOver) {
                std::cout << "[" << colorStr << "?]";
            }
            else {
                std::string valStr = c.isJoker() ? "J" : std::to_wstring(c.value()).size() == 1 ? std::to_string(c.value()) : std::to_string(c.value());
                if (c.isRevealed()) {
                    std::cout << "(" << valStr << colorStr << ")";
                }
                else {
                    std::cout << "[" << valStr << colorStr << "]";
                }
            }
        }
        std::cout << "\n";
    }

    void ConsoleGame::printPhaseHint() const {
        std::cout << "当前阶段: " << phaseName(game_.phase())
            << " | 当前回合方: " << (game_.currentPlayer() == kHuman ? "【你】" : "【AI】") << "\n";
    }

    std::string ConsoleGame::phaseName(GamePhase phase) {
        switch (phase) {
        case GamePhase::NotStarted: return "游戏未开始";
        case GamePhase::TurnDrawRequired: return "必须摸牌";
        case GamePhase::TurnGuess: return "声明猜测对手暗牌";
        case GamePhase::TurnAfterCorrectGuess: return "猜对奖励阶段 (可继续或收手)";
        case GamePhase::TurnPlacePendingHidden: return "将摸到的暗牌放回手牌";
        case GamePhase::TurnPlacePendingRevealed: return "惩罚：将罚牌公开并放回手牌";
        case GamePhase::GameOver: return "游戏结束";
        }
        return "未知";
    }

    std::string ConsoleGame::colorName(CardColor color) {
        return color == CardColor::Black ? "黑色" : "白色";
    }

    int ConsoleGame::readInt(const char* prompt, int minVal, int maxVal) const {
        int val;
        while (true) {
            std::cout << prompt;
            if (std::cin >> val) {
                if (val >= minVal && val <= maxVal) {
                    return val;
                }
                std::cout << "输入超出范围 [" << minVal << ", " << maxVal << "]，请重试。\n";
            }
            else {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "无效的数字输入，请重试。\n";
            }
        }
    }

    CardColor ConsoleGame::chooseDrawPile(bool forHuman) const {
        if (forHuman) {
            while (true) {
                std::cout << "请选择摸牌颜色 (0: ⬛黑色, 1: ⬜白色): ";
                int choice;
                if (std::cin >> choice) {
                    if (choice == 0 && game_.canDrawFrom(CardColor::Black)) return CardColor::Black;
                    if (choice == 1 && game_.canDrawFrom(CardColor::White)) return CardColor::White;
                    std::cout << "该牌堆已空或选择无效！\n";
                }
                else {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
            }
        }
        else {
            bool canB = game_.canDrawFrom(CardColor::Black);
            bool canW = game_.canDrawFrom(CardColor::White);
            if (canB && canW) {
                return (std::rand() % 2 == 0) ? CardColor::Black : CardColor::White;
            }
            return canB ? CardColor::Black : CardColor::White;
        }
    }

    void ConsoleGame::humanTurn() {
        switch (game_.phase()) {
        case GamePhase::TurnDrawRequired:
            handleMandatoryDraw();
            break;
        case GamePhase::TurnGuess:
            handleGuess();
            break;
        case GamePhase::TurnAfterCorrectGuess:
            handleAfterCorrect();
            break;
        case GamePhase::TurnPlacePendingHidden:
            handlePlaceHidden();
            break;
        case GamePhase::TurnPlacePendingRevealed:
            handlePlaceRevealed();
            break;
        default:
            break;
        }
    }

    void ConsoleGame::handleMandatoryDraw() {
        CardColor color = chooseDrawPile(true);
        Card c = game_.draw(color);
        std::cout << ">>>> 你摸到了一张 " << colorName(color) << " 牌。\n";
    }

    void ConsoleGame::handleGuess() {
        std::cout << "开始破解 AI 的密码...\n";
        const Hand& aiHand = game_.hand(kAi);

        // 让玩家选择猜测 AI 手牌的哪个位置
        int targetIdx = readInt("请输入你想猜测的 AI 暗牌下标 (从 0 开始): ", 0, static_cast<int>(aiHand.size() - 1));
        if (!aiHand.isHidden(targetIdx)) {
            std::cout << "错误：该位置的牌已经是明牌了，请重新选择！\n";
            return;
        }

        int guessVal = readInt("请宣告你猜测的数字 (0-11, Joker请输入 -1): ", -1, 11);

        int colorChoice = readInt("请宣告你猜测的颜色 (0: ⬛黑色, 1: ⬜白色): ", 0, 1);
        CardColor guessColor = (colorChoice == 0) ? CardColor::Black : CardColor::White;

        std::cout << ">>>> 你宣告：AI 的第 " << targetIdx << " 张牌是 " << guessVal << (guessColor == CardColor::Black ? "B" : "W") << "...\n";

        GuessOutcome outcome = game_.guess(kAi, targetIdx, guessVal, guessColor);
        if (outcome == GuessOutcome::Correct) {
            std::cout << "🎉 猜对了！该牌被强制公开！\n";
        }
        else {
            std::cout << "❌ 猜错了！非常遗憾。\n";
        }
    }

    void ConsoleGame::handleAfterCorrect() {
        std::cout << "【连击奖励】你上一次猜对了！您可以选择：\n";
        std::cout << "1. 继续连击：再次猜测 AI 的其他暗牌（不需要也不可以再摸牌）\n";
        std::cout << "2. 收手结束：稳健收兵，安全安置您这回合摸到的暗牌\n";
        int choice = readInt("请输入选项 (1 或 2): ", 1, 2);

        if (choice == 1) {
            game_.continueGuessing();
            std::cout << ">>>> 你选择继续猜测！\n";
        }
        else {
            game_.stopGuessingAndEndTurn();
            std::cout << ">>>> 你选择收手，当前轮猜牌阶段结束。\n";
        }
    }

    void ConsoleGame::handlePlaceHidden() {
        int n = static_cast<int>(game_.hand(kHuman).size());
        std::cout << "请将摸到的暗牌插入到你的手牌中。当前手牌大小为 " << n << "。\n";
        std::cout << "可以插入的合法下标范围是 [0 到 " << n << "]。\n";

        while (true) {
            int idx = readInt("请输入你想放置的位置下标: ", 0, n);
            try {
                game_.placePendingHidden(static_cast<std::size_t>(idx));
                std::cout << ">>>> 暗牌已成功归位。\n";
                break;
            }
            catch (...) {
                std::cout << "⚠️ 错误：放置位置违反了数值严格递增排序规则！请重新选择。\n";
            }
        }
    }

    void ConsoleGame::handlePlaceRevealed() {
        int n = static_cast<int>(game_.hand(kHuman).size());
        std::cout << "【惩罚亮牌】因为你猜错了，刚才摸到的牌必须【公开】亮出插入手牌。\n";
        std::cout << "可以插入的合法下标范围是 [0 到 " << n << "]。\n";

        while (true) {
            int idx = readInt("请输入你想放置的位置下标: ", 0, n);
            try {
                game_.placePendingRevealed(static_cast<std::size_t>(idx));
                std::cout << ">>>> 惩罚明牌已公开摆放就位。\n";
                break;
            }
            catch (...) {
                std::cout << "⚠️ 错误：放置位置违反了数值严格递增排序规则！请重新选择。\n";
            }
        }
    }

    void ConsoleGame::aiTurn() {
