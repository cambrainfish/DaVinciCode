#pragma once

#include "GameManager.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace dvcode::ui {

/// EasyX 图形界面：人类 P0 vs 电脑 P1，仅依赖 GameManager 规则 API
class GameView {
public:
    static constexpr int kWindowWidth = 1000;
    static constexpr int kWindowHeight = 720;

    GameView();
    void run();

private:
    struct CardSlot {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        PlayerId owner = PlayerId::Player0;
        std::size_t index = 0;
    };

    struct Button {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        int id = 0;
        const wchar_t* label = L"";
    };

    enum class BtnId : int {
        DrawBlack = 1,
        DrawWhite,
        GuessConfirm,
        Continue,
        Stop,
        ValueDown,
        ValueUp,
        ToggleColor,
        Restart,
    };

    GameManager game_;
    PlayerId human_{PlayerId::Player0};

    std::vector<CardSlot> slots_;
    std::vector<Button> buttons_;

    std::optional<std::size_t> selected_opp_index_;
    std::optional<std::size_t> selected_own_index_;
    int guess_value_{0};
    CardColor guess_color_{CardColor::Black};

    std::wstring status_line_;
    bool ai_busy_{false};

    void newGame();
    void layoutSlots();
    void layoutButtons();
    void render();
    void handleMouse(int x, int y);
    void handleButton(BtnId id);
    void tryGuess();
    void tryPlacePending(std::size_t index);
    void tryPenalty(std::size_t index);
    void runAiTurn();
    void setStatus(const std::wstring& text);

    [[nodiscard]] bool hit(const CardSlot& s, int x, int y) const noexcept;
    [[nodiscard]] bool hit(const Button& b, int x, int y) const noexcept;
    [[nodiscard]] std::wstring phaseText() const;
    [[nodiscard]] std::wstring cardText(const Card& c, PlayerId owner) const;
};

}  // namespace dvcode::ui
