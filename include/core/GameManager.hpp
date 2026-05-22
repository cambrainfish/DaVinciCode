#pragma once

#include "core/Card.hpp"
#include "core/Deck.hpp"
#include "core/Hand.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace dvcode {

enum class PlayerId : int { Player0 = 0, Player1 = 1 };

enum class GamePhase {
    NotStarted,
    /// 必须先摸牌（牌堆非空时）；牌堆空则直接进入 TurnGuess
    TurnDrawRequired,
    /// 须猜测对手一张暗牌（pending 牌可能仍在手中外）
    TurnGuess,
    /// 猜对：可继续猜，或结束并将 pending 暗插入手牌
    TurnAfterCorrectGuess,
    /// 猜对后选择结束回合：将 pending 暗牌按规则插入手牌
    TurnPlacePendingHidden,
    /// 猜错且本回合摸过牌：将 pending 亮牌按规则插入手牌（即惩罚完成）
    TurnPlacePendingRevealed,
    GameOver,
};

enum class GuessOutcome { Correct, Wrong };

/// 双人达芬奇密码对局状态机（逻辑层，与 UI 无关）
class GameManager {
public:
    static constexpr std::size_t kPlayerCount = 2;

    void startNewGame();

    [[nodiscard]] GamePhase phase() const noexcept { return phase_; }
    [[nodiscard]] PlayerId currentPlayer() const noexcept { return current_; }
    [[nodiscard]] const Deck& deck() const noexcept { return deck_; }
    [[nodiscard]] const Hand& hand(PlayerId p) const noexcept { return hands_[toIndex(p)]; }
    [[nodiscard]] PlayerId opponent(PlayerId p) const noexcept;
    [[nodiscard]] std::optional<PlayerId> winner() const noexcept { return winner_; }

    [[nodiscard]] bool hasPendingDraw() const noexcept { return pending_draw_.has_value(); }
    [[nodiscard]] const Card& pendingDraw() const { return *pending_draw_; }

    /// 本回合须摸牌且至少一堆非空
    [[nodiscard]] bool canDraw() const noexcept;
    [[nodiscard]] bool canDrawFrom(CardColor pile) const noexcept;
    [[nodiscard]] bool mustDrawBeforeGuess() const noexcept;
    [[nodiscard]] bool canPlacePendingHidden() const noexcept;
    [[nodiscard]] bool canPlacePendingRevealed() const noexcept;
    [[nodiscard]] bool canGuess() const noexcept;
    [[nodiscard]] bool canContinueGuessing() const noexcept;
    [[nodiscard]] bool canStopGuessing() const noexcept;

    /// 从选定颜色牌堆摸牌（强制）：进入猜牌阶段，牌暂存于 pending
    Card draw(CardColor pile);

    void placePendingHidden(std::size_t index);
    void placePendingRevealed(std::size_t index);

    GuessOutcome guess(PlayerId target, std::size_t index, int value, CardColor color);
    void continueGuessing();
    void stopGuessingAndEndTurn();
    void applyPenaltyReveal(std::size_t ownIndex);

private:
    static std::size_t toIndex(PlayerId p) noexcept {
        return static_cast<std::size_t>(p);
    }

    void dealToPlayer(PlayerId p);
    void insertPendingInto(std::size_t index, bool reveal);
    void checkWinAfterOpponentReveal(PlayerId opponentPlayer);
    void endTurn();
    void beginTurnDrawPhase();
    void requirePhase(GamePhase expected) const;

    Deck deck_;
    std::array<Hand, kPlayerCount> hands_{};
    GamePhase phase_{GamePhase::NotStarted};
    PlayerId current_{PlayerId::Player0};
    std::optional<Card> pending_draw_;
    std::optional<PlayerId> winner_;
};

}  // namespace dvcode
