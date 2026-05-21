#pragma once

#include "core/Card.hpp"

#include <cstddef>
#include <vector>

namespace dvcode {

/// 牌堆：26 张标准牌组，洗牌与发牌
class Deck {
public:
    static constexpr std::size_t kStandardDeckSize = 26;
    static constexpr std::size_t kInitialDeal = 4;

    Deck();

    [[nodiscard]] bool empty() const noexcept { return cards_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return cards_.size(); }

    void buildStandard();
    void shuffle();
    Card draw();
    std::vector<Card> deal(std::size_t count);

private:
    std::vector<Card> cards_;
};

}  // namespace dvcode
