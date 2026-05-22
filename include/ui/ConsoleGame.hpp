#pragma once

#include "core/GameManager.hpp"

namespace dvcode::ui {

/// 纯控制台文本 UI：人类 P0 vs 电脑 P1
class ConsoleGame {
public:
    void run();

private:
    static constexpr PlayerId kHuman = PlayerId::Player0;
    static constexpr PlayerId kAi = PlayerId::Player1;

    GameManager game_;

    void printLine(char ch = '=') const;
    void printBoard() const;
    void printPhaseHint() const;
    void printHelp() const;

    void humanTurn();
    void aiTurn();

    void handleMandatoryDraw();
    void handleGuess();
    void handleAfterCorrect();
    void handlePlaceHidden();
    void handlePlaceRevealed();
    void handlePenalty();

    [[nodiscard]] static std::string phaseName(GamePhase phase);
    [[nodiscard]] static std::string colorName(CardColor color);
    void printHandRow(const char* label, PlayerId owner, bool hideOpponent) const;
    [[nodiscard]] int readInt(const char* prompt, int minVal, int maxVal) const;
    [[nodiscard]] CardColor chooseDrawPile(bool forHuman) const;
};

}  // namespace dvcode::ui
