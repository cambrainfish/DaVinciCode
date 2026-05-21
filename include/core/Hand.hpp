#pragma once

#include "core/Card.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace dvcode {

/// 玩家手牌：保持与游戏规则一致的有序性
class Hand {
public:
    [[nodiscard]] const std::vector<Card>& cards() const noexcept { return cards_; }
    [[nodiscard]] std::size_t size() const noexcept { return cards_.size(); }
    [[nodiscard]] std::size_t hiddenCount() const noexcept;

    [[nodiscard]] const Card& at(std::size_t index) const { return cards_.at(index); }

    /// 非 Joker：按小→大插入（同数 Black < White）
    void insertSorted(const Card& card);

    /// Joker：插入到任意位置（由调用方选择下标）
    void insertAt(std::size_t index, const Card& card);

    /// 检查是否存在一种 Joker 赋值使整手牌满足排序规则
    [[nodiscard]] bool isValidOrder() const noexcept;

    [[nodiscard]] bool isHidden(std::size_t index) const noexcept;
    void revealAt(std::size_t index);

    /// 猜牌：仅对未翻开牌有效；Joker 位需与整手排序相容
    [[nodiscard]] bool isGuessCorrect(std::size_t index, int value,
                                      CardColor color) const noexcept;

    void clear() noexcept { cards_.clear(); }

private:
    [[nodiscard]] static std::optional<std::size_t>
    findSortedInsertIndex(const std::vector<Card>& hand, const Card& card);

    std::vector<Card> cards_;
};

}  // namespace dvcode
