#pragma once

#include "Types.h"
#include "Cards.h"
#include "Combat.h"

#include <string>

namespace ReyesCuadra
{
    enum class GamePhase
    {
        Menu,
        PathSelection,
        EventRoll,
        EventResult,
        CardSelection,
        Initiative,
        InitiativeResult,
        CombatIntro,
        CombatResult,
        Victory
    };

    constexpr int LOG_LINES = 5;

    struct GameState
    {
        GamePhase phase = GamePhase::Menu;

        Player  players[PLAYER_COUNT];
        DeckSet deckSet;

        int currentPlayer = 0;
        int currentStage  = 0;

        DiceRoll    eventRoll;
        std::string eventEffects;
        int         eventDifficulty = 0;
        bool        eventSuccess    = false;

        int       cardOptions[CARD_OPTIONS] = {};
        int       cardOptionCount           = 0;
        CardKind  offerKind                 = CardKind::Blessing;
        CardLevel offerLevel                = CardLevel::Weak;

        int  initiativeOrder[PLAYER_COUNT] = {};
        Duel duels[DUEL_COUNT];
        int  currentDuel = 0;
        int  kingId      = -1;

        std::string log[LOG_LINES];
    };

    void InitGame(GameState& game);
    void UpdateGame(GameState& game);
    void DrawGame(const GameState& game);
}
