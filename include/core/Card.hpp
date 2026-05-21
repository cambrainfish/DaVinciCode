#pragma once

#include <compare>
#include <optional>
#include <string>

namespace dvcode {

enum class CardColor { Black, White };

/// 单张牌：0–11 数字牌或 Joker（wild）
class Card {
public:
    static constexpr int kJokerValue = -1;
    static constexpr int kMinValue = 0;
    static constexpr int kMaxValue = 11;

    Card() = default;
    Card(int value, CardColor color, bool revealed = false);
    static Card makeJoker(bool revealed = false);

    [[nodiscard]] bool isJoker() const noexcept { return is_joker_; }
    [[nodiscard]] int value() const noexcept { return value_; }
    [[nodiscard]] CardColor color() const noexcept { return color_; }
    [[nodiscard]] bool isRevealed() const noexcept { return revealed_; }

    void setRevealed(bool revealed) noexcept { revealed_ = revealed; }

    /// 严格排序：数值升序；同数 Black < White。不含 Joker 的比较。
    [[nodiscard]] static std::optional<std::strong_ordering>
    compareStrict(const Card& a, const Card& b) noexcept;

    [[nodiscard]] std::string toString() const;

private:
    int value_{0};
    CardColor color_{CardColor::Black};
    bool is_joker_{false};
    bool revealed_{false};
};

}  // namespace dvcode
