#include "Game.h"
#include "Board.h"
#include "Dice.h"
#include "Effects.h"
#include "Ui.h"

namespace ReyesCuadra
{
    constexpr float PANEL_Y       = 420.0f;
    constexpr float PANEL_H       = 270.0f;
    constexpr float LEFT_PANEL_X  = 40.0f;
    constexpr float LEFT_PANEL_W  = 620.0f;
    constexpr float RIGHT_PANEL_X = 680.0f;
    constexpr float RIGHT_PANEL_W = 560.0f;

    static const Color BACKGROUND_COLOR = Color{ 18, 20, 32, 255 };
    static const Color PANEL_COLOR      = Color{ 30, 34, 52, 255 };
    static const Color PANEL_BORDER     = Color{ 70, 78, 110, 255 };
    static const Color BLESSING_COLOR   = Color{ 90, 190, 120, 255 };
    static const Color CURSE_COLOR      = Color{ 220, 90, 90, 255 };

    static const char* CREDITS_LINE =
        "Federico Fernandez Soto   ·   Salvador Rosafioriti   ·   Valentin Reyes";

    static void AddLog(GameState& game, const char* text)
    {
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
            player.cards.clear();
        }

        InitDeckSet(game.deckSet);

        game.currentPlayer   = 0;
        game.currentStage    = 0;
        game.eventDifficulty = 0;
        game.eventSuccess    = false;
        game.eventEffects.clear();
        game.cardOptionCount = 0;
        game.currentDuel     = 0;
        game.kingId          = -1;

        for (int i = 0; i < DUEL_COUNT; ++i) game.duels[i] = Duel{};
        for (int i = 0; i < LOG_LINES; ++i)  game.log[i].clear();

        AddLog(game, "Comienza la partida: 4 jugadores en la base de la montaña.");
    }

    static void RollEvent(GameState& game)
    {
        Player& player = game.players[game.currentPlayer];

        RollRequest request;
        request.isCombat  = false;
        request.rollIndex = -1;

        game.eventDifficulty = GetEventDifficulty(player.path, game.currentStage);
        game.eventRoll       = RollWithEffects(player, nullptr, request, game.eventEffects);

        if (game.eventRoll.criticalHit)       game.eventSuccess = true;
        else if (game.eventRoll.criticalFail) game.eventSuccess = false;
        else                                  game.eventSuccess = (game.eventRoll.total >= game.eventDifficulty);

        AddLog(game, TextFormat("Jugador %i (%s): D20 %i -> %i vs %i -> %s",
                                game.currentPlayer + 1,
                                GetPathName(player.path),
                                game.eventRoll.natural,
                                game.eventRoll.total,
                                game.eventDifficulty,
                                game.eventSuccess ? "ÉXITO" : "FALLO"));

        game.phase = GamePhase::EventResult;
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

        CardInstance card;
        card.defId = game.cardOptions[chosenIndex];
        player.cards.push_back(card);

        ReturnCardOptions(game.deckSet, game.offerKind, game.offerLevel,
                          game.cardOptions, game.cardOptionCount, chosenIndex);

        AddLog(game, TextFormat("Jugador %i toma: %s", game.currentPlayer + 1,
                                GetCardDef(card.defId).name));

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

    void InitGame(GameState& game)
    {
        StartMatch(game);
        game.phase = GamePhase::Menu;
    }

    void UpdateGame(GameState& game)
    {
        switch (game.phase)
        {
            case GamePhase::Menu:
            {
                if (IsKeyPressed(KEY_ENTER))
                {
                    StartMatch(game);
                    game.phase = GamePhase::PathSelection;
                }
            } break;

            case GamePhase::PathSelection:
            {
                if (IsKeyPressed(KEY_ONE))   { game.players[game.currentPlayer].path = PathKind::Easy;   game.phase = GamePhase::EventRoll; }
                if (IsKeyPressed(KEY_TWO))   { game.players[game.currentPlayer].path = PathKind::Normal; game.phase = GamePhase::EventRoll; }
                if (IsKeyPressed(KEY_THREE)) { game.players[game.currentPlayer].path = PathKind::Hard;   game.phase = GamePhase::EventRoll; }
            } break;

            case GamePhase::EventRoll:
            {
                if (IsKeyPressed(KEY_SPACE)) RollEvent(game);
            } break;

            case GamePhase::EventResult:
            {
                if (IsKeyPressed(KEY_SPACE)) OfferCards(game);
            } break;

            case GamePhase::CardSelection:
            {
                if (game.cardOptionCount > 0 && IsKeyPressed(KEY_ONE))   TakeCard(game, 0);
                if (game.cardOptionCount > 1 && IsKeyPressed(KEY_TWO))   TakeCard(game, 1);
                if (game.cardOptionCount > 2 && IsKeyPressed(KEY_THREE)) TakeCard(game, 2);
            } break;

            case GamePhase::Initiative:
            {
                if (IsKeyPressed(KEY_SPACE))
                {
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
                    Duel& duel = game.duels[game.currentDuel];
                    ResolveDuel(duel, game.players);

                    AddLog(game, TextFormat("J%i %i - %i J%i -> gana J%i",
                                            duel.playerA + 1, duel.winsA,
                                            duel.winsB, duel.playerB + 1,
                                            duel.winner + 1));

                    game.phase = GamePhase::CombatResult;
                }
            } break;

            case GamePhase::CombatResult:
            {
                if (IsKeyPressed(KEY_SPACE))
                {
                    if (game.duels[game.currentDuel].isFinal)
                    {
                        game.kingId = game.duels[game.currentDuel].winner;
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
                    StartMatch(game);
                    game.phase = GamePhase::PathSelection;
                }
            } break;
        }
    }

    static void DrawPanel(Rectangle bounds, const char* title)
    {
        DrawRectangleRec(bounds, PANEL_COLOR);
        DrawRectangleLinesEx(bounds, 2.0f, PANEL_BORDER);
        UiText(title, static_cast<int>(bounds.x) + 16, static_cast<int>(bounds.y) + 12, 20, GOLD, FontStyle::Bold);
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

    static void DrawPlayerList(const GameState& game)
    {
        Rectangle bounds{ RIGHT_PANEL_X, PANEL_Y, RIGHT_PANEL_W, PANEL_H };
        DrawPanel(bounds, "JUGADORES");

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = game.players[i];

            int rowY = static_cast<int>(PANEL_Y) + 50 + i * 52;
            bool isActive = (i == game.currentPlayer) &&
                            (game.phase == GamePhase::PathSelection ||
                             game.phase == GamePhase::EventRoll ||
                             game.phase == GamePhase::EventResult ||
                             game.phase == GamePhase::CardSelection);

            if (isActive)
            {
                DrawRectangle(static_cast<int>(RIGHT_PANEL_X) + 8, rowY - 6,
                              static_cast<int>(RIGHT_PANEL_W) - 16, 46,
                              Fade(player.color, 0.15f));
            }

            DrawCircle(static_cast<int>(RIGHT_PANEL_X) + 30, rowY + 16, 12.0f, player.color);
            UiText(TextFormat("%i", i + 1), static_cast<int>(RIGHT_PANEL_X) + 26, rowY + 8, 16, BLACK, FontStyle::Bold);

            UiText(TextFormat("Jugador %i", i + 1),
                   static_cast<int>(RIGHT_PANEL_X) + 52, rowY, 18, RAYWHITE, FontStyle::Bold);

            UiText(TextFormat("Camino: %s", GetPathName(player.path)),
                   static_cast<int>(RIGHT_PANEL_X) + 52, rowY + 22, 14,
                   GetPathColor(player.path));

            UiText(TextFormat("%i bendiciones", CountCards(player, CardKind::Blessing)),
                   static_cast<int>(RIGHT_PANEL_X) + 220, rowY + 2, 14, BLESSING_COLOR);

            UiText(TextFormat("%i maldiciones", CountCards(player, CardKind::Curse)),
                   static_cast<int>(RIGHT_PANEL_X) + 220, rowY + 22, 14, CURSE_COLOR);

            UiText(TextFormat("%i sin usar", CountPendingCards(player)),
                   static_cast<int>(RIGHT_PANEL_X) + 380, rowY + 12, 16, LIGHTGRAY);
        }
    }

    static void DrawLog(const GameState& game)
    {
        for (int i = 0; i < LOG_LINES; ++i)
        {
            float alpha = 0.35f + 0.13f * static_cast<float>(i);

            UiText(game.log[i].c_str(),
                   static_cast<int>(LEFT_PANEL_X) + 16,
                   static_cast<int>(PANEL_Y) + 190 + i * 16,
                   13, Fade(RAYWHITE, alpha));
        }
    }

    static void DrawCardOptionsUI(const GameState& game)
    {
        for (int i = 0; i < game.cardOptionCount; ++i)
        {
            const CardDef& def = GetCardDef(game.cardOptions[i]);

            bool isBlessing = (def.kind == CardKind::Blessing);
            Color color = isBlessing ? BLESSING_COLOR : CURSE_COLOR;

            Rectangle bounds{ LEFT_PANEL_X + 14.0f, PANEL_Y + 46.0f + static_cast<float>(i) * 68.0f,
                              LEFT_PANEL_W - 28.0f, 62.0f };

            DrawRectangleRounded(bounds, 0.12f, 6, Fade(color, 0.16f));
            DrawRectangleLinesEx(bounds, 2.0f, Fade(color, 0.8f));

            UiText(TextFormat("[%i]", i + 1),
                   static_cast<int>(bounds.x) + 10, static_cast<int>(bounds.y) + 8, 16, RAYWHITE, FontStyle::Bold);

            UiText(def.name,
                   static_cast<int>(bounds.x) + 46, static_cast<int>(bounds.y) + 6, 18, color, FontStyle::Bold);

            UiTextWrapped(def.text,
                          static_cast<int>(bounds.x) + 46, static_cast<int>(bounds.y) + 28,
                          static_cast<int>(bounds.width) - 60, 13, LIGHTGRAY, 2);
        }
    }

    static void DrawTurnPanel(const GameState& game)
    {
        Rectangle bounds{ LEFT_PANEL_X, PANEL_Y, LEFT_PANEL_W, PANEL_H };

        int textX = static_cast<int>(LEFT_PANEL_X) + 16;
        int line1 = static_cast<int>(PANEL_Y) + 46;
        int line2 = static_cast<int>(PANEL_Y) + 74;
        int line3 = static_cast<int>(PANEL_Y) + 102;
        int line4 = static_cast<int>(PANEL_Y) + 138;

        const Player& player = game.players[game.currentPlayer];

        switch (game.phase)
        {
            case GamePhase::PathSelection:
            {
                DrawPanel(bounds, TextFormat("TURNO DEL JUGADOR %i  -  ETAPA %i / %i",
                                             game.currentPlayer + 1,
                                             game.currentStage + 1, STAGE_COUNT));

                UiText("Elegí tu camino para esta etapa:", textX, line1, 18, RAYWHITE);

                UiText(TextFormat("[1] FÁCIL     dificultad %i   ->  carta común",
                                  GetEventDifficulty(PathKind::Easy, game.currentStage)),
                       textX, line2, 16, GetPathColor(PathKind::Easy));

                UiText(TextFormat("[2] NORMAL    dificultad %i   ->  carta peligrosa",
                                  GetEventDifficulty(PathKind::Normal, game.currentStage)),
                       textX, line3, 16, GetPathColor(PathKind::Normal));

                UiText(TextFormat("[3] DIFÍCIL   dificultad %i   ->  carta épica",
                                  GetEventDifficulty(PathKind::Hard, game.currentStage)),
                       textX, line3 + 28, 16, GetPathColor(PathKind::Hard));
            } break;

            case GamePhase::EventRoll:
            {
                DrawPanel(bounds, TextFormat("JUGADOR %i  -  CAMINO %s",
                                             game.currentPlayer + 1, GetPathName(player.path)));

                UiText(TextFormat("Dificultad del evento: %i",
                                  GetEventDifficulty(player.path, game.currentStage)),
                       textX, line1, 18, RAYWHITE);

                RollRequest request;
                request.isCombat  = false;
                request.rollIndex = -1;

                std::string pending = DescribePendingEffects(player, nullptr, request);

                UiText("Cartas que se activan en esta tirada:", textX, line2, 14, LIGHTGRAY);
                UiTextWrapped(pending.empty() ? "ninguna" : pending.c_str(),
                              textX, line2 + 20, static_cast<int>(LEFT_PANEL_W) - 32, 14,
                              pending.empty() ? GRAY : GOLD, 2);

                UiText("[ESPACIO] tirar el D20", textX, line4 + 14, 20, GOLD, FontStyle::Bold);
            } break;

            case GamePhase::EventResult:
            {
                DrawPanel(bounds, game.eventSuccess ? "EVENTO SUPERADO" : "EVENTO FALLIDO");

                UiText(TextFormat("D20: %i", game.eventRoll.natural), textX, line1, 26,
                       game.eventSuccess ? GREEN : RED, FontStyle::Bold);

                UiText(TextFormat("Total: %i        Dificultad: %i",
                                  game.eventRoll.total, game.eventDifficulty),
                       textX + 120, line1 + 6, 18, RAYWHITE);

                if (game.eventRoll.criticalHit)
                {
                    UiText("20 NATURAL: éxito crítico, se ignoran los modificadores",
                           textX, line2 + 8, 15, GOLD);
                }
                else if (game.eventRoll.criticalFail)
                {
                    UiText("1 NATURAL: fallo crítico, se ignoran los modificadores",
                           textX, line2 + 8, 15, ORANGE);
                }
                else if (!game.eventEffects.empty())
                {
                    UiText("Se aplicaron:", textX, line2 + 8, 14, LIGHTGRAY);
                    UiTextWrapped(game.eventEffects.c_str(), textX + 96, line2 + 8,
                                  static_cast<int>(LEFT_PANEL_W) - 130, 14, GOLD, 2);
                }

                UiText(game.eventSuccess ? "[ESPACIO] elegir bendición"
                                         : "[ESPACIO] recibir maldición",
                       textX, line4 + 14, 20, GOLD, FontStyle::Bold);
            } break;

            case GamePhase::CardSelection:
            {
                DrawPanel(bounds, game.offerKind == CardKind::Blessing
                                  ? TextFormat("JUGADOR %i: elegí una bendición %s",
                                               game.currentPlayer + 1, GetCardLevelName(game.offerLevel))
                                  : TextFormat("JUGADOR %i: elegí una maldición %s",
                                               game.currentPlayer + 1, GetCardLevelName(game.offerLevel)));

                DrawCardOptionsUI(game);
            } break;

            case GamePhase::Initiative:
            {
                DrawPanel(bounds, "FASE DE COMBATE");
                UiText("Los cuatro jugadores llegaron a la cima.", textX, line1, 18, RAYWHITE);
                UiText("La iniciativa define los cruces del torneo.", textX, line2, 16, LIGHTGRAY);
                UiText("[ESPACIO] tirar iniciativa", textX, line4 + 14, 20, GOLD, FontStyle::Bold);
            } break;

            case GamePhase::InitiativeResult:
            {
                DrawPanel(bounds, "CRUCES DEL TORNEO");

                UiText(TextFormat("Semifinal 1:  Jugador %i  vs  Jugador %i",
                                  game.duels[0].playerA + 1, game.duels[0].playerB + 1),
                       textX, line1, 18, RAYWHITE);

                UiText(TextFormat("Semifinal 2:  Jugador %i  vs  Jugador %i",
                                  game.duels[1].playerA + 1, game.duels[1].playerB + 1),
                       textX, line2, 18, RAYWHITE);

                UiText("Después juegan los dos perdedores y, al final, los dos ganadores.",
                       textX, line3, 14, LIGHTGRAY);

                UiText("[ESPACIO] continuar", textX, line4 + 14, 20, GOLD, FontStyle::Bold);
            } break;

            case GamePhase::CombatIntro:
            {
                const Duel& duel = game.duels[game.currentDuel];

                DrawPanel(bounds, duel.isFinal ? "BATALLA FINAL" : "COMBATE");

                UiText(GetDuelTitle(duel), textX, line1, 24, RAYWHITE, FontStyle::Bold);

                UiText(TextFormat("Al mejor de %i tiradas.", COMBAT_ROLLS),
                       textX, line2 + 6, 16, LIGHTGRAY);

                UiText(TextFormat("Cartas sin usar:  J%i tiene %i    J%i tiene %i",
                                  duel.playerA + 1, CountPendingCards(game.players[duel.playerA]),
                                  duel.playerB + 1, CountPendingCards(game.players[duel.playerB])),
                       textX, line3 + 6, 16, LIGHTGRAY);

                UiText("[ESPACIO] resolver el combate", textX, line4 + 14, 20, GOLD, FontStyle::Bold);
            } break;

            case GamePhase::CombatResult:
            {
                const Duel& duel = game.duels[game.currentDuel];

                DrawPanel(bounds, TextFormat("GANA EL JUGADOR %i   (%i - %i)",
                                             duel.winner + 1,
                                             (duel.winner == duel.playerA) ? duel.winsA : duel.winsB,
                                             (duel.winner == duel.playerA) ? duel.winsB : duel.winsA));

                std::string rollsA = DescribeRolls(duel.rollsA, duel.rollsPlayed);
                std::string rollsB = DescribeRolls(duel.rollsB, duel.rollsPlayed);

                UiText(TextFormat("Jugador %i:", duel.playerA + 1), textX, line1, 17,
                       game.players[duel.playerA].color, FontStyle::Bold);
                UiText(rollsA.c_str(), textX + 110, line1, 17, RAYWHITE);

                UiText(TextFormat("Jugador %i:", duel.playerB + 1), textX, line2, 17,
                       game.players[duel.playerB].color, FontStyle::Bold);
                UiText(rollsB.c_str(), textX + 110, line2, 17, RAYWHITE);

                UiText("! = 20 natural      ? = 1 natural", textX, line3, 13, GRAY);

                std::string effects = duel.effectsA;
                if (!duel.effectsB.empty())
                {
                    if (!effects.empty()) effects += ", ";
                    effects += duel.effectsB;
                }

                if (!effects.empty())
                {
                    UiTextWrapped(TextFormat("Cartas aplicadas: %s", effects.c_str()),
                                  textX, line3 + 22, static_cast<int>(LEFT_PANEL_W) - 32, 13, GOLD, 2);
                }

                UiText("[ESPACIO] continuar", textX, line4 + 14, 20, GOLD, FontStyle::Bold);
            } break;

            default: break;
        }
    }

    static void DrawMenu()
    {
        const char* title = "KING OF THE HILL";
        UiTextCentered(title, SCREEN_WIDTH / 2, 150, 60, GOLD, FontStyle::Bold);

        UiTextCentered("versión base  ·  4 jugadores en la misma computadora",
                       SCREEN_WIDTH / 2, 224, 20, LIGHTGRAY);

        UiTextCentered("INTEGRANTES", SCREEN_WIDTH / 2, 268, 16, Fade(GOLD, 0.75f), FontStyle::Bold);
        UiTextCentered(CREDITS_LINE, SCREEN_WIDTH / 2, 292, 20, RAYWHITE, FontStyle::Bold);

        const char* rules[] =
        {
            "1. En cada etapa elegís uno de los tres caminos: fácil, normal o difícil.",
            "2. Tirás un D20 y lo comparás con la dificultad del evento.",
            "3. Si lo superás elegís una bendición, si fallás recibís una maldición.",
            "4. Las cartas se activan solas en la tirada que indica su texto.",
            "5. Al llegar a la cima se juega el torneo: semifinales y batalla final."
        };

        for (int i = 0; i < 5; ++i)
        {
            UiText(rules[i], 250, 348 + i * 30, 18, RAYWHITE);
        }

        int implemented = GetCardDefCount();
        int percent = (implemented * 100) / CARD_DESIGN_TOTAL;

        UiTextCentered(TextFormat("Cartas implementadas: %i de %i  (%i%%)",
                                  implemented, CARD_DESIGN_TOTAL, percent),
                       SCREEN_WIDTH / 2, 520, 16, Fade(RAYWHITE, 0.6f));

        UiTextCentered("[ENTER] comenzar", SCREEN_WIDTH / 2, 570, 28, GOLD, FontStyle::Bold);
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
        int ranking[PLAYER_COUNT];
        GetFinalRanking(game, ranking);

        const Player& king = game.players[game.kingId];

        UiTextCentered("FIN DE LA PARTIDA", SCREEN_WIDTH / 2, 56, 20, LIGHTGRAY);
        UiTextCentered("REY DE LA COLINA", SCREEN_WIDTH / 2, 92, 56, GOLD, FontStyle::Bold);
        UiTextCentered(TextFormat("JUGADOR %i", game.kingId + 1), SCREEN_WIDTH / 2, 168, 40,
                       king.color, FontStyle::Bold);

        Rectangle panel{ SCREEN_WIDTH / 2.0f - 340.0f, 250.0f, 680.0f, 250.0f };
        DrawRectangleRounded(panel, 0.06f, 8, PANEL_COLOR);
        DrawRectangleLinesEx(panel, 2.0f, PANEL_BORDER);

        UiText("POSICIONES FINALES", static_cast<int>(panel.x) + 24,
               static_cast<int>(panel.y) + 16, 18, GOLD, FontStyle::Bold);

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = game.players[ranking[i]];
            int rowY = static_cast<int>(panel.y) + 58 + i * 46;

            if (i == 0)
            {
                DrawRectangle(static_cast<int>(panel.x) + 12, rowY - 6,
                              static_cast<int>(panel.width) - 24, 42, Fade(GOLD, 0.12f));
            }

            UiText(TextFormat("%iº", i + 1), static_cast<int>(panel.x) + 26, rowY + 2, 22,
                   (i == 0) ? GOLD : LIGHTGRAY, FontStyle::Bold);

            DrawCircle(static_cast<int>(panel.x) + 76, rowY + 16, 12.0f, player.color);
            UiText(TextFormat("%i", player.id + 1), static_cast<int>(panel.x) + 72, rowY + 8, 16,
                   BLACK, FontStyle::Bold);

            UiText(TextFormat("Jugador %i", player.id + 1),
                   static_cast<int>(panel.x) + 104, rowY + 4, 20, RAYWHITE, FontStyle::Bold);

            UiText(TextFormat("%i bendiciones    %i maldiciones    %i cartas sin usar",
                              CountCards(player, CardKind::Blessing),
                              CountCards(player, CardKind::Curse),
                              CountPendingCards(player)),
                   static_cast<int>(panel.x) + 250, rowY + 10, 15, LIGHTGRAY);
        }

        UiTextCentered("[R] jugar de nuevo", SCREEN_WIDTH / 2, 540, 28, GOLD, FontStyle::Bold);
        UiTextCentered("[ESC] salir del juego", SCREEN_WIDTH / 2, 584, 20, LIGHTGRAY);

        UiTextCentered(CREDITS_LINE, SCREEN_WIDTH / 2, 650, 16, Fade(RAYWHITE, 0.5f));
    }

    void DrawGame(const GameState& game)
    {
        ClearBackground(BACKGROUND_COLOR);

        if (game.phase == GamePhase::Menu)
        {
            DrawMenu();
            return;
        }

        if (game.phase == GamePhase::Victory)
        {
            DrawFinalScreen(game);
            return;
        }

        UiText("KING OF THE HILL", 40, 20, 30, GOLD, FontStyle::Bold);
        UiText(CREDITS_LINE, 42, 56, 14, Fade(RAYWHITE, 0.45f));

        UiText(TextFormat("Etapa %i / %i",
                          (game.currentStage < STAGE_COUNT ? game.currentStage + 1 : STAGE_COUNT),
                          STAGE_COUNT),
               SCREEN_WIDTH - 200, 32, 22, LIGHTGRAY);

        DrawBoard();

        bool exploring = (game.phase == GamePhase::PathSelection ||
                          game.phase == GamePhase::EventRoll ||
                          game.phase == GamePhase::EventResult ||
                          game.phase == GamePhase::CardSelection);

        DrawPlayerTokens(game.players, exploring ? game.currentPlayer : -1);

        DrawTurnPanel(game);

        if (game.phase != GamePhase::CardSelection) DrawLog(game);

        DrawPlayerList(game);
    }
}
