#pragma once

#include "Types.h"
#include "Cards.h"
#include "Dice.h"

#include <string>

namespace ReyesCuadra
{
    struct RollRequest
    {
        bool isCombat  = false;
        int  rollIndex = -1;
    };

    DiceRoll RollWithEffects(Player& self, Player* rival, const RollRequest& request,
                             std::string& appliedNames);

    std::string DescribePendingEffects(const Player& self, const Player* rival,
                                       const RollRequest& request);

    void EndCombatEffects(Player& player);
}
