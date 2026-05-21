#include "core/Hand.hpp"

#include <algorithm>
#include <stdexcept>

namespace dvcode {

namespace {

bool violatesAdjacent(const Card& left, const Card& right) noexcept {
    const auto ord = Card::compareStrict(left, right);
    return ord.has_value() && (*ord == std::strong_ordering::greater);
}

/// 回溯：为每个 Joker 尝试赋值，判断能否满足全手有序
bool canAssignJokers(const std::vector<Card>& hand, std::size_t index) {
    if (index == hand.size()) {
        return true;
    }
    if (!hand[index].isJoker()) {
        if (index > 0) {
            const auto ord = Card::compareStrict(hand[index - 1], hand[index]);
            if (ord.has_value() && *ord == std::strong_ordering::greater) {
                return false;
            }
        }
        return canAssignJokers(hand, index + 1);
    }

    for (int v = Card::kMinValue; v <= Card::kMaxValue; ++v) {
        for (auto color : {CardColor::Black, CardColor::White}) {
            Card trial{v, color, hand[index].isRevealed()};
            if (index > 0) {
                const auto leftOrd = Card::compareStrict(hand[index - 1], trial);
                if (leftOrd.has_value() && *leftOrd == std::strong_ordering::greater) {
                    continue;
                }
            }
            auto nextHand = hand;
            nextHand[index] = trial;
            if (canAssignJokers(nextHand, index + 1)) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

std::size_t Hand::hiddenCount() const noexcept {
    return static_cast<std::size_t>(
        std::count_if(cards_.begin(), cards_.end(),
                      [](const Card& c) { return !c.isRevealed(); }));
}

std::optional<std::size_t> Hand::findSortedInsertIndex(const std::vector<Card>& hand,
                                                       const Card& card) {
    if (card.isJoker()) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i <= hand.size(); ++i) {
        const bool okLeft =
            (i == 0) ||
            (Card::compareStrict(hand[i - 1], card).value_or(
                 std::strong_ordering::less) != std::strong_ordering::greater);
        const bool okRight =
            (i == hand.size()) ||
            (Card::compareStrict(card, hand[i]).value_or(
                 std::strong_ordering::less) != std::strong_ordering::greater);
        if (okLeft && okRight) {
            return i;
        }
    }
    return std::nullopt;
}

void Hand::insertSorted(const Card& card) {
    if (card.isJoker()) {
        throw std::invalid_argument("use insertAt for Joker");
    }
    const auto pos = findSortedInsertIndex(cards_, card);
    if (!pos) {
        throw std::runtime_error("no valid sorted position for card");
    }
    cards_.insert(cards_.begin() + static_cast<std::ptrdiff_t>(*pos), card);
}

void Hand::insertAt(std::size_t index, const Card& card) {
    if (index > cards_.size()) {
        throw std::out_of_range("insert index out of range");
    }
    cards_.insert(cards_.begin() + static_cast<std::ptrdiff_t>(index), card);
    if (!isValidOrder()) {
        cards_.erase(cards_.begin() + static_cast<std::ptrdiff_t>(index));
        throw std::runtime_error("insert would break hand order");
    }
}

bool Hand::isHidden(std::size_t index) const noexcept {
    if (index >= cards_.size()) {
        return false;
    }
    return !cards_[index].isRevealed();
}

void Hand::revealAt(std::size_t index) {
    if (index >= cards_.size()) {
        throw std::out_of_range("reveal index out of range");
    }
    cards_[index].setRevealed(true);
}

bool Hand::isGuessCorrect(std::size_t index, int value, CardColor color) const noexcept {
    if (index >= cards_.size() || cards_[index].isRevealed()) {
        return false;
    }
    const Card& target = cards_[index];
    if (!target.isJoker()) {
        return target.value() == value && target.color() == color;
    }
    auto trialHand = cards_;
    trialHand[index] = Card{value, color, false};
    for (std::size_t i = 1; i < trialHand.size(); ++i) {
        if (violatesAdjacent(trialHand[i - 1], trialHand[i])) {
            return false;
        }
    }
    return canAssignJokers(trialHand, 0);
}

bool Hand::isValidOrder() const noexcept {
    for (std::size_t i = 1; i < cards_.size(); ++i) {
        if (violatesAdjacent(cards_[i - 1], cards_[i])) {
            return false;
        }
    }
    if (std::any_of(cards_.begin(), cards_.end(),
                    [](const Card& c) { return c.isJoker(); })) {
        return canAssignJokers(cards_, 0);
    }
    return true;
}

}  // namespace dvcode
