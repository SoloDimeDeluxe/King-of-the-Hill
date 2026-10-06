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

    struct CombatMemory
    {
        int  rollsPlayed          = 0;
        int  outcomes[MAX_COMBAT_ROLLS] = {};
        int  wins                 = 0;
        int  losses               = 0;
        int  blessingsUsed        = 0;
        bool blessingUsedLastRoll = false;
        bool blessingUsedEver     = false;
        int  savedResult          = 0;
        bool saveArmed            = false;
        int  rollsLimit           = COMBAT_ROLLS;
    };

    struct ChoiceOption
    {
        std::string label;
        int         value = 0;
    };

    struct PendingChoice
    {
        bool                      active = false;
        std::string               title;
        std::vector<ChoiceOption> options;
    };

    struct RollSession
    {
        Player*       self        = nullptr;
        Player*       rival       = nullptr;
        CombatMemory* memory      = nullptr;
        RollRequest   request;

        int           step        = 0;
        bool          done        = false;
        PendingChoice choice;

        DiceRoll      roll;
        std::string   appliedNames;

        int  diceValues[MAX_DICE] = {};
        int  diceCount            = 1;
        int  scaleSteps           = 0;
        int  betChoice            = -1;
        int  parityGuess          = -1;
        int  adjustChoice         = 0;
    };

    struct WinRules
    {
        bool lowestWins       = false;
        bool closestTo        = false;
        int  closestTarget    = 10;
        int  rivalNeedsAtLeast = 0;
        bool tieIsLoss        = false;
    };

    void BeginRoll(RollSession& session, Player& self, Player* rival,
                   const RollRequest& request, CombatMemory* memory);
    void AdvanceRoll(RollSession& session, int chosenOption);

    DiceRoll RollSimple(Player& self, Player* rival, const RollRequest& request,
                        std::string& appliedNames, CombatMemory* memory);

    std::string DescribePendingEffects(const Player& self, const Player* rival,
                                       const RollRequest& request);

    WinRules GatherWinRules(const Player& self, const Player& rival, int rollIndex);

    void StartCombatEffects(Player& player, Player& rival, CombatMemory& memory);
    void EndCombatEffects(Player& player);
    void ApplyOnAcquire(Player& player, Player others[], int playerCount, int defId,
                        std::string& message);
    bool BlocksCombatReward(const Player& player);
    void ClaimCombatReward(Player& winner, Player& loser, std::string& message);
}
