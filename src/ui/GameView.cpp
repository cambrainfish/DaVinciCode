#include "ui/GameView.hpp"

#include <graphics.h>

#include <chrono>
#include <cstdio>
#include <cwchar>
#include <random>
#include <thread>

namespace dvcode::ui {

namespace {

constexpr COLORREF kColorBg = RGB(28, 36, 48);
constexpr COLORREF kColorPanel = RGB(45, 55, 72);
constexpr COLORREF kColorBlackCard = RGB(35, 40, 50);
constexpr COLORREF kColorWhiteCard = RGB(235, 238, 245);
constexpr COLORREF kColorHidden = RGB(55, 90, 140);
constexpr COLORREF kColorJoker = RGB(120, 70, 160);
constexpr COLORREF kColorHighlight = RGB(255, 200, 60);
constexpr COLORREF kColorBtn = RGB(70, 110, 170);
constexpr COLORREF kColorBtnDisabled = RGB(80, 80, 90);

void setFont(int size) {
    settextstyle(size, 0, L"微软雅黑");
}

void fillRoundButton(int x, int y, int w, int h, COLORREF fill, bool enabled) {
    setfillcolor(enabled ? fill : kColorBtnDisabled);
    solidroundrect(x, y, x + w, y + h, 10, 10);
}

}  // namespace

GameView::GameView() {
    initgraph(kWindowWidth, kWindowHeight);
    setbkcolor(kColorBg);
    setbkmode(TRANSPARENT);
    BeginBatchDraw();
    newGame();
}

void GameView::newGame() {
    game_.startNewGame();
    selected_opp_index_.reset();
    selected_own_index_.reset();
    guess_value_ = 0;
    guess_color_ = CardColor::Black;
    ai_busy_ = false;
    setStatus(L"新对局：你是 P0（下方），电脑 P1（上方）");
}

void GameView::setStatus(const std::wstring& text) {
    status_line_ = text;
}

std::wstring GameView::phaseText() const {
    switch (game_.phase()) {
        case GamePhase::NotStarted:
            return L"未开始";
        case GamePhase::TurnDrawRequired:
            return L"须先摸牌";
        case GamePhase::TurnGuess:
            return L"猜对手暗牌";
        case GamePhase::TurnAfterCorrectGuess:
            return L"猜对：继续或结束";
        case GamePhase::TurnPlacePendingHidden:
            return L"点击己方插位（暗牌）";
        case GamePhase::TurnPlacePendingRevealed:
            return L"点击己方插位（亮牌）";
        case GamePhase::TurnRevealPenalty:
            return L"点击翻开己方一张暗牌";
        case GamePhase::GameOver:
            return L"游戏结束";
    }
    return L"";
}

std::wstring GameView::cardText(const Card& c, PlayerId owner) const {
    const bool showFace = (owner == human_) || c.isRevealed();
    if (!showFace) {
        return L"?";
    }
    if (c.isJoker()) {
        return L"Jk";
    }
    wchar_t buf[16];
    std::swprintf(buf, 16, L"%d%c", c.value(), c.color() == CardColor::Black ? L'B' : L'W');
    return buf;
}

void GameView::layoutSlots() {
    slots_.clear();
    constexpr int cardW = 72;
    constexpr int cardH = 104;
    constexpr int gap = 8;
    constexpr int margin = 40;

    for (int pi = 0; pi < 2; ++pi) {
        const auto pid = static_cast<PlayerId>(pi);
        const auto& hand = game_.hand(pid);
        const int totalW =
            static_cast<int>(hand.size()) * cardW + (hand.empty() ? 0 : static_cast<int>(hand.size() - 1) * gap);
        const int startX = (kWindowWidth - totalW) / 2;
        const int y = (pi == 0) ? kWindowHeight - margin - cardH : margin;
        for (std::size_t i = 0; i < hand.size(); ++i) {
            const int x = startX + static_cast<int>(i) * (cardW + gap);
            slots_.push_back({x, y, cardW, cardH, pid, i});
        }
    }

    if (game_.hasPendingDraw() && game_.currentPlayer() == human_) {
        slots_.push_back({kWindowWidth / 2 - 36, kWindowHeight / 2 - 52, cardW, cardH, human_, 999});
    }
}

void GameView::layoutButtons() {
    buttons_.clear();
    constexpr int bh = 36;
    constexpr int bw = 100;
    constexpr int gap = 12;
    int x = 40;
    const int y = kWindowHeight / 2 - 20;

    const bool humanTurn = game_.currentPlayer() == human_ && !ai_busy_;
    const bool over = game_.phase() == GamePhase::GameOver;

    auto add = [&](BtnId id, const wchar_t* label, bool enabled) {
        buttons_.push_back({x, y, bw, bh, static_cast<int>(id), label});
        x += bw + gap;
        (void)enabled;
    };

    if (over) {
        add(BtnId::Restart, L"再来一局", true);
        return;
    }

    if (!humanTurn) {
        return;
    }

    switch (game_.phase()) {
        case GamePhase::TurnDrawRequired:
            add(BtnId::DrawBlack, L"摸黑堆", game_.canDrawFrom(CardColor::Black));
            add(BtnId::DrawWhite, L"摸白堆", game_.canDrawFrom(CardColor::White));
            break;
        case GamePhase::TurnGuess:
            add(BtnId::ValueDown, L"数-", true);
            add(BtnId::ValueUp, L"数+", true);
            add(BtnId::ToggleColor, L"黑白", true);
            add(BtnId::GuessConfirm, L"确认猜", selected_opp_index_.has_value());
            break;
        case GamePhase::TurnAfterCorrectGuess:
            add(BtnId::Continue, L"继续猜", game_.canContinueGuessing());
            add(BtnId::Stop, L"结束猜", game_.canStopGuessing());
            break;
        default:
            break;
    }
}

bool GameView::hit(const CardSlot& s, int x, int y) const noexcept {
    return x >= s.x && x <= s.x + s.w && y >= s.y && y <= s.y + s.h;
}

bool GameView::hit(const Button& b, int x, int y) const noexcept {
    return x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h;
}

void GameView::render() {
    cleardevice();

    setfillcolor(kColorPanel);
    solidroundrect(20, 60, kWindowWidth - 20, 130, 12, 12);

    setFont(22);
    settextcolor(WHITE);
    outtextxy(30, 20, L"达芬奇密码");

    setFont(18);
    std::wstring header = L"阶段: ";
    header += phaseText();
    header += L"  |  当前: P";
    header += std::to_wstring(static_cast<int>(game_.currentPlayer()));
    header += L"  |  黑:";
    header += std::to_wstring(game_.deck().size(CardColor::Black));
    header += L" 白:";
    header += std::to_wstring(game_.deck().size(CardColor::White));
    if (game_.hasPendingDraw()) {
        header += L"  |  Pending: ";
        header += cardText(game_.pendingDraw(), game_.currentPlayer());
    }
    outtextxy(30, 75, header.c_str());
    outtextxy(30, 105, status_line_.c_str());

    if (game_.hasPendingDraw() && game_.currentPlayer() == human_) {
        setFont(16);
        outtextxy(kWindowWidth / 2 - 80, kWindowHeight / 2 - 80, L"待插入");
    }

    layoutSlots();
    for (const CardSlot& s : slots_) {
        if (s.index == 999) {
            setfillcolor(kColorJoker);
            solidroundrect(s.x, s.y, s.x + s.w, s.y + s.h, 8, 8);
            settextcolor(WHITE);
            setFont(18);
            outtextxy(s.x + 12, s.y + 40, cardText(game_.pendingDraw(), human_).c_str());
            continue;
        }

        const Card& c = game_.hand(s.owner).at(s.index);
        COLORREF fill = kColorHidden;
        if (s.owner == human_ || c.isRevealed()) {
            if (c.isJoker()) {
                fill = kColorJoker;
            } else if (c.color() == CardColor::Black) {
                fill = kColorBlackCard;
            } else {
                fill = kColorWhiteCard;
            }
        }

        const bool selected =
            (selected_opp_index_ && s.owner != human_ && s.index == *selected_opp_index_) ||
            (selected_own_index_ && s.owner == human_ && s.index == *selected_own_index_);

        if (selected) {
            setlinecolor(kColorHighlight);
            setlinestyle(PS_SOLID, 3);
        } else {
            setlinestyle(PS_SOLID, 1);
            setlinecolor(RGB(120, 120, 130));
        }

        setfillcolor(fill);
        solidroundrect(s.x, s.y, s.x + s.w, s.y + s.h, 8, 8);
        setlinestyle(PS_SOLID, 1);

        setFont(18);
        if (fill == kColorWhiteCard) {
            settextcolor(BLACK);
        } else {
            settextcolor(WHITE);
        }
        const std::wstring label = cardText(c, s.owner);
        outtextxy(s.x + 16, s.y + 42, label.c_str());

        setFont(14);
        settextcolor(RGB(180, 180, 190));
        wchar_t idx[8];
        std::swprintf(idx, 8, L"#%zu", s.index);
        outtextxy(s.x + 4, s.y + 4, idx);
    }

    if (game_.phase() == GamePhase::TurnGuess && game_.currentPlayer() == human_) {
        setFont(18);
        settextcolor(WHITE);
        wchar_t guess[32];
        std::swprintf(guess, 32, L"猜测: %d %s", guess_value_,
                      guess_color_ == CardColor::Black ? L"黑" : L"白");
        outtextxy(kWindowWidth - 220, kWindowHeight / 2 - 10, guess);
    }

    layoutButtons();
    for (const Button& b : buttons_) {
        const bool enabled = true;
        fillRoundButton(b.x, b.y, b.w, b.h, kColorBtn, enabled);
        setFont(16);
        settextcolor(WHITE);
        const int tw = textwidth(b.label);
        outtextxy(b.x + (b.w - tw) / 2, b.y + 8, b.label);
    }

    if (game_.phase() == GamePhase::GameOver && game_.winner()) {
        setFont(28);
        settextcolor(kColorHighlight);
        std::wstring win = L"胜者: P";
        win += std::to_wstring(static_cast<int>(*game_.winner()));
        win += (*game_.winner() == human_) ? L"（你赢了！）" : L"（电脑赢了）";
        outtextxy(kWindowWidth / 2 - 120, kWindowHeight / 2 + 40, win.c_str());
    }

    FlushBatchDraw();
}

void GameView::tryGuess() {
    if (!selected_opp_index_ || !game_.canGuess()) {
        return;
    }
    const PlayerId target = PlayerId::Player1;
    try {
        const auto outcome =
            game_.guess(target, *selected_opp_index_, guess_value_, guess_color_);
        selected_opp_index_.reset();
        if (outcome == GuessOutcome::Correct) {
            setStatus(L"猜对了！");
        } else {
            setStatus(L"猜错了。");
        }
    } catch (const std::exception&) {
        setStatus(L"猜牌失败（位置或阶段不对）");
    }
}

void GameView::tryPlacePending(std::size_t index) {
    try {
        if (game_.canPlacePendingHidden()) {
            game_.placePendingHidden(index);
            setStatus(L"已暗牌插入，回合结束。");
        } else if (game_.canPlacePendingRevealed()) {
            game_.placePendingRevealed(index);
            setStatus(L"已亮牌插入，请选择惩罚翻开。");
        }
        selected_own_index_.reset();
    } catch (const std::exception&) {
        setStatus(L"插入失败");
    }
}

void GameView::tryPenalty(std::size_t index) {
    try {
        game_.applyPenaltyReveal(index);
        setStatus(L"惩罚翻开完成。");
        selected_own_index_.reset();
    } catch (const std::exception&) {
        setStatus(L"惩罚翻开失败");
    }
}

void GameView::handleButton(BtnId id) {
    switch (id) {
        case BtnId::DrawBlack:
            if (game_.canDrawFrom(CardColor::Black)) {
                const Card c = game_.draw(CardColor::Black);
                setStatus(std::wstring(L"黑堆摸到 ") + cardText(c, human_) + L"，请猜牌");
            }
            break;
        case BtnId::DrawWhite:
            if (game_.canDrawFrom(CardColor::White)) {
                const Card c = game_.draw(CardColor::White);
                setStatus(std::wstring(L"白堆摸到 ") + cardText(c, human_) + L"，请猜牌");
            }
            break;
        case BtnId::ValueDown:
            guess_value_ = (guess_value_ + Card::kMaxValue) % (Card::kMaxValue + 1);
            break;
        case BtnId::ValueUp:
            guess_value_ = (guess_value_ + 1) % (Card::kMaxValue + 1);
            break;
        case BtnId::ToggleColor:
            guess_color_ = guess_color_ == CardColor::Black ? CardColor::White : CardColor::Black;
            break;
        case BtnId::GuessConfirm:
            tryGuess();
            break;
        case BtnId::Continue:
            if (game_.canContinueGuessing()) {
                game_.continueGuessing();
                setStatus(L"继续猜牌");
            }
            break;
        case BtnId::Stop:
            if (game_.canStopGuessing()) {
                game_.stopGuessingAndEndTurn();
                setStatus(L"结束猜牌阶段");
            }
            break;
        case BtnId::Restart:
            newGame();
            break;
    }
}

void GameView::handleMouse(int x, int y) {
    if (game_.phase() == GamePhase::GameOver) {
        for (const Button& b : buttons_) {
            if (hit(b, x, y) && b.id == static_cast<int>(BtnId::Restart)) {
                handleButton(BtnId::Restart);
            }
        }
        return;
    }

    if (game_.currentPlayer() != human_ || ai_busy_) {
        return;
    }

    for (const Button& b : buttons_) {
        if (hit(b, x, y)) {
            handleButton(static_cast<BtnId>(b.id));
            return;
        }
    }

    for (const CardSlot& s : slots_) {
        if (!hit(s, x, y) || s.index == 999) {
            continue;
        }

        if (game_.phase() == GamePhase::TurnGuess && s.owner != human_ &&
            game_.hand(s.owner).isHidden(s.index)) {
            selected_opp_index_ = s.index;
            setStatus(L"已选对手第 " + std::to_wstring(s.index) + L" 张，调整数值后点确认猜");
            return;
        }

        if ((game_.canPlacePendingHidden() || game_.canPlacePendingRevealed()) &&
            s.owner == human_) {
            tryPlacePending(s.index);
            return;
        }

        if (game_.canRevealPenalty() && s.owner == human_ &&
            game_.hand(human_).isHidden(s.index)) {
            tryPenalty(s.index);
            return;
        }
    }
}

void GameView::runAiTurn() {
    if (game_.phase() == GamePhase::GameOver || game_.currentPlayer() == human_) {
        ai_busy_ = false;
        return;
    }

    ai_busy_ = true;
    static std::mt19937 rng{std::random_device{}()};

    std::this_thread::sleep_for(std::chrono::milliseconds(450));

    try {
        if (game_.mustDrawBeforeGuess()) {
            static std::mt19937 rng{std::random_device{}()};
            std::vector<CardColor> piles;
            if (game_.canDrawFrom(CardColor::Black)) {
                piles.push_back(CardColor::Black);
            }
            if (game_.canDrawFrom(CardColor::White)) {
                piles.push_back(CardColor::White);
            }
            std::uniform_int_distribution<std::size_t> dist(0, piles.size() - 1);
            const CardColor pile = piles[dist(rng)];
            const Card c = game_.draw(pile);
            (void)c;
            setStatus(L"电脑已摸牌");
        } else if (game_.canGuess()) {
            const PlayerId target = human_;
            const auto& hand = game_.hand(target);
            std::vector<std::size_t> hidden;
            for (std::size_t i = 0; i < hand.size(); ++i) {
                if (hand.isHidden(i)) {
                    hidden.push_back(i);
                }
            }
            if (!hidden.empty()) {
                std::uniform_int_distribution<std::size_t> distIdx(0, hidden.size() - 1);
                std::uniform_int_distribution<int> distVal(Card::kMinValue, Card::kMaxValue);
                const std::size_t idx = hidden[distIdx(rng)];
                const int val = distVal(rng);
                const auto color =
                    (rng() % 2 == 0) ? CardColor::Black : CardColor::White;
                const auto outcome = game_.guess(target, idx, val, color);
                setStatus(outcome == GuessOutcome::Correct ? L"电脑猜对" : L"电脑猜错");
            }
        } else if (game_.canContinueGuessing()) {
            if (rng() % 2 == 0) {
                game_.continueGuessing();
            } else {
                game_.stopGuessingAndEndTurn();
            }
        } else if (game_.canStopGuessing()) {
            game_.stopGuessingAndEndTurn();
        } else if (game_.canPlacePendingHidden() || game_.canPlacePendingRevealed()) {
            const auto& mine = game_.hand(PlayerId::Player1);
            bool placed = false;
            for (std::size_t i = 0; i <= mine.size(); ++i) {
                try {
                    if (game_.canPlacePendingHidden()) {
                        game_.placePendingHidden(i);
                    } else {
                        game_.placePendingRevealed(i);
                    }
                    placed = true;
                    break;
                } catch (...) {
                }
            }
            if (!placed) {
                setStatus(L"电脑无法插入 pending");
            }
        } else if (game_.canRevealPenalty()) {
            const auto& mine = game_.hand(PlayerId::Player1);
            for (std::size_t i = 0; i < mine.size(); ++i) {
                if (mine.isHidden(i)) {
                    game_.applyPenaltyReveal(i);
                    break;
                }
            }
        }
    } catch (...) {
        setStatus(L"电脑回合异常");
    }

    ai_busy_ = false;
}

void GameView::run() {
    ExMessage msg{};
    for (;;) {
        runAiTurn();
        render();

        while (peekmessage(&msg, EX_MOUSE, true)) {
            if (msg.message == WM_LBUTTONDOWN) {
                handleMouse(static_cast<int>(msg.x), static_cast<int>(msg.y));
            }
        }

        if (_kbhit() && _getch() == 27) {
            break;
        }
        Sleep(16);
    }

    EndBatchDraw();
    closegraph();
}

}  // namespace dvcode::ui
