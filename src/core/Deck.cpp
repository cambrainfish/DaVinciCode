#include "core/Deck.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>

namespace dvcode {

Deck::Deck() {
    buildStandard();
}

std::vector<Card>& Deck::pile(CardColor color) {
    return color == CardColor::Black ? black_pile_ : white_pile_;
}

const std::vector<Card>& Deck::pile(CardColor color) const {
    return color == CardColor::Black ? black_pile_ : white_pile_;
}

bool Deck::empty() const noexcept {
    return black_pile_.empty() && white_pile_.empty();
}

bool Deck::empty(CardColor color) const noexcept {
    return pile(color).empty();
}

std::size_t Deck::size() const noexcept {
    return black_pile_.size() + white_pile_.size();
}

std::size_t Deck::size(CardColor color) const noexcept {
    return pile(color).size();
}

void Deck::buildStandard() {
    black_pile_.clear();
    white_pile_.clear();
    black_pile_.reserve(kCardsPerPile);
    white_pile_.reserve(kCardsPerPile);

    for (int v = Card::kMinValue; v <= Card::kMaxValue; ++v) {
        black_pile_.emplace_back(v, CardColor::Black);
        white_pile_.emplace_back(v, CardColor::White);
    }
    black_pile_.push_back(Card::makeJoker());
    white_pile_.push_back(Card::makeJoker());

    if (black_pile_.size() != kCardsPerPile || white_pile_.size() != kCardsPerPile ||
        size() != kStandardDeckSize) {
        throw std::logic_error("standard deck size mismatch");
    }
}

void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::ranges::shuffle(black_pile_, gen);
    std::ranges::shuffle(white_pile_, gen);
}

Card Deck::draw(CardColor color) {
    auto& p = pile(color);
    if (p.empty()) {
        throw std::runtime_error("draw from empty pile");
    }
    Card top = p.back();
    p.pop_back();
    return top;
}

std::vector<Card> Deck::deal(std::size_t count) {
    if (count > size()) {
        throw std::runtime_error("not enough cards to deal");
    }
    std::vector<Card> dealt;
    dealt.reserve(count);

    std::random_device rd;
    std::mt19937 gen{rd()};

    for (std::size_t i = 0; i < count; ++i) {
        std::vector<CardColor> available;
        if (!black_pile_.empty()) {
            available.push_back(CardColor::Black);
        }
        if (!white_pile_.empty()) {
            available.push_back(CardColor::White);
        }
        if (available.empty()) {
            throw std::runtime_error("deal interrupted: piles empty");
        }
        std::uniform_int_distribution<std::size_t> dist(0, available.size() - 1);
        dealt.push_back(draw(available[dist(gen)]));
    }
    return dealt;
}

}  // namespace dvcode
