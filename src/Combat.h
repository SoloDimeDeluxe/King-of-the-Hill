#pragma once

#include "Types.h"
#include "Dice.h"

#include <string>

namespace ReyesCuadra
{
    struct Duel
    {
        int playerA = -1;
        int playerB = -1;

        DiceRoll rollsA[MAX_COMBAT_ROLLS];
        DiceRoll rollsB[MAX_COMBAT_ROLLS];
        int      rollsPlayed = 0;

        int  winsA    = 0;
        int  winsB    = 0;
        int  winner   = -1;
        bool resolved = false;
        bool isFinal  = false;

        std::string effectsA;
        std::string effectsB;
    };

    void RollInitiative(Player players[], int order[]);
    void BuildBracket(const int order[], Duel duels[]);
    void BuildSecondRound(Duel duels[]);
    void ResolveDuel(Duel& duel, Player players[]);

    const char* GetDuelTitle(const Duel& duel);
}
