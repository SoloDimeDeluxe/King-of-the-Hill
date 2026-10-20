#pragma once

#include "Types.h"
#include "Cards.h"
#include "Combat.h"
#include "Effects.h"
#include "Enemy.h"

#include <string>

namespace ReyesCuadra
{
    enum class GamePhase
    {
        Menu,
        TurnOrder,
        PathSelection,
        EventRoll,
        EventChoice,
        EventResult,
        EnemyIntro,
        EnemyTarget,
        EnemyChoice,
        EnemyResult,
        EnemyOutcome,
        CardSelection,
        Initiative,
        InitiativeResult,
        CombatIntro,
        CombatRoll,
        CombatChoice,
        CombatResult,
        Victory
    };

    constexpr int LOG_LINES = 4;

    struct GameState
    {
        GamePhase phase = GamePhase::Menu;

        Player  players[PLAYER_COUNT];
        DeckSet deckSet;

        int currentPlayer = 0;
        int currentStage  = 0;

        int      turnOrder[PLAYER_COUNT] = { 0, 1, 2, 3 };
        int      turnIndex               = 0;
        PathKind pathPreview             = PathKind::Normal;

        bool showCards  = false;
        int  cardsShown = 0;

        std::string bannerText;
        Color       bannerColor = Color{ 255, 255, 255, 255 };
        double      bannerTime  = 0.0;

        RollSession eventSession;
        DiceRoll    eventRoll;
        std::string eventEffects;
        int         eventDifficulty = 0;
        bool        eventSuccess    = false;

        int       cardOptions[CARD_OPTIONS] = {};
        int       cardOptionCount           = 0;
        CardKind  offerKind                 = CardKind::Blessing;
        CardLevel offerLevel                = CardLevel::Weak;
        int       rewardBump                = 0;

        Enemy       enemy;
        int         enemyTarget = 0;
        bool        enemyWon    = false;
        std::string enemyNote;

        int  initiativeOrder[PLAYER_COUNT] = {};
        Duel duels[DUEL_COUNT];
        int  currentDuel = 0;
        int  kingId      = -1;

        // El juego es de 1 jugador: los otros tres los maneja la CPU. El motor
        // soporta más humanos, pero la interfaz no deja cambiarlo.
        int    humanCount    = 1;
        double revealUntil   = 0.0;
        double resultTime    = 0.0;
        double cpuNextAction = 0.0;
        int    cpuSignature  = -1;

        std::string log[LOG_LINES];
    };

    void InitGame(GameState& game);
    void UpdateGame(GameState& game);
    void DrawGame(const GameState& game);
}
