#include "core/GameManager.hpp"

#include <stdexcept>

namespace dvcode {

namespace {

void dealCardIntoHand(Hand& hand, Card card) {
    card.setRevealed(false);
    if (card.isJoker()) {
        for (std::size_t i = 0; i <= hand.size(); ++i) {
            try {
                hand.insertAt(i, card);
                return;
            } catch (...) {
            }
        }
        throw std::runtime_error("cannot place joker in initial hand");
    }
    hand.insertSorted(card);
}

}  // namespace

void GameManager::startNewGame() {
    deck_ = Deck{};
    deck_.shuffle();
    hands_[0].clear();
    hands_[1].clear();
    pending_draw_.reset();
    winner_.reset();
    current_ = PlayerId::Player0;
    phase_ = GamePhase::NotStarted;

    dealToPlayer(PlayerId::Player0);
    dealToPlayer(PlayerId::Player1);

    phase_ = GamePhase::TurnDrawChoice;
}

PlayerId GameManager::opponent(PlayerId p) const noexcept {
    return p == PlayerId::Player0 ? PlayerId::Player1 : PlayerId::Player0;
}

void GameManager::dealToPlayer(PlayerId p) {
    for (Card c : deck_.deal(Deck::kInitialDeal)) {
        dealCardIntoHand(hands_[toIndex(p)], c);
    }
}

void GameManager::requirePhase(GamePhase expected) const {
    if (phase_ != expected) {
        throw std::logic_error("invalid game phase for action");
    }
}

bool GameManager::canDraw() const noexcept {
    return phase_ == GamePhase::TurnDrawChoice && !deck_.empty();
}

bool GameManager::canSkipDraw() const noexcept {
    return phase_ == GamePhase::TurnDrawChoice;
}

bool GameManager::canPlacePendingHidden() const noexcept {
    return phase_ == GamePhase::TurnPlacePendingHidden && pending_draw_.has_value();
}

bool GameManager::canPlacePendingRevealed() const noexcept {
    return phase_ == GamePhase::TurnPlacePendingRevealed && pending_draw_.has_value();
}

bool GameManager::canGuess() const noexcept {
    return phase_ == GamePhase::TurnGuess || phase_ == GamePhase::TurnAfterCorrectGuess;
}

bool GameManager::canContinueGuessing() const noexcept {
    return phase_ == GamePhase::TurnAfterCorrectGuess;
}

bool GameManager::canStopGuessing() const noexcept {
    return phase_ == GamePhase::TurnAfterCorrectGuess;
}

bool GameManager::canRevealPenalty() const noexcept {
    return phase_ == GamePhase::TurnRevealPenalty;
}

Card GameManager::draw() {
    requirePhase(GamePhase::TurnDrawChoice);
    if (deck_.empty()) {
        throw std::logic_error("cannot draw from empty deck");
    }
    Card c = deck_.draw();
    c.setRevealed(false);
    pending_draw_ = c;
    phase_ = GamePhase::TurnGuess;
    return c;
}

void GameManager::skipDraw() {
    requirePhase(GamePhase::TurnDrawChoice);
    phase_ = GamePhase::TurnGuess;
}

void GameManager::insertPendingInto(std::size_t index, bool reveal) {
    if (!pending_draw_.has_value()) {
        throw std::logic_error("no pending card to place");
    }
    Card c = *pending_draw_;
    c.setRevealed(reveal);
    Hand& mine = hands_[toIndex(current_)];
    mine.insertAt(index, c);
    pending_draw_.reset();
}

void GameManager::placePendingHidden(std::size_t index) {
    requirePhase(GamePhase::TurnPlacePendingHidden);
    insertPendingInto(index, false);
    endTurn();
}

void GameManager::placePendingRevealed(std::size_t index) {
    requirePhase(GamePhase::TurnPlacePendingRevealed);
    insertPendingInto(index, true);
    phase_ = GamePhase::TurnRevealPenalty;
}

GuessOutcome GameManager::guess(PlayerId target, std::size_t index, int value,
                                CardColor color) {
    if (!canGuess()) {
        throw std::logic_error("cannot guess in current phase");
    }
    if (target == current_) {
        throw std::invalid_argument("cannot guess your own hand");
    }

    Hand& oppHand = hands_[toIndex(target)];
    if (!oppHand.isHidden(index)) {
        throw std::invalid_argument("target card is already revealed");
    }

    const bool correct = oppHand.isGuessCorrect(index, value, color);
    if (correct) {
        oppHand.revealAt(index);
        checkWinAfterOpponentReveal(target);
        if (phase_ == GamePhase::GameOver) {
            return GuessOutcome::Correct;
        }
        phase_ = GamePhase::TurnAfterCorrectGuess;
        return GuessOutcome::Correct;
    }

    if (pending_draw_.has_value()) {
        phase_ = GamePhase::TurnPlacePendingRevealed;
    } else {
        phase_ = GamePhase::TurnRevealPenalty;
    }
    return GuessOutcome::Wrong;
}

void GameManager::checkWinAfterOpponentReveal(PlayerId opponentPlayer) {
    if (hands_[toIndex(opponentPlayer)].hiddenCount() == 0) {
        winner_ = current_;
        phase_ = GamePhase::GameOver;
    }
}

void GameManager::continueGuessing() {
    requirePhase(GamePhase::TurnAfterCorrectGuess);
    phase_ = GamePhase::TurnGuess;
}

void GameManager::stopGuessingAndEndTurn() {
    requirePhase(GamePhase::TurnAfterCorrectGuess);
    if (pending_draw_.has_value()) {
        phase_ = GamePhase::TurnPlacePendingHidden;
        return;
    }
    endTurn();
}

void GameManager::applyPenaltyReveal(std::size_t ownIndex) {
    requirePhase(GamePhase::TurnRevealPenalty);
    Hand& mine = hands_[toIndex(current_)];
    if (!mine.isHidden(ownIndex)) {
        throw std::invalid_argument("penalty target must be hidden");
    }
    mine.revealAt(ownIndex);
    endTurn();
}

void GameManager::endTurn() {
    if (phase_ == GamePhase::GameOver) {
        return;
    }
    if (pending_draw_.has_value()) {
        throw std::logic_error("cannot end turn with pending draw");
    }
    current_ = opponent(current_);
    phase_ = GamePhase::TurnDrawChoice;
}

}  // namespace dvcode
