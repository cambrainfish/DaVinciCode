#include "GameView.hpp"
#include <graphics.h>
#include <cstdlib>
#include <ctime>

namespace dvcode::ui {

    // 显式引入核心命名空间，根除所有关于 PlayerId、GamePhase、CardColor 等类型的未知标识符报错
    using namespace dvcode;

    GameView::GameView() {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        status_line_ = L"系统初始化完毕。";
    }

    void GameView::run() {
        initgraph(kWindowWidth, kWindowHeight);
        BeginBatchDraw();

        newGame();

        while (true) {
            render();

            // 如果属于 AI 回合，运行内嵌的 AI 决策状态机
            if (game_.currentPlayer() == PlayerId::Player1 && game_.phase() != GamePhase::GameOver) {
                runAiTurn();
                continue;
            }

            if (MouseHit()) {
                MOUSEMSG msg = GetMouseMsg();
                if (msg.uMsg == WM_LBUTTONDOWN) {
                    handleMouse(msg.x, msg.y);
                }
            }
            Sleep(15);
        }

        EndBatchDraw();
        closegraph();
    }

    void GameView::newGame() {
        game_.startNewGame();
        selected_opp_index_.reset();
        selected_own_index_.reset();
        guess_value_ = 0;
        guess_color_ = CardColor::Black;
        status_line_ = L"新游戏开始！请点击左侧面板【摸黑牌】或【摸白牌】。";
        layoutSlots();
        layoutButtons();
    }

    void GameView::layoutSlots() {
        slots_.clear();

        // 电脑 AI (Player 1) 手牌槽
        const Hand& aiHand = game_.hand(PlayerId::Player1);
        int aiTotalW = static_cast<int>(aiHand.size()) * 70 + (static_cast<int>(aiHand.size()) - 1) * 15;
        int aiStartX = (kWindowWidth - aiTotalW) / 2;
        for (std::size_t i = 0; i < aiHand.size(); ++i) {
            slots_.push_back({ aiStartX + static_cast<int>(i) * 85, 110, 70, 110, PlayerId::Player1, i });
        }

        // 人类玩家 (Player 0) 手牌槽
        const Hand& humHand = game_.hand(PlayerId::Player0);
        int humTotalW = static_cast<int>(humHand.size()) * 70 + (static_cast<int>(humHand.size()) - 1) * 15;
        int humStartX = (kWindowWidth - humTotalW) / 2;
        for (std::size_t i = 0; i < humHand.size(); ++i) {
            slots_.push_back({ humStartX + static_cast<int>(i) * 85, 450, 70, 110, PlayerId::Player0, i });
        }
    }

    void GameView::layoutButtons() {
        buttons_.clear();
        buttons_.push_back({ 100, 310, 110, 40, static_cast<int>(BtnId::DrawBlack), L"摸黑牌" });
        buttons_.push_back({ 100, 365, 110, 40, static_cast<int>(BtnId::DrawWhite), L"摸白牌" });

        buttons_.push_back({ 330, 310, 50, 40, static_cast<int>(BtnId::ValueDown), L"减 -" });
        buttons_.push_back({ 460, 310, 50, 40, static_cast<int>(BtnId::ValueUp), L"加 +" });
        buttons_.push_back({ 330, 365, 180, 40, static_cast<int>(BtnId::ToggleColor), L"切换颜色" });
        buttons_.push_back({ 540, 335, 120, 40, static_cast<int>(BtnId::GuessConfirm), L"宣告猜测" });

        buttons_.push_back({ 720, 310, 110, 40, static_cast<int>(BtnId::Continue), L"继续猜" });
        buttons_.push_back({ 720, 365, 110, 40, static_cast<int>(BtnId::Stop), L"收手结束" });
        buttons_.push_back({ 860, 20, 110, 40, static_cast<int>(BtnId::Restart), L"重新开始" });
    }

    void GameView::render() {
        setbkcolor(RGB(24, 38, 48));
        cleardevice();

        settextcolor(RGB(175, 195, 210));
        settextstyle(22, 0, L"微软雅黑");
        outtextxy(50, 60, L"💻 电脑 AI 的阵地");
        outtextxy(50, 600, L"👨 你的密码防线");

        // 渲染卡牌
        for (const auto& slot : slots_) {
            const Hand& h = game_.hand(slot.owner);
            if (slot.index >= h.size()) continue;
            const Card& card = h.at(slot.index);

            bool isSelected = false;
            if (slot.owner == PlayerId::Player1 && selected_opp_index_ == slot.index) isSelected = true;
            if (slot.owner == PlayerId::Player0 && selected_own_index_ == slot.index) isSelected = true;

            COLORREF bg = (card.color() == CardColor::Black) ? RGB(20, 20, 22) : RGB(242, 242, 245);
            COLORREF txt = (card.color() == CardColor::Black) ? RGB(240, 240, 240) : RGB(20, 20, 20);

            if (isSelected) {
                setlinecolor(RGB(255, 215, 0));
                setlinestyle(PS_SOLID, 4);
            }
            else {
                setlinecolor(RGB(110, 125, 140));
                setlinestyle(PS_SOLID, 1);
            }

            setfillcolor(bg);
            fillrectangle(slot.x, slot.y, slot.x + slot.w, slot.y + slot.h);

            settextcolor(txt);
            settextstyle(28, 0, L"Consolas", 0, 0, FW_BOLD, false, false, false);

            std::wstring text = cardText(card, slot.owner);
            outtextxy(slot.x + (text.size() == 1 ? 26 : 16), slot.y + 40, text.c_str());
        }
        setlinestyle(PS_SOLID, 1);

        // 中间属性行渲染
        settextcolor(RGB(255, 255, 255));
        settextstyle(20, 0, L"微软雅黑");
        std::wstring preview = L"设定猜解数: " + std::to_wstring(guess_value_) +
            L" [" + (guess_color_ == CardColor::Black ? L"黑" : L"白") + L"]";
        outtextxy(340, 275, preview.c_str());

        // 绘制状态按钮
        for (const auto& btn : buttons_) {
            bool active = true;
            GamePhase p = game_.phase();

            if (btn.id == static_cast<int>(BtnId::DrawBlack) || btn.id == static_cast<int>(BtnId::DrawWhite)) {
                active = (p == GamePhase::TurnDrawRequired && game_.currentPlayer() == human_);
            }
            else if (btn.id == static_cast<int>(BtnId::GuessConfirm) || btn.id == static_cast<int>(BtnId::ValueUp) ||
                btn.id == static_cast<int>(BtnId::ValueDown) || btn.id == static_cast<int>(BtnId::ToggleColor)) {
                active = (p == GamePhase::TurnGuess && game_.currentPlayer() == human_ && selected_opp_index_.has_value());
            }
            else if (btn.id == static_cast<int>(BtnId::Continue) || btn.id == static_cast<int>(BtnId::Stop)) {
                active = (p == GamePhase::TurnAfterCorrectGuess && game_.currentPlayer() == human_);
            }

            if (active) {
                setfillcolor(RGB(64, 84, 108));
                settextcolor(RGB(255, 255, 255));
                setlinecolor(RGB(180, 190, 200));
            }
            else {
                setfillcolor(RGB(38, 46, 54));
                settextcolor(RGB(90, 100, 110));
                setlinecolor(RGB(55, 62, 70));
            }
            fillrectangle(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h);
            settextstyle(16, 0, L"微软雅黑");
            outtextxy(btn.x + 20, btn.y + 10, btn.label);
        }

        settextcolor(RGB(115, 230, 140));
        settextstyle(18, 0, L"微软雅黑");
        std::wstring display_status = phaseText() + L" | " + status_line_;
        outtextxy(50, 675, display_status.c_str());

        FlushBatchDraw();
    }

    void GameView::handleMouse(int x, int y) {
        for (const auto& btn : buttons_) {
            if (hit(btn, x, y)) {
                handleButton(static_cast<BtnId>(btn.id));
                return;
            }
        }

        for (const auto& slot : slots_) {
            if (hit(slot, x, y)) {
                if (slot.owner == PlayerId::Player1 && game_.phase() == GamePhase::TurnGuess) {
                    if (game_.hand(PlayerId::Player1).isHidden(slot.index)) {
                        selected_opp_index_ = slot.index;
                        status_line_ = L"锁定了 AI 第 " + std::to_wstring(slot.index) + L" 张牌，请设置下方数字进行猜测。";
                    }
                }
                else if (slot.owner == PlayerId::Player0) {
                    if (game_.phase() == GamePhase::TurnPlacePendingHidden || game_.phase() == GamePhase::TurnPlacePendingRevealed) {
                        tryPlacePending(slot.index);
                    }
                }
                return;
            }
        }

        // 兜底右侧空白区域点击，认定插入最后尾
        if ((game_.phase() == GamePhase::TurnPlacePendingHidden || game_.phase() == GamePhase::TurnPlacePendingRevealed) && y >= 450 && y <= 560) {
            tryPlacePending(game_.hand(PlayerId::Player0).size());
        }
    }

    void GameView::handleButton(BtnId id) {
        if (id == BtnId::Restart) {
            newGame();
            return;
        }
        if (game_.currentPlayer() != human_ || game_.phase() == GamePhase::GameOver) return;

        switch (id) {
        case BtnId::DrawBlack:
            if (game_.canDrawFrom(CardColor::Black)) {
                game_.draw(CardColor::Black);
                status_line = L"摸黑牌成功！请选择上排 AI 一张暗牌并进行猜测。";
                layoutSlots();
            }
            break;
        case BtnId::DrawWhite:
            if (game_.canDrawFrom(CardColor::White)) {
                game_.draw(CardColor::White);
                status_line = L"摸白牌成功！请选择上排 AI 一张暗牌并进行猜测。";
                layoutSlots();
            }
            break;
        case BtnId::ValueDown: if (guess_value_ > 0) guess_value_--; break;
        case BtnId::ValueUp: if (guess_value_ < 11) guess_value_++; break;
        case BtnId::ToggleColor:
            guess_color_ = (guess_color_ == CardColor::Black) ? CardColor::White : CardColor::Black;
            break;
        case BtnId::GuessConfirm:
            tryGuess();
            break;
        case BtnId::Continue:
            if (game_.canContinueGuessing()) {
                game_.continueGuessing();
                selected_opp_index_.reset();
                status_line_ = L"继续连击！选择下一张 AI 的暗牌进行猜测。";
            }
            break;
        case BtnId::Stop:
            if (game_.canStopGuessing()) {
                game_.stopGuessingAndEndTurn();
                status_line_ = L"收手成功。请点击下方你的手牌相应间隙，把刚才抽到的暗牌放好。";
            }
            break;
        default:
            break;
        }
    }

    void GameView::tryGuess() {
        if (!selected_opp_index_.has_value()) return;
        std::size_t idx = *selected_opp_index_;

        GuessOutcome outcome = game_.guess(PlayerId::Player1, idx, guess_value_, guess_color_);
        if (outcome == GuessOutcome::Correct) {
            status_line_ = L"🎉 猜对了！点击【继续猜】乘胜追击，或者点击【收手结束】。";
        }
        else {
            status_line_ = L"❌ 猜错了！请点击你下方手牌位置，把刚才抽到的罚牌【公开】插进去。";
        }
        selected_opp_index_.reset();
        layoutSlots();
    }

    void GameView::tryPlacePending(std::size_t index) {
        try {
            if (game_.phase() == GamePhase::TurnPlacePendingHidden) {
                game_.placePendingHidden(index);
                status_line_ = L"暗牌安全就位，回合切至 AI。";
            }
            else if (game_.phase() == GamePhase::TurnPlacePendingRevealed) {
                tryPenalty(index);
            }
            layoutSlots();
        }
        catch (...) {
            status_line_ = L"⚠️ 位置违反了大小严格递增规则！请重新选择位置。";
        }
    }

    void GameView::tryPenalty(std::size_t index) {
        game_.placePendingRevealed(index);
        status_line_ = L"惩罚明牌已公开摆放。";
    }

    void GameView::runAiTurn() {
        if (game_.phase() == GamePhase::GameOver || game_.currentPlayer() != PlayerId::Player1) return;

        ai_busy_ = true;
        render();
        Sleep(800); // 模拟思考延时

        switch (game_.phase()) {
        case GamePhase::TurnDrawRequired: {
            bool canB = game_.canDrawFrom(CardColor::Black);
            bool canW = game_.canDrawFrom(CardColor::White);
            CardColor color = CardColor::Black;
            if (canB && canW) color = (std::rand() % 2 == 0) ? CardColor::Black : CardColor::White;
            else color = canB ? CardColor::Black : CardColor::White;

            game_.draw(color);
            status_line_ = L"AI 决定摸走了一张【" + std::wstring(color == CardColor::Black ? L"黑" : L"白") + L"】牌。";
            layoutSlots();
            break;
        }
        case GamePhase::TurnGuess: {
            const Hand& humanHand = game_.hand(PlayerId::Player0);
            std::size_t targetIdx = 0;
            bool found = false;
            for (std::size_t i = 0; i < humanHand.size(); ++i) {
                if (humanHand.isHidden(i)) {
                    targetIdx = i;
                    found = true;
                    break;
                }
            }
            if (!found) targetIdx = 0;

            int aiGuessValue = static_cast<int>(3 + targetIdx);
            if (targetIdx == 0) aiGuessValue = 2;
            if (aiGuessValue > 11) aiGuessValue = 11;
            CardColor aiGuessColor = humanHand.at(targetIdx).color();

            status_line = L"AI 锁定了你第 " + std::to_wstring(targetIdx) + L" 个位置的暗牌并发动了破译密码攻击...";
            render();
            Sleep(1000);

            GuessOutcome res = game_.guess(PlayerId::Player0, targetIdx, aiGuessValue, aiGuessColor);
            if (res == GuessOutcome::Correct) {
                status_line_ = L"🔴 警报！AI 精准猜中了你的牌，谜底是 " + std::to_wstring(aiGuessValue) + (aiGuessColor == CardColor::Black ? L"B" : L"W") + L"！";
            }
            else {
                status_line_ = L"🟢 呼！AI 猜错了，避过一劫。";
            }
            layoutSlots();
            break;
        }
        case GamePhase::TurnAfterCorrectGuess: {
            status_line_ = L"AI 决定保守起见，选择收手不再继续连击。";
            render();
            Sleep(800);
            game_.stopGuessingAndEndTurn();
            break;
        }
        case GamePhase::TurnPlacePendingHidden: {
            bool placed = false;
            for (std::size_t i = 0; i <= game_.hand(PlayerId::Player1).size(); ++i) {
                try {
                    GameManager snapshot = game_;
                    snapshot.placePendingHidden(i);
                    game_.placePendingHidden(i);
                    placed = true;
                    status_line_ = L"AI 将暗牌插入了自己的密码排布中。轮到你操作了！";
                    break;
                }
                catch (...) { continue; }
            }
            if (!placed) try { game_.placePendingHidden(0); }
            catch (...) {}
            layoutSlots();
            break;
        }
        case GamePhase::TurnPlacePendingRevealed: {
            bool placed = false;
            for (std::size_t i = 0; i <= game_.hand(PlayerId::Player1).size(); ++i) {
                try {
                    GameManager snapshot = game_;
                    snapshot.placePendingRevealed(i);
                    game_.placePendingRevealed(i);
                    placed = true;
                    status_line_ = L"AI 猜错受罚公开亮牌。开始你的回合！";
                    break;
                }
                catch (...) { continue; }
            }
            if (!placed) try { game_.placePendingRevealed(0); }
            catch (...) {}
            layoutSlots();
            break;
        }
        default:
            break;
        }

        ai_busy_ = false;
    }

    void GameView::setStatus(const std::wstring& text) {
        status_line_ = text;
    }

    bool GameView::hit(const CardSlot& s, int x, int y) const noexcept {
        return x >= s.x && x <= s.x + s.w && y >= s.y && y <= s.y + s.h;
    }

    bool GameView::hit(const Button& b, int x, int y) const noexcept {
        return x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h;
    }

    std::wstring GameView::phaseText() const {
        switch (game_.phase()) {
        case GamePhase::NotStarted: return L"[游戏未开始]";
        case GamePhase::TurnDrawRequired: return L"[需要摸牌]";
        case GamePhase::TurnGuess: return L"[进行猜测]";
        case GamePhase::TurnAfterCorrectGuess: return L"[猜对奖励阶段]";
        case GamePhase::TurnPlacePendingHidden: return L"[暗牌归位中]";
        case GamePhase::TurnPlacePendingRevealed: return L"[惩罚牌归位中]";
        case GamePhase::GameOver: return L"[游戏结束]";
        }
        return L"[未知阶段]";
    }

    std::wstring GameView::cardText(const Card& c, PlayerId owner) const {
        if (owner == PlayerId::Player1 && c.isHidden()) {
            if (game_.phase() != GamePhase::GameOver) {
                return L"?";
            }
        }
        std::wstring str = c.isJoker() ? L"J" : std::to_wstring(c.value());
        if (c.isRevealed()) str += L"*";
        return str;
    }

} // namespace dvcode::ui
