#include "Game.h"
#include "Audio.h"
#include "Board.h"
#include "Cpu.h"
#include "Fx.h"
#include "Theme.h"
#include "Dice.h"
#include "Ui.h"

namespace ReyesCuadra
{
    constexpr float PANEL_Y       = 420.0f;
    constexpr float PANEL_H       = 270.0f;
    constexpr float LEFT_PANEL_X  = 40.0f;
    constexpr float LEFT_PANEL_W  = 620.0f;
    constexpr float RIGHT_PANEL_X = 680.0f;
    constexpr float RIGHT_PANEL_W = 560.0f;

    static const Color PANEL_COLOR    = Color{ 24, 34, 60, 242 };
    static const Color PANEL_BORDER   = Color{ 86, 60, 102, 255 };
    static const Color BLESSING_COLOR = COL_BLESSING;
    static const Color CURSE_COLOR    = COL_CURSE;

    constexpr float REVEAL_TIME    = 0.55f;
    constexpr double CPU_THINK_TIME = 0.5;

    static const char* CREDITS_LINE =
        "Federico Fernandez Soto   ·   Salvador Rosafioriti   ·   Valentin Reyes";

    static void AddLog(GameState& game, const char* text)
    {
        if (text == nullptr || text[0] == '\0') return;

        for (int i = 0; i < LOG_LINES - 1; ++i) game.log[i] = game.log[i + 1];
        game.log[LOG_LINES - 1] = text;
    }

    static void StartMatch(GameState& game)
    {
        static const Color PLAYER_COLORS[PLAYER_COUNT] =
        {
            Color{ 240, 200,  70, 255 },
            Color{ 235, 110,  80, 255 },
            Color{  95, 170, 235, 255 },
            Color{ 150, 220, 130, 255 }
        };

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            Player& player = game.players[i];

            player.id             = i;
            player.color          = PLAYER_COLORS[i];
            player.path           = PathKind::Normal;
            player.stage          = 0;
            player.initiativeRoll = 0;
            player.isCpu          = (i >= game.humanCount);
            player.cards.clear();
        }

        InitDeckSet(game.deckSet);

        game.currentPlayer   = 0;
        game.currentStage    = 0;
        game.eventDifficulty = 0;
        game.eventSuccess    = false;
        game.eventEffects.clear();
        game.eventSession    = RollSession{};
        game.cardOptionCount = 0;
        game.currentDuel     = 0;
        game.kingId          = -1;

        for (int i = 0; i < DUEL_COUNT; ++i) game.duels[i] = Duel{};
        for (int i = 0; i < LOG_LINES; ++i)  game.log[i].clear();

        PlayTrack(MusicTrack::Explore);

        game.cpuSignature  = -1;
        game.cpuNextAction = 0.0;

        int cpuCount = PLAYER_COUNT - game.humanCount;
        if (cpuCount > 0)
        {
            AddLog(game, TextFormat("Comienza la partida: %i jugador(es) y %i CPU.",
                                    game.humanCount, cpuCount));
        }
        else
        {
            AddLog(game, "Comienza la partida: 4 jugadores en la base de la montaña.");
        }
    }

    static void FinishEventRoll(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        game.eventRoll    = game.eventSession.roll;
        game.eventEffects = game.eventSession.appliedNames;

        if (game.eventRoll.criticalHit)       game.eventSuccess = true;
        else if (game.eventRoll.criticalFail) game.eventSuccess = false;
        else                                  game.eventSuccess = (game.eventRoll.total >= game.eventDifficulty);

        game.revealUntil = GetTime() + REVEAL_TIME;
        game.resultTime  = game.revealUntil;

        if (game.eventRoll.criticalHit)       PlaySfxDelayed(SoundId::Crit, 0.55f);
        else if (game.eventSuccess)           PlaySfxDelayed(SoundId::Success, 0.55f);
        else                                  PlaySfxDelayed(SoundId::Fail, 0.55f);

        AddLog(game, TextFormat("Jugador %i%s (%s): D20 %i -> %i vs %i -> %s",
                                game.currentPlayer + 1,
                                player.isCpu ? " [CPU]" : "",
                                GetPathName(player.path),
                                game.eventRoll.natural,
                                game.eventRoll.total,
                                game.eventDifficulty,
                                game.eventSuccess ? "ÉXITO" : "FALLO"));

        game.phase = GamePhase::EventResult;
    }

    static void StartEventRoll(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        RollRequest request;
        request.isCombat  = false;
        request.rollIndex = -1;

        game.eventDifficulty = GetEventDifficulty(player.path, game.currentStage);

        PlaySfx(SoundId::Dice);
        BeginRoll(game.eventSession, player, nullptr, request, nullptr);

        if (game.eventSession.choice.active) game.phase = GamePhase::EventChoice;
        else if (game.eventSession.done)     FinishEventRoll(game);
    }

    static void AdvanceEventChoice(GameState& game, int option)
    {
        PlaySfx(SoundId::Select);
        AdvanceRoll(game.eventSession, option);

        if (game.eventSession.choice.active) return;
        if (game.eventSession.done) FinishEventRoll(game);
    }

    static void OfferCards(GameState& game)
    {
        const Player& player = game.players[game.currentPlayer];

        game.offerKind  = game.eventSuccess ? CardKind::Blessing : CardKind::Curse;
        game.offerLevel = GetCardLevelForPath(player.path);

        game.cardOptionCount = DrawCardOptions(game.deckSet, game.offerKind,
                                               game.offerLevel, game.cardOptions);

        game.phase = GamePhase::CardSelection;
    }

    static void TakeCard(GameState& game, int chosenIndex)
    {
        Player& player = game.players[game.currentPlayer];

        int defId = game.cardOptions[chosenIndex];
        const CardDef& def = GetCardDef(defId);

        CardInstance card;
        card.defId = defId;

        bool rollScoped = (def.scope == EffectScope::NextRoll ||
                           def.scope == EffectScope::NextExplorationRoll ||
                           def.scope == EffectScope::NextCombatRoll);

        if (rollScoped) card.rollsLeft = (def.uses > 0) ? def.uses : 1;

        if (def.condition == 1 && CountCards(player, CardKind::Blessing) < def.paramB)
        {
            card.spent = true;
        }

        player.cards.push_back(card);

        ReturnCardOptions(game.deckSet, game.offerKind, game.offerLevel,
                          game.cardOptions, game.cardOptionCount, chosenIndex);

        PlaySfx(SoundId::Card);
        AddLog(game, TextFormat("Jugador %i toma: %s", game.currentPlayer + 1, def.name));

        std::string message;
        ApplyOnAcquire(player, game.players, PLAYER_COUNT, defId, message);
        AddLog(game, message.c_str());

        ++player.stage;
        ++game.currentPlayer;

        if (game.currentPlayer >= PLAYER_COUNT)
        {
            game.currentPlayer = 0;
            ++game.currentStage;
        }

        if (game.currentStage >= STAGE_COUNT)
        {
            AddLog(game, "Todos llegaron a la zona de combate.");
            game.phase = GamePhase::Initiative;
        }
        else
        {
            game.phase = GamePhase::PathSelection;
        }
    }

    static void StepCombat(GameState& game, int option)
    {
        Duel& duel = game.duels[game.currentDuel];

        int rollsBefore = duel.rollsPlayed;
        bool wasChoosing = DuelChoicePending(duel);

        if (wasChoosing) PlaySfx(SoundId::Select);
        else             PlaySfx(SoundId::Dice);

        StepDuel(duel, game.players, option);

        if (duel.rollsPlayed > rollsBefore) PlaySfxDelayed(SoundId::Hit, 0.45f);

        if (duel.session.done || duel.rollsPlayed > rollsBefore)
        {
            game.revealUntil = GetTime() + REVEAL_TIME;
            game.resultTime  = game.revealUntil;
        }

        if (DuelChoicePending(duel))
        {
            game.phase = GamePhase::CombatChoice;
            return;
        }

        game.phase = GamePhase::CombatRoll;

        if (duel.resolved)
        {
            AddLog(game, TextFormat("J%i %i - %i J%i -> gana J%i",
                                    duel.playerA + 1, duel.winsA,
                                    duel.winsB, duel.playerB + 1,
                                    duel.winner + 1));
            AddLog(game, duel.rewardMessage.c_str());
            PlaySfxDelayed(SoundId::Success, 0.9f);
            game.phase = GamePhase::CombatResult;
        }
    }

    void InitGame(GameState& game)
    {
        StartMatch(game);
        game.phase = GamePhase::Menu;
    }

    static int CpuActorFor(const GameState& game)
    {
        switch (game.phase)
        {
            case GamePhase::PathSelection:
            case GamePhase::EventRoll:
            case GamePhase::EventChoice:
            case GamePhase::EventResult:
            case GamePhase::CardSelection:
                return game.currentPlayer;

            case GamePhase::CombatRoll:
            case GamePhase::CombatChoice:
            {
                const Duel& duel = game.duels[game.currentDuel];
                if (duel.playerA < 0 || duel.playerB < 0) return -1;
                return (duel.activeSide == 0) ? duel.playerA : duel.playerB;
            }

            default: return -1;
        }
    }

    static bool IsCpuTurn(const GameState& game)
    {
        int actor = CpuActorFor(game);
        return (actor >= 0) && game.players[actor].isCpu;
    }

    static int PhaseSignature(const GameState& game)
    {
        const Duel& duel = game.duels[game.currentDuel];

        return static_cast<int>(game.phase) * 1000
             + game.currentPlayer * 100
             + duel.rollsPlayed * 10
             + duel.activeSide
             + game.currentDuel;
    }

    static void RunCpuTurn(GameState& game)
    {
        switch (game.phase)
        {
            case GamePhase::PathSelection:
            {
                Player& player = game.players[game.currentPlayer];
                PathKind chosen = CpuChoosePath(player, game.currentStage);

                PlaySfx(SoundId::Select);
                player.path = chosen;
                AddLog(game, TextFormat("Jugador %i [CPU] toma el camino %s",
                                        game.currentPlayer + 1, GetPathName(chosen)));
                game.phase = GamePhase::EventRoll;
            } break;

            case GamePhase::EventRoll:   StartEventRoll(game); break;
            case GamePhase::EventChoice: AdvanceEventChoice(game, CpuChooseOption(game.eventSession.choice)); break;
            case GamePhase::EventResult: OfferCards(game); break;

            case GamePhase::CardSelection:
                TakeCard(game, CpuChooseCard(game.cardOptions, game.cardOptionCount));
                break;

            case GamePhase::CombatRoll:   StepCombat(game, -1); break;
            case GamePhase::CombatChoice:
                StepCombat(game, CpuChooseOption(game.duels[game.currentDuel].session.choice));
                break;

            default: break;
        }
    }

    static int ReadOptionKey(int optionCount)
    {
        if (optionCount > 0 && IsKeyPressed(KEY_ONE))   return 0;
        if (optionCount > 1 && IsKeyPressed(KEY_TWO))   return 1;
        if (optionCount > 2 && IsKeyPressed(KEY_THREE)) return 2;
        if (optionCount > 3 && IsKeyPressed(KEY_FOUR))  return 3;
        if (optionCount > 4 && IsKeyPressed(KEY_FIVE))  return 4;
        return -1;
    }

    void UpdateGame(GameState& game)
    {
        if (IsKeyPressed(KEY_M)) ToggleMute();

        if (GetTime() < game.revealUntil) return;

        int signature = PhaseSignature(game);
        if (signature != game.cpuSignature)
        {
            game.cpuSignature  = signature;
            game.cpuNextAction = GetTime() + CPU_THINK_TIME;
        }

        if (IsCpuTurn(game))
        {
            if (GetTime() >= game.cpuNextAction) RunCpuTurn(game);
            return;
        }

        switch (game.phase)
        {
            case GamePhase::Menu:
            {
                for (int i = 0; i < PLAYER_COUNT; ++i)
                {
                    if (IsKeyPressed(KEY_ONE + i))
                    {
                        PlaySfx(SoundId::Select);
                        game.humanCount = i + 1;
                    }
                }

                if (IsKeyPressed(KEY_ENTER))
                {
                    PlaySfx(SoundId::Select);
                    StartMatch(game);
                    game.phase = GamePhase::PathSelection;
                }
            } break;

            case GamePhase::PathSelection:
            {
                PathKind chosen = PathKind::Normal;
                bool picked = false;

                if (IsKeyPressed(KEY_ONE))   { chosen = PathKind::Easy;   picked = true; }
                if (IsKeyPressed(KEY_TWO))   { chosen = PathKind::Normal; picked = true; }
                if (IsKeyPressed(KEY_THREE)) { chosen = PathKind::Hard;   picked = true; }

                if (picked)
                {
                    PlaySfx(SoundId::Select);
                    game.players[game.currentPlayer].path = chosen;
                    game.phase = GamePhase::EventRoll;
                }
            } break;

            case GamePhase::EventRoll:
            {
                if (IsKeyPressed(KEY_SPACE)) StartEventRoll(game);
            } break;

            case GamePhase::EventChoice:
            {
                int option = ReadOptionKey(static_cast<int>(game.eventSession.choice.options.size()));
                if (option >= 0) AdvanceEventChoice(game, option);
            } break;

            case GamePhase::EventResult:
            {
                if (IsKeyPressed(KEY_SPACE)) OfferCards(game);
            } break;

            case GamePhase::CardSelection:
            {
                int option = ReadOptionKey(game.cardOptionCount);
                if (option >= 0) TakeCard(game, option);
            } break;

            case GamePhase::Initiative:
            {
                if (IsKeyPressed(KEY_SPACE))
                {
                    PlaySfx(SoundId::Dice);
                    PlayTrack(MusicTrack::Combat);

                    RollInitiative(game.players, game.initiativeOrder);
                    BuildBracket(game.initiativeOrder, game.duels);

                    AddLog(game, TextFormat("Iniciativa: J%i(%i) J%i(%i) J%i(%i) J%i(%i)",
                        game.initiativeOrder[0] + 1, game.players[game.initiativeOrder[0]].initiativeRoll,
                        game.initiativeOrder[1] + 1, game.players[game.initiativeOrder[1]].initiativeRoll,
                        game.initiativeOrder[2] + 1, game.players[game.initiativeOrder[2]].initiativeRoll,
                        game.initiativeOrder[3] + 1, game.players[game.initiativeOrder[3]].initiativeRoll));

                    game.currentDuel = 0;
                    game.phase = GamePhase::InitiativeResult;
                }
            } break;

            case GamePhase::InitiativeResult:
            {
                if (IsKeyPressed(KEY_SPACE)) game.phase = GamePhase::CombatIntro;
            } break;

            case GamePhase::CombatIntro:
            {
                if (IsKeyPressed(KEY_SPACE))
                {
                    StartDuel(game.duels[game.currentDuel], game.players);
                    game.phase = GamePhase::CombatRoll;
                }
            } break;

            case GamePhase::CombatRoll:
            {
                if (IsKeyPressed(KEY_SPACE)) StepCombat(game, -1);
            } break;

            case GamePhase::CombatChoice:
            {
                const Duel& duel = game.duels[game.currentDuel];
                int option = ReadOptionKey(static_cast<int>(duel.session.choice.options.size()));
                if (option >= 0) StepCombat(game, option);
            } break;

            case GamePhase::CombatResult:
            {
                if (IsKeyPressed(KEY_SPACE))
                {
                    if (game.duels[game.currentDuel].isFinal)
                    {
                        game.kingId = game.duels[game.currentDuel].winner;
                        StopTrack();
                        PlaySfx(SoundId::Victory);
                        game.phase  = GamePhase::Victory;
                    }
                    else
                    {
                        ++game.currentDuel;
                        if (game.currentDuel == 2) BuildSecondRound(game.duels);
                        game.phase = GamePhase::CombatIntro;
                    }
                }
            } break;

            case GamePhase::Victory:
            {
                if (IsKeyPressed(KEY_R))
                {
                    PlaySfx(SoundId::Select);
                    StartMatch(game);
                    game.phase = GamePhase::PathSelection;
                }
            } break;
        }
    }

    // ------------------------------------------------------------------ dibujo

    static const char* PlayerLabel(const Player& player)
    {
        return player.isCpu ? TextFormat("Jugador %i [CPU]", player.id + 1)
                            : TextFormat("Jugador %i", player.id + 1);
    }

    static bool IsCpuPlaying(const GameState& game, int actor)
    {
        return actor >= 0 && game.players[actor].isCpu;
    }

    static bool IsRevealing(const GameState& game)
    {
        return GetTime() < game.revealUntil;
    }

    static void DrawPanel(Rectangle bounds, const char* title)
    {
        DrawSoftPanel(bounds, PANEL_COLOR, PANEL_BORDER, 0.08f);

        Rectangle header{ bounds.x, bounds.y, bounds.width, 34.0f };
        DrawRectangleRounded(header, 0.35f, 6, Fade(COL_MAGENTA, 0.35f));
        DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y) + 24,
                      static_cast<int>(bounds.width), 10, Fade(COL_MAGENTA, 0.0f));

        UiText(title, static_cast<int>(bounds.x) + 16, static_cast<int>(bounds.y) + 8,
               19, COL_CREAM, FontStyle::Bold);
    }

    static void DrawDie(int x, int y, int size, int value, bool spinning, Color accent)
    {
        (void)spinning;

        Rectangle face{ static_cast<float>(x), static_cast<float>(y),
                        static_cast<float>(size), static_cast<float>(size) };

        Rectangle shadow = face;
        shadow.x += 3.0f; shadow.y += 4.0f;
        DrawRectangleRounded(shadow, 0.26f, 6, Fade(BLACK, 0.4f));

        DrawRectangleRounded(face, 0.26f, 6, COL_CREAM);
        DrawRectangleRoundedLinesEx(face, 0.26f, 6, 3.0f, accent);

        const char* text = TextFormat("%i", value);
        int textSize = (value >= 10) ? size - 22 : size - 16;
        int width = UiMeasure(text, textSize, FontStyle::Bold);

        UiText(text, x + size / 2 - width / 2, y + size / 2 - textSize / 2 - 2,
               textSize, COL_INK_DEEP, FontStyle::Bold);
    }

    static std::string DescribeRolls(const DiceRoll rolls[], int count)
    {
        std::string text;

        for (int i = 0; i < count; ++i)
        {
            if (i > 0) text += "    ";
            text += TextFormat("%i", rolls[i].total);
            if (rolls[i].criticalHit)  text += "!";
            if (rolls[i].criticalFail) text += "?";
        }

        return text;
    }

    static void DrawBanner(const GameState& game)
    {
        double elapsed = GetTime() - game.resultTime;
        if (elapsed < 0.0 || elapsed > 1.3) return;

        float t = EaseOutBack(static_cast<float>(elapsed) / 0.28f);
        float fade = (elapsed > 1.0) ? 1.0f - static_cast<float>(elapsed - 1.0) / 0.3f : 1.0f;

        const char* text = game.eventSuccess ? "¡ÉXITO!" : "FALLO";
        Color color = game.eventSuccess ? COL_BLESSING : COL_CURSE;

        int size = static_cast<int>(18.0f + 30.0f * t);
        int width = UiMeasure(text, size, FontStyle::Bold);
        int x = SCREEN_WIDTH / 2 - width / 2;
        int y = 300;

        DrawRectangleRounded(Rectangle{ static_cast<float>(x) - 26.0f, static_cast<float>(y) - 12.0f,
                                        static_cast<float>(width) + 52.0f, static_cast<float>(size) + 24.0f },
                             0.4f, 8, Fade(COL_INK_DEEP, 0.8f * fade));
        UiText(text, x, y, size, Fade(color, fade), FontStyle::Bold);
    }

    static void DrawChoice(const PendingChoice& choice)
    {
        Rectangle bounds{ LEFT_PANEL_X, PANEL_Y, LEFT_PANEL_W, PANEL_H };
        DrawPanel(bounds, "ELEGÍ");

        UiTextWrapped(choice.title.c_str(), static_cast<int>(LEFT_PANEL_X) + 16,
                      static_cast<int>(PANEL_Y) + 44, static_cast<int>(LEFT_PANEL_W) - 32,
                      16, COL_CREAM, 2, FontStyle::Bold);

        for (int i = 0; i < static_cast<int>(choice.options.size()); ++i)
        {
            Rectangle row{ LEFT_PANEL_X + 14.0f, PANEL_Y + 88.0f + static_cast<float>(i) * 42.0f,
                           LEFT_PANEL_W - 28.0f, 36.0f };

            DrawRectangleRounded(row, 0.3f, 6, Fade(COL_MAGENTA, 0.18f));
            DrawRectangleRoundedLinesEx(row, 0.3f, 6, 2.0f, Fade(COL_GOLD, 0.7f));

            UiText(TextFormat("[%i]", i + 1), static_cast<int>(row.x) + 12,
                   static_cast<int>(row.y) + 9, 16, COL_GOLD, FontStyle::Bold);

            UiTextWrapped(choice.options[i].label.c_str(), static_cast<int>(row.x) + 52,
                          static_cast<int>(row.y) + 10, static_cast<int>(row.width) - 70,
                          15, COL_CREAM, 1);
        }
    }

    static void DrawPlayerList(const GameState& game)
    {
        Rectangle bounds{ RIGHT_PANEL_X, PANEL_Y, RIGHT_PANEL_W, PANEL_H };
        DrawPanel(bounds, "JUGADORES");

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = game.players[i];

            int rowY = static_cast<int>(PANEL_Y) + 52 + i * 52;
            bool isActive = (i == game.currentPlayer) &&
                            (game.phase == GamePhase::PathSelection ||
                             game.phase == GamePhase::EventRoll ||
                             game.phase == GamePhase::EventChoice ||
                             game.phase == GamePhase::EventResult ||
                             game.phase == GamePhase::CardSelection);

            Rectangle row{ RIGHT_PANEL_X + 8.0f, static_cast<float>(rowY) - 6.0f,
                           RIGHT_PANEL_W - 16.0f, 46.0f };

            if (isActive)
            {
                DrawRectangleRounded(row, 0.3f, 6, Fade(player.color, 0.18f));
                DrawRectangleRoundedLinesEx(row, 0.3f, 6, 2.0f, Fade(player.color, 0.6f));
            }
            else
            {
                DrawRectangleRounded(row, 0.3f, 6, Fade(COL_INK_DEEP, 0.35f));
            }

            DrawCircle(static_cast<int>(RIGHT_PANEL_X) + 32, rowY + 17, 12.0f, player.color);
            UiText(TextFormat("%i", i + 1), static_cast<int>(RIGHT_PANEL_X) + 28, rowY + 9, 15,
                   COL_INK_DEEP, FontStyle::Bold);

            UiText(TextFormat("Jugador %i", i + 1),
                   static_cast<int>(RIGHT_PANEL_X) + 54, rowY + 1, 17, COL_CREAM, FontStyle::Bold);

            if (player.isCpu)
            {
                Rectangle tag{ RIGHT_PANEL_X + 142.0f, static_cast<float>(rowY) + 2.0f, 38.0f, 17.0f };
                DrawRectangleRounded(tag, 0.5f, 5, Fade(COL_MAGENTA, 0.55f));
                UiText("CPU", static_cast<int>(tag.x) + 8, static_cast<int>(tag.y) + 2, 12,
                       COL_CREAM, FontStyle::Bold);
            }

            UiText(GetPathName(player.path),
                   static_cast<int>(RIGHT_PANEL_X) + 54, rowY + 22, 14, GetPathColor(player.path));

            UiText(TextFormat("%i", CountCards(player, CardKind::Blessing)),
                   static_cast<int>(RIGHT_PANEL_X) + 230, rowY + 10, 18, COL_BLESSING, FontStyle::Bold);
            UiText("bendiciones", static_cast<int>(RIGHT_PANEL_X) + 248, rowY + 13, 13,
                   Fade(COL_BLESSING, 0.75f));

            UiText(TextFormat("%i", CountCards(player, CardKind::Curse)),
                   static_cast<int>(RIGHT_PANEL_X) + 370, rowY + 10, 18, COL_CURSE, FontStyle::Bold);
            UiText("maldiciones", static_cast<int>(RIGHT_PANEL_X) + 388, rowY + 13, 13,
                   Fade(COL_CURSE, 0.75f));

            UiText(TextFormat("%i sin usar", CountPendingCards(player)),
                   static_cast<int>(RIGHT_PANEL_X) + 470, rowY + 13, 13, COL_CREAM_DIM);
        }
    }

    static void DrawLog(const GameState& game)
    {
        for (int i = 0; i < LOG_LINES; ++i)
        {
            float alpha = 0.30f + 0.12f * static_cast<float>(i);

            UiText(game.log[i].c_str(),
                   static_cast<int>(LEFT_PANEL_X) + 16,
                   static_cast<int>(PANEL_Y) + 190 + i * 16,
                   13, Fade(COL_CREAM, alpha));
        }
    }

    static void DrawCardOptionsUI(const GameState& game)
    {
        for (int i = 0; i < game.cardOptionCount; ++i)
        {
            const CardDef& def = GetCardDef(game.cardOptions[i]);

            bool isBlessing = (def.kind == CardKind::Blessing);
            Color color = isBlessing ? COL_BLESSING : COL_CURSE;
            Color levelColor = GetPathColor(static_cast<PathKind>(static_cast<int>(def.level)));

            Rectangle bounds{ LEFT_PANEL_X + 14.0f, PANEL_Y + 44.0f + static_cast<float>(i) * 68.0f,
                              LEFT_PANEL_W - 28.0f, 62.0f };

            DrawRectangleRounded(bounds, 0.16f, 6, Fade(COL_INK_DEEP, 0.75f));
            DrawRectangleRounded(Rectangle{ bounds.x, bounds.y, bounds.width, 22.0f }, 0.5f, 6,
                                 Fade(color, 0.28f));
            DrawRectangleRoundedLinesEx(bounds, 0.16f, 6, 2.0f, Fade(color, 0.85f));
            DrawRectangleRounded(Rectangle{ bounds.x + 6.0f, bounds.y + 6.0f, 5.0f, bounds.height - 12.0f },
                                 0.8f, 4, levelColor);

            UiText(TextFormat("[%i]", i + 1),
                   static_cast<int>(bounds.x) + 18, static_cast<int>(bounds.y) + 4, 15,
                   COL_CREAM, FontStyle::Bold);

            UiText(def.name,
                   static_cast<int>(bounds.x) + 52, static_cast<int>(bounds.y) + 3, 17, color, FontStyle::Bold);

            UiTextWrapped(def.text,
                          static_cast<int>(bounds.x) + 18, static_cast<int>(bounds.y) + 26,
                          static_cast<int>(bounds.width) - 32, 13, COL_CREAM_DIM, 2);
        }
    }

    static void DrawCombatPips(const Duel& duel, int x, int y, const GameState& game)
    {
        for (int i = 0; i < COMBAT_ROLLS; ++i)
        {
            int cx = x + i * 26;
            Color color = Fade(COL_CREAM, 0.18f);

            if (i < duel.rollsPlayed)
            {
                if (duel.outcomes[i] > 0)      color = game.players[duel.playerA].color;
                else if (duel.outcomes[i] < 0) color = game.players[duel.playerB].color;
                else                           color = Fade(COL_CREAM, 0.45f);
            }

            DrawCircle(cx, y, 8.0f, color);
            DrawCircleLines(cx, y, 8.0f, Fade(COL_INK_DEEP, 0.8f));
        }
    }

    static void DrawCombatRolls(const GameState& game, const Duel& duel, int textX, int line1, int line2, int line3)
    {
        bool revealing = IsRevealing(game);
        int shown = duel.rollsPlayed;

        std::string rollsA = DescribeRolls(duel.rollsA, shown);
        std::string rollsB = DescribeRolls(duel.rollsB, shown);

        UiText(PlayerLabel(game.players[duel.playerA]), textX, line1, 17,
               game.players[duel.playerA].color, FontStyle::Bold);
        UiText(rollsA.c_str(), textX + 110, line1, 17, COL_CREAM);

        UiText(PlayerLabel(game.players[duel.playerB]), textX, line2, 17,
               game.players[duel.playerB].color, FontStyle::Bold);
        UiText(rollsB.c_str(), textX + 110, line2, 17, COL_CREAM);

        DrawCombatPips(duel, textX + 440, line1 + 20, game);

        if (revealing && !duel.resolved)
        {
            int side = duel.activeSide;
            int lineY = (side == 0) ? line2 : line1;
            int value = SpinningValue(static_cast<float>(GetTime()), DICE_FACES);
            DrawDie(textX + 330, lineY - 6, 34, value, true, COL_GOLD);
        }

        UiText("! = 20 natural      ? = 1 natural", textX, line3 + 6, 13, Fade(COL_CREAM, 0.45f));
    }

    static void DrawTurnPanel(const GameState& game)
    {
        Rectangle bounds{ LEFT_PANEL_X, PANEL_Y, LEFT_PANEL_W, PANEL_H };

        int textX = static_cast<int>(LEFT_PANEL_X) + 16;
        int line1 = static_cast<int>(PANEL_Y) + 50;
        int line2 = static_cast<int>(PANEL_Y) + 78;
        int line3 = static_cast<int>(PANEL_Y) + 106;
        int line4 = static_cast<int>(PANEL_Y) + 140;

        const Player& player = game.players[game.currentPlayer];

        switch (game.phase)
        {
            case GamePhase::PathSelection:
            {
                DrawPanel(bounds, TextFormat("TURNO DE %s  ·  ETAPA %i / %i",
                                             player.isCpu ? TextFormat("LA CPU (JUGADOR %i)", game.currentPlayer + 1)
                                                          : TextFormat("JUGADOR %i", game.currentPlayer + 1),
                                             game.currentStage + 1, STAGE_COUNT));

                UiText(player.isCpu ? "La CPU está eligiendo camino..."
                                    : "Elegí tu camino para esta etapa:",
                       textX, line1, 17, COL_CREAM);

                const char* labels[3] = { "[1] FÁCIL", "[2] NORMAL", "[3] DIFÍCIL" };
                const char* rewards[3] = { "carta común", "carta peligrosa", "carta épica" };

                for (int i = 0; i < 3; ++i)
                {
                    PathKind path = static_cast<PathKind>(i);
                    int rowY = line2 + i * 28;

                    DrawRectangleRounded(Rectangle{ static_cast<float>(textX) - 6.0f,
                                                    static_cast<float>(rowY) - 4.0f,
                                                    LEFT_PANEL_W - 44.0f, 24.0f },
                                         0.4f, 6, Fade(GetPathColor(path), 0.12f));

                    UiText(labels[i], textX, rowY, 16, GetPathColor(path), FontStyle::Bold);
                    UiText(TextFormat("dificultad %i", GetEventDifficulty(path, game.currentStage)),
                           textX + 120, rowY, 16, COL_CREAM);
                    UiText(rewards[i], textX + 250, rowY, 15, Fade(COL_CREAM, 0.7f));
                }
            } break;

            case GamePhase::EventRoll:
            {
                DrawPanel(bounds, TextFormat("JUGADOR %i  ·  CAMINO %s",
                                             game.currentPlayer + 1, GetPathName(player.path)));

                UiText("Dificultad", textX, line1, 14, Fade(COL_CREAM, 0.6f));
                UiText(TextFormat("%i", GetEventDifficulty(player.path, game.currentStage)),
                       textX, line1 + 16, 32, COL_CREAM, FontStyle::Bold);

                RollRequest request;
                request.isCombat  = false;
                request.rollIndex = -1;

                std::string pending = DescribePendingEffects(player, nullptr, request);

                UiText("Cartas que se activan en esta tirada:", textX + 90, line1, 14, Fade(COL_CREAM, 0.6f));
                UiTextWrapped(pending.empty() ? "ninguna" : pending.c_str(),
                              textX + 90, line1 + 20, static_cast<int>(LEFT_PANEL_W) - 130, 14,
                              pending.empty() ? Fade(COL_CREAM, 0.4f) : COL_GOLD, 2);

                if (player.isCpu) UiText("La CPU está por tirar...", textX, line4 + 14, 19,
                                          Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                else               UiText("[ESPACIO] tirar el D20", textX, line4 + 14, 19,
                                          COL_GOLD, FontStyle::Bold);
            } break;

            case GamePhase::EventChoice:
            {
                DrawChoice(game.eventSession.choice);
            } break;

            case GamePhase::EventResult:
            {
                bool revealing = IsRevealing(game);

                DrawPanel(bounds, revealing ? "TIRANDO..."
                                            : (game.eventSuccess ? "EVENTO SUPERADO" : "EVENTO FALLIDO"));

                int value = revealing ? SpinningValue(static_cast<float>(GetTime()), DICE_FACES)
                                      : game.eventRoll.natural;
                Color accent = revealing ? COL_GOLD
                                         : (game.eventSuccess ? COL_BLESSING : COL_CURSE);

                DrawDie(textX, line1 - 4, 58, value, revealing, accent);

                if (!revealing)
                {
                    UiText(TextFormat("Total %i", game.eventRoll.total),
                           textX + 76, line1 + 2, 26, COL_CREAM, FontStyle::Bold);
                    UiText(TextFormat("contra dificultad %i", game.eventDifficulty),
                           textX + 76, line1 + 32, 15, Fade(COL_CREAM, 0.7f));

                    if (game.eventRoll.criticalHit)
                    {
                        UiText("20 NATURAL: éxito crítico, se ignoran los modificadores",
                               textX, line3 + 14, 15, COL_GOLD);
                    }
                    else if (game.eventRoll.criticalFail)
                    {
                        UiText("1 NATURAL: fallo crítico, se ignoran los modificadores",
                               textX, line3 + 14, 15, ORANGE);
                    }
                    else if (!game.eventEffects.empty())
                    {
                        UiTextWrapped(TextFormat("Se aplicaron: %s", game.eventEffects.c_str()),
                                      textX, line3 + 10, static_cast<int>(LEFT_PANEL_W) - 32, 14, COL_GOLD, 2);
                    }

                    if (player.isCpu)
                    {
                        UiText("La CPU elige su carta...", textX, line4 + 14, 19,
                               Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                    }
                    else
                    {
                        UiText(game.eventSuccess ? "[ESPACIO] elegir bendición"
                                                 : "[ESPACIO] recibir maldición",
                               textX, line4 + 14, 19, COL_GOLD, FontStyle::Bold);
                    }
                }
            } break;

            case GamePhase::CardSelection:
            {
                DrawPanel(bounds, TextFormat("%s: %s %s", PlayerLabel(player),
                                             game.offerKind == CardKind::Blessing ? "bendición" : "maldición",
                                             GetCardLevelName(game.offerLevel)));

                DrawCardOptionsUI(game);
            } break;

            case GamePhase::Initiative:
            {
                DrawPanel(bounds, "FASE DE COMBATE");
                UiText("Los cuatro jugadores llegaron a la cima.", textX, line1, 17, COL_CREAM);
                UiText("La iniciativa define los cruces del torneo.", textX, line2, 15, Fade(COL_CREAM, 0.7f));
                UiText("[ESPACIO] tirar iniciativa", textX, line4 + 14, 19, COL_GOLD, FontStyle::Bold);
            } break;

            case GamePhase::InitiativeResult:
            {
                DrawPanel(bounds, "CRUCES DEL TORNEO");

                UiText(TextFormat("Semifinal 1:  Jugador %i  vs  Jugador %i",
                                  game.duels[0].playerA + 1, game.duels[0].playerB + 1),
                       textX, line1, 17, COL_CREAM);

                UiText(TextFormat("Semifinal 2:  Jugador %i  vs  Jugador %i",
                                  game.duels[1].playerA + 1, game.duels[1].playerB + 1),
                       textX, line2, 17, COL_CREAM);

                UiText("Después juegan los dos perdedores y, al final, los dos ganadores.",
                       textX, line3, 14, Fade(COL_CREAM, 0.7f));

                UiText("[ESPACIO] continuar", textX, line4 + 14, 19, COL_GOLD, FontStyle::Bold);
            } break;

            case GamePhase::CombatIntro:
            {
                const Duel& duel = game.duels[game.currentDuel];

                DrawPanel(bounds, duel.isFinal ? "BATALLA FINAL" : "COMBATE");

                UiText(GetDuelTitle(duel), textX, line1, 23, COL_CREAM, FontStyle::Bold);
                UiText(TextFormat("Al mejor de %i tiradas.", COMBAT_ROLLS),
                       textX, line2 + 8, 15, Fade(COL_CREAM, 0.7f));

                UiText(TextFormat("Cartas sin usar:  J%i tiene %i    ·    J%i tiene %i",
                                  duel.playerA + 1, CountPendingCards(game.players[duel.playerA]),
                                  duel.playerB + 1, CountPendingCards(game.players[duel.playerB])),
                       textX, line3 + 8, 15, Fade(COL_CREAM, 0.7f));

                UiText("[ESPACIO] empezar el combate", textX, line4 + 14, 19, COL_GOLD, FontStyle::Bold);
            } break;

            case GamePhase::CombatRoll:
            {
                const Duel& duel = game.duels[game.currentDuel];
                int who = (duel.activeSide == 0) ? duel.playerA : duel.playerB;

                DrawPanel(bounds, TextFormat("COMBATE  ·  TIRADA %i", duel.rollsPlayed + 1));

                DrawCombatRolls(game, duel, textX, line1, line2, line3);

                if (!IsRevealing(game))
                {
                    if (IsCpuPlaying(game, who))
                    {
                        UiText(TextFormat("Tira la CPU (Jugador %i)...", who + 1),
                               textX, line4 + 14, 19, Fade(COL_CREAM, 0.7f), FontStyle::Bold);
                    }
                    else
                    {
                        UiText(TextFormat("[ESPACIO] tira el Jugador %i", who + 1),
                               textX, line4 + 14, 19, COL_GOLD, FontStyle::Bold);
                    }
                }
            } break;

            case GamePhase::CombatChoice:
            {
                DrawChoice(game.duels[game.currentDuel].session.choice);
            } break;

            case GamePhase::CombatResult:
            {
                const Duel& duel = game.duels[game.currentDuel];

                DrawPanel(bounds, TextFormat("GANA EL JUGADOR %i   (%i - %i)",
                                             duel.winner + 1,
                                             (duel.winner == duel.playerA) ? duel.winsA : duel.winsB,
                                             (duel.winner == duel.playerA) ? duel.winsB : duel.winsA));

                DrawCombatRolls(game, duel, textX, line1, line2, line3);

                std::string effects = duel.effectsA;
                if (!duel.effectsB.empty())
                {
                    if (!effects.empty()) effects += ", ";
                    effects += duel.effectsB;
                }

                if (!effects.empty())
                {
                    UiTextWrapped(TextFormat("Cartas aplicadas: %s", effects.c_str()),
                                  textX, line3 + 26, static_cast<int>(LEFT_PANEL_W) - 32, 13, COL_GOLD, 2);
                }

                if (!IsRevealing(game))
                {
                    UiText("[ESPACIO] continuar", textX, line4 + 14, 19, COL_GOLD, FontStyle::Bold);
                }
            } break;

            default: break;
        }
    }

    static void DrawTitle(int centerX, int y, int size)
    {
        const char* title = "KING OF THE HILL";
        int width = UiMeasure(title, size, FontStyle::Bold);

        UiText(title, centerX - width / 2 + 3, y + 4, size, Fade(COL_MAGENTA, 0.9f), FontStyle::Bold);
        UiText(title, centerX - width / 2, y, size, COL_CREAM, FontStyle::Bold);

        DrawCrown(centerX + width / 2 + 6, y + 4, size * 0.55f, COL_GOLD);
    }

    static void DrawMenu(int humanCount)
    {
        float time = static_cast<float>(GetTime());

        Rectangle band{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), 112.0f };
        DrawRectangleRec(band, COL_MAGENTA);
        DrawWaveBand(band, Fade(COL_INK_DEEP, 0.55f), time, 4);

        DrawTitle(SCREEN_WIDTH / 2, 150, 56);

        UiTextCentered("versión base  ·  hasta 4 jugadores, el resto lo juega la CPU",
                       SCREEN_WIDTH / 2, 222, 19, Fade(COL_CREAM, 0.75f));

        Rectangle card{ SCREEN_WIDTH / 2.0f - 330.0f, 258.0f, 660.0f, 236.0f };
        DrawSoftPanel(card, PANEL_COLOR, PANEL_BORDER, 0.06f);

        UiTextCentered("INTEGRANTES", SCREEN_WIDTH / 2, 278, 15, Fade(COL_GOLD, 0.85f), FontStyle::Bold);
        UiTextCentered(CREDITS_LINE, SCREEN_WIDTH / 2, 300, 18, COL_CREAM, FontStyle::Bold);

        const char* rules[] =
        {
            "En cada etapa elegís uno de los tres caminos: fácil, normal o difícil.",
            "Tirás un D20 y lo comparás con la dificultad del evento.",
            "Si lo superás elegís una bendición, si fallás recibís una maldición.",
            "Las cartas se activan solas en la tirada que indica su texto.",
            "En la cima se juega el torneo: combates al mejor de 3 tiradas."
        };

        for (int i = 0; i < 5; ++i)
        {
            int y = 338 + i * 28;
            DrawCircle(static_cast<int>(card.x) + 32, y + 8, 4.0f, COL_MAGENTA_LIT);
            UiText(rules[i], static_cast<int>(card.x) + 48, y, 16, Fade(COL_CREAM, 0.9f));
        }

        UiTextCentered("¿CUÁNTOS JUEGAN?", SCREEN_WIDTH / 2, 518, 15,
                       Fade(COL_GOLD, 0.85f), FontStyle::Bold);

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            int humans = i + 1;
            bool selected = (humans == humanCount);

            Rectangle box{ SCREEN_WIDTH / 2.0f - 170.0f + static_cast<float>(i) * 88.0f,
                           542.0f, 72.0f, 42.0f };

            DrawRectangleRounded(box, 0.3f, 6, selected ? Fade(COL_MAGENTA, 0.75f)
                                                        : Fade(COL_INK_DEEP, 0.6f));
            DrawRectangleRoundedLinesEx(box, 0.3f, 6, 2.0f,
                                        selected ? COL_GOLD : Fade(COL_CREAM, 0.3f));

            UiTextCentered(TextFormat("%i", humans), static_cast<int>(box.x + box.width * 0.5f),
                           static_cast<int>(box.y) + 6, 22,
                           selected ? COL_CREAM : Fade(COL_CREAM, 0.6f), FontStyle::Bold);
            UiTextCentered(humans == 1 ? "jugador" : "jugadores",
                           static_cast<int>(box.x + box.width * 0.5f),
                           static_cast<int>(box.y) + 28, 10, Fade(COL_CREAM, 0.6f));
        }

        int cpuCount = PLAYER_COUNT - humanCount;
        UiTextCentered(cpuCount > 0 ? TextFormat("[1-4] elegir  ·  los otros %i los juega la CPU", cpuCount)
                                    : "[1-4] elegir  ·  los cuatro en la misma computadora",
                       SCREEN_WIDTH / 2, 592, 14, Fade(COL_CREAM, 0.6f));

        float glow = 0.75f + 0.25f * Pulse(time, 2.4f);
        UiTextCentered("[ENTER] comenzar", SCREEN_WIDTH / 2, 618, 26, Fade(COL_GOLD, glow), FontStyle::Bold);
        UiTextCentered("[F11] pantalla completa   ·   [M] silenciar la música",
                       SCREEN_WIDTH / 2, 658, 13, Fade(COL_CREAM, 0.45f));
    }

    static void GetFinalRanking(const GameState& game, int ranking[])
    {
        const Duel& finalDuel  = game.duels[3];
        const Duel& losersDuel = game.duels[2];

        ranking[0] = finalDuel.winner;
        ranking[1] = (finalDuel.winner == finalDuel.playerA) ? finalDuel.playerB : finalDuel.playerA;
        ranking[2] = losersDuel.winner;
        ranking[3] = (losersDuel.winner == losersDuel.playerA) ? losersDuel.playerB : losersDuel.playerA;
    }

    static void DrawFinalScreen(const GameState& game)
    {
        float time = static_cast<float>(GetTime());

        Rectangle band{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), 96.0f };
        DrawRectangleRec(band, COL_MAGENTA);
        DrawWaveBand(band, Fade(COL_INK_DEEP, 0.55f), time, 3);

        int ranking[PLAYER_COUNT];
        GetFinalRanking(game, ranking);
        const Player& king = game.players[game.kingId];

        UiTextCentered("FIN DE LA PARTIDA", SCREEN_WIDTH / 2, 36, 19, Fade(COL_CREAM, 0.85f), FontStyle::Bold);

        DrawCrown(SCREEN_WIDTH / 2, 128, 64.0f + 4.0f * Pulse(time, 1.8f), COL_GOLD);

        UiTextCentered("REY DE LA COLINA", SCREEN_WIDTH / 2, 164, 46, COL_GOLD, FontStyle::Bold);
        UiTextCentered(TextFormat("JUGADOR %i", game.kingId + 1), SCREEN_WIDTH / 2, 218, 34,
                       king.color, FontStyle::Bold);

        Rectangle panel{ SCREEN_WIDTH / 2.0f - 340.0f, 276.0f, 680.0f, 226.0f };
        DrawSoftPanel(panel, PANEL_COLOR, PANEL_BORDER, 0.06f);

        UiText("POSICIONES FINALES", static_cast<int>(panel.x) + 24,
               static_cast<int>(panel.y) + 14, 16, COL_GOLD, FontStyle::Bold);

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = game.players[ranking[i]];
            int rowY = static_cast<int>(panel.y) + 48 + i * 42;

            Rectangle row{ panel.x + 12.0f, static_cast<float>(rowY) - 4.0f, panel.width - 24.0f, 38.0f };
            DrawRectangleRounded(row, 0.3f, 6, (i == 0) ? Fade(COL_GOLD, 0.14f) : Fade(COL_INK_DEEP, 0.35f));

            UiText(TextFormat("%iº", i + 1), static_cast<int>(panel.x) + 28, rowY + 4, 20,
                   (i == 0) ? COL_GOLD : COL_CREAM_DIM, FontStyle::Bold);

            DrawCircle(static_cast<int>(panel.x) + 78, rowY + 15, 12.0f, player.color);
            UiText(TextFormat("%i", player.id + 1), static_cast<int>(panel.x) + 74, rowY + 7, 15,
                   COL_INK_DEEP, FontStyle::Bold);

            UiText(PlayerLabel(player),
                   static_cast<int>(panel.x) + 104, rowY + 5, 19, COL_CREAM, FontStyle::Bold);

            UiText(TextFormat("%i bendiciones    %i maldiciones    %i sin usar",
                              CountCards(player, CardKind::Blessing),
                              CountCards(player, CardKind::Curse),
                              CountPendingCards(player)),
                   static_cast<int>(panel.x) + 250, rowY + 10, 14, Fade(COL_CREAM, 0.75f));
        }

        float glow = 0.75f + 0.25f * Pulse(time, 2.4f);
        UiTextCentered("[R] jugar de nuevo", SCREEN_WIDTH / 2, 528, 26, Fade(COL_GOLD, glow), FontStyle::Bold);
        UiTextCentered("[ESC] salir del juego", SCREEN_WIDTH / 2, 566, 17, Fade(COL_CREAM, 0.6f));
        UiTextCentered(CREDITS_LINE, SCREEN_WIDTH / 2, 648, 15, Fade(COL_CREAM, 0.45f));
    }

    void DrawGame(const GameState& game)
    {
        DrawBackdrop(static_cast<float>(GetTime()));

        if (game.phase == GamePhase::Menu)
        {
            DrawMenu(game.humanCount);
            return;
        }

        if (game.phase == GamePhase::Victory)
        {
            DrawFinalScreen(game);
            return;
        }

        DrawRectangle(0, 0, SCREEN_WIDTH, 76, Fade(COL_INK_DEEP, 0.55f));
        DrawTitle(206, 16, 26);
        UiText(CREDITS_LINE, 42, 52, 13, Fade(COL_CREAM, 0.42f));

        if (IsAudioMuted())
        {
            UiText("SIN SONIDO [M]", SCREEN_WIDTH - 210, 54, 13, Fade(COL_CREAM, 0.5f));
        }

        UiText("[F11] pantalla completa", 42, SCREEN_HEIGHT - 20, 12, Fade(COL_CREAM, 0.3f));

        UiText(TextFormat("Etapa %i / %i",
                          (game.currentStage < STAGE_COUNT ? game.currentStage + 1 : STAGE_COUNT),
                          STAGE_COUNT),
               SCREEN_WIDTH - 210, 24, 21, Fade(COL_CREAM, 0.85f), FontStyle::Bold);

        DrawBoard();

        bool exploring = (game.phase == GamePhase::PathSelection ||
                          game.phase == GamePhase::EventRoll ||
                          game.phase == GamePhase::EventChoice ||
                          game.phase == GamePhase::EventResult ||
                          game.phase == GamePhase::CardSelection);

        DrawPlayerTokens(game.players, exploring ? game.currentPlayer : -1);

        if (game.phase == GamePhase::EventResult) DrawBanner(game);

        DrawTurnPanel(game);

        bool busyPanel = (game.phase == GamePhase::CardSelection ||
                          game.phase == GamePhase::EventChoice ||
                          game.phase == GamePhase::CombatChoice);

        if (!busyPanel) DrawLog(game);

        DrawPlayerList(game);
    }
}
