#include "core/Deck.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>

namespace dvcode {

Deck::Deck() {
    buildStandard();
}

void Deck::buildStandard() {
    cards_.clear();
    cards_.reserve(kStandardDeckSize);

    for (int v = Card::kMinValue; v <= Card::kMaxValue; ++v) {
        cards_.emplace_back(v, CardColor::Black);
        cards_.emplace_back(v, CardColor::White);
    }
    cards_.push_back(Card::makeJoker());
    cards_.push_back(Card::makeJoker());

    if (cards_.size() != kStandardDeckSize) {
        throw std::logic_error("standard deck size mismatch");
    }
}

void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::ranges::shuffle(cards_, gen);
}

Card Deck::draw() {
    if (cards_.empty()) {
        throw std::runtime_error("draw from empty deck");
    }
    Card top = cards_.back();
    cards_.pop_back();
    return top;
}

std::vector<Card> Deck::deal(std::size_t count) {
    if (count > cards_.size()) {
        throw std::runtime_error("not enough cards to deal");
    }
    std::vector<Card> dealt;
    dealt.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        dealt.push_back(draw());
    }
    return dealt;
}

}  // namespace dvcode
