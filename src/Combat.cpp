#include "Combat.h"

namespace ReyesCuadra
{
    void RollInitiative(Player players[], int order[])
    {
        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            players[i].initiativeRoll = RollRawD20();
            order[i] = i;
        }

        for (int i = 0; i < PLAYER_COUNT - 1; ++i)
        {
            for (int j = i + 1; j < PLAYER_COUNT; ++j)
            {
                int rollI = players[order[i]].initiativeRoll;
                int rollJ = players[order[j]].initiativeRoll;

                bool swapNeeded = (rollJ > rollI) || (rollJ == rollI && RandomInt(0, 1) == 1);

                if (swapNeeded)
                {
                    int temp = order[i];
                    order[i] = order[j];
                    order[j] = temp;
                }
            }
        }
    }

    static Duel MakeDuel(int playerA, int playerB, bool isFinal)
    {
        Duel duel;
        duel.playerA = playerA;
        duel.playerB = playerB;
        duel.isFinal = isFinal;
        return duel;
    }

    void BuildBracket(const int order[], Duel duels[])
    {
        duels[0] = MakeDuel(order[0], order[1], false);
        duels[1] = MakeDuel(order[2], order[3], false);
        duels[2] = MakeDuel(-1, -1, false);
        duels[3] = MakeDuel(-1, -1, true);
    }

    void BuildSecondRound(Duel duels[])
    {
        int loserA = (duels[0].winner == duels[0].playerA) ? duels[0].playerB : duels[0].playerA;
        int loserB = (duels[1].winner == duels[1].playerA) ? duels[1].playerB : duels[1].playerA;

        duels[2] = MakeDuel(loserA, loserB, false);
        duels[3] = MakeDuel(duels[0].winner, duels[1].winner, true);
    }

    static int CompareRolls(const DiceRoll& a, const DiceRoll& b, const WinRules& rulesA,
                            const WinRules& rulesB)
    {
        if (a.criticalHit && !b.criticalHit) return 1;
        if (b.criticalHit && !a.criticalHit) return -1;
        if (a.criticalFail && !b.criticalFail) return -1;
        if (b.criticalFail && !a.criticalFail) return 1;

        int valueA = a.total;
        int valueB = b.total;

        if (rulesA.closestTo || rulesB.closestTo)
        {
            int target = rulesA.closestTo ? rulesA.closestTarget : rulesB.closestTarget;
            int distanceA = (valueA > target) ? valueA - target : target - valueA;
            int distanceB = (valueB > target) ? valueB - target : target - valueB;

            if (distanceA < distanceB) return 1;
            if (distanceB < distanceA) return -1;
            return 0;
        }

        bool lowest = rulesA.lowestWins || rulesB.lowestWins;

        if (rulesA.rivalNeedsAtLeast > 0 && valueA < rulesA.rivalNeedsAtLeast) return -1;
        if (rulesB.rivalNeedsAtLeast > 0 && valueB < rulesB.rivalNeedsAtLeast) return 1;

        if (valueA == valueB)
        {
            if (rulesA.tieIsLoss) return -1;
            if (rulesB.tieIsLoss) return 1;
            return 0;
        }

        bool aWins = lowest ? (valueA < valueB) : (valueA > valueB);
        return aWins ? 1 : -1;
    }

    static void AppendEffects(std::string& target, const std::string& names)
    {
        if (names.empty()) return;
        if (target.find(names) != std::string::npos) return;
        if (!target.empty()) target += ", ";
        target += names;
    }

    void StartDuel(Duel& duel, Player players[])
    {
        Player& playerA = players[duel.playerA];
        Player& playerB = players[duel.playerB];

        StartCombatEffects(playerA, playerB, duel.memoryA);
        StartCombatEffects(playerB, playerA, duel.memoryB);

        duel.started       = true;
        duel.rollsPlayed   = 0;
        duel.winsA         = 0;
        duel.winsB         = 0;
        duel.activeSide    = 0;
        duel.sessionActive = false;
    }

    bool DuelChoicePending(const Duel& duel)
    {
        return duel.sessionActive && duel.session.choice.active;
    }

    static int CombatLength(const Duel& duel)
    {
        int length = (duel.memoryA.rollsLimit < duel.memoryB.rollsLimit)
                     ? duel.memoryA.rollsLimit : duel.memoryB.rollsLimit;
        if (length < 1) length = 1;
        return length;
    }

    static void RecordOutcome(Duel& duel, int result)
    {
        int index = duel.rollsPlayed;

        duel.outcomes[index] = result;

        duel.memoryA.outcomes[index] = result;
        duel.memoryB.outcomes[index] = -result;

        if (result > 0)      { ++duel.winsA; ++duel.memoryA.wins; ++duel.memoryB.losses; }
        else if (result < 0) { ++duel.winsB; ++duel.memoryB.wins; ++duel.memoryA.losses; }

        ++duel.rollsPlayed;
        duel.memoryA.rollsPlayed = duel.rollsPlayed;
        duel.memoryB.rollsPlayed = duel.rollsPlayed;
    }

    void StepDuel(Duel& duel, Player players[], int chosenOption)
    {
        if (duel.resolved) return;

        Player& playerA = players[duel.playerA];
        Player& playerB = players[duel.playerB];

        if (!duel.started) StartDuel(duel, players);

        if (!duel.sessionActive)
        {
            RollRequest request;
            request.isCombat  = true;
            request.rollIndex = duel.rollsPlayed;

            if (duel.activeSide == 0)
            {
                BeginRoll(duel.session, playerA, &playerB, request, &duel.memoryA);
            }
            else
            {
                BeginRoll(duel.session, playerB, &playerA, request, &duel.memoryB);
            }

            duel.sessionActive = true;
        }
        else
        {
            AdvanceRoll(duel.session, chosenOption);
        }

        if (!duel.session.done) return;

        if (duel.activeSide == 0)
        {
            duel.rollsA[duel.rollsPlayed] = duel.session.roll;
            AppendEffects(duel.effectsA, duel.session.appliedNames);
            duel.activeSide    = 1;
            duel.sessionActive = false;
            return;
        }

        duel.rollsB[duel.rollsPlayed] = duel.session.roll;
        AppendEffects(duel.effectsB, duel.session.appliedNames);
        duel.sessionActive = false;
        duel.activeSide    = 0;

        WinRules rulesA = GatherWinRules(playerA, playerB, duel.rollsPlayed);
        WinRules rulesB = GatherWinRules(playerB, playerA, duel.rollsPlayed);

        int result = CompareRolls(duel.rollsA[duel.rollsPlayed], duel.rollsB[duel.rollsPlayed],
                                  rulesA, rulesB);
        RecordOutcome(duel, result);

        bool roundsDone = (duel.rollsPlayed >= CombatLength(duel));
        bool decided    = (duel.winsA != duel.winsB);

        if ((roundsDone && decided) || duel.rollsPlayed >= MAX_COMBAT_ROLLS)
        {
            duel.winner   = (duel.winsA >= duel.winsB) ? duel.playerA : duel.playerB;
            duel.resolved = true;

            Player& winner = players[duel.winner];
            Player& loser  = players[(duel.winner == duel.playerA) ? duel.playerB : duel.playerA];
            ClaimCombatReward(winner, loser, duel.rewardMessage);

            EndCombatEffects(playerA);
            EndCombatEffects(playerB);
        }
    }

    const char* GetDuelTitle(const Duel& duel)
    {
        if (duel.playerA < 0 || duel.playerB < 0) return "Esperando resultados...";

        return TextFormat("Jugador %i  vs  Jugador %i", duel.playerA + 1, duel.playerB + 1);
    }
}
