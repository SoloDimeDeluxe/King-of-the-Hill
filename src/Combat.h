#pragma once

#include "Types.h"
#include "Dice.h"
#include "Effects.h"

#include <string>

namespace ReyesCuadra
{
    struct Duel
    {
        int playerA = -1;
        int playerB = -1;

        DiceRoll rollsA[MAX_COMBAT_ROLLS];
        DiceRoll rollsB[MAX_COMBAT_ROLLS];
        int      outcomes[MAX_COMBAT_ROLLS] = {};
        int      rollsPlayed = 0;

        int  winsA    = 0;
        int  winsB    = 0;
        int  winner   = -1;
        bool started  = false;
        bool resolved = false;
        bool isFinal  = false;

        CombatMemory memoryA;
        CombatMemory memoryB;
        RollSession  session;
        bool         sessionActive = false;
        int          activeSide    = 0;

        std::string effectsA;
        std::string effectsB;
        std::string rewardMessage;
    };

    void RollInitiative(Player players[], int order[]);
    void BuildBracket(const int order[], Duel duels[]);
    void BuildSecondRound(Duel duels[]);

    void StartDuel(Duel& duel, Player players[]);
    void StepDuel(Duel& duel, Player players[], int chosenOption);
    bool DuelChoicePending(const Duel& duel);

    const char* GetDuelTitle(const Duel& duel);
}
