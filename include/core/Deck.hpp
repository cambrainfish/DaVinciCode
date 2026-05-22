#pragma once

#include "core/Card.hpp"

#include <cstddef>
#include <vector>

namespace dvcode {

/// 黑白两堆牌：摸牌时由玩家选择颜色，再从该堆随机抽一张
class Deck {
public:
    static constexpr std::size_t kStandardDeckSize = 26;
    static constexpr std::size_t kCardsPerPile = 13;  // 0–11 + 1 Joker
    static constexpr std::size_t kInitialDeal = 4;

    Deck();

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool empty(CardColor color) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t size(CardColor color) const noexcept;

    void buildStandard();
    void shuffle();

    /// 从指定颜色牌堆顶抽一张（调用前须确认该堆非空）
    Card draw(CardColor color);

    /// 开局发牌：每次从非空牌堆中随机选一堆抽取
    std::vector<Card> deal(std::size_t count);

private:
    std::vector<Card>& pile(CardColor color);
    const std::vector<Card>& pile(CardColor color) const;

    std::vector<Card> black_pile_;
    std::vector<Card> white_pile_;
};

}  // namespace dvcode
