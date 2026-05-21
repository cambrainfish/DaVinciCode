#include "core/Card.hpp"

#include <sstream>

namespace dvcode {

Card::Card(int value, CardColor color, bool revealed)
    : value_{value}, color_{color}, is_joker_{false}, revealed_{revealed} {
}

Card Card::makeJoker(bool revealed) {
    Card c;
    c.is_joker_ = true;
    c.value_ = kJokerValue;
    c.revealed_ = revealed;
    return c;
}

std::optional<std::strong_ordering> Card::compareStrict(const Card& a,
                                                        const Card& b) noexcept {
    if (a.isJoker() || b.isJoker()) {
        return std::nullopt;
    }
    if (a.value_ != b.value_) {
        return a.value_ <=> b.value_;
    }
    if (a.color_ == b.color_) {
        return std::strong_ordering::equal;
    }
    if (a.color_ == CardColor::Black && b.color_ == CardColor::White) {
        return std::strong_ordering::less;
    }
    return std::strong_ordering::greater;
}

std::string Card::toString() const {
    if (is_joker_) {
        return revealed_ ? "Joker*" : "Joker";
    }
    std::ostringstream oss;
    oss << value_ << (color_ == CardColor::Black ? 'B' : 'W');
    if (revealed_) {
        oss << '*';
    }
    return oss.str();
}

}  // namespace dvcode
