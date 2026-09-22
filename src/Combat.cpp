#include "Combat.h"
#include "Effects.h"

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

                bool swapNeeded = (rollJ > rollI) ||
                                  (rollJ == rollI && RandomInt(0, 1) == 1);

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

    static int CompareRolls(const DiceRoll& a, const DiceRoll& b)
    {
        if (a.criticalHit && !b.criticalHit) return 1;
        if (b.criticalHit && !a.criticalHit) return -1;

        if (a.criticalFail && !b.criticalFail) return -1;
        if (b.criticalFail && !a.criticalFail) return 1;

        if (a.total > b.total) return 1;
        if (b.total > a.total) return -1;

        return 0;
    }

    static void AppendEffects(std::string& target, const std::string& names)
    {
        if (names.empty()) return;
        if (target.find(names) != std::string::npos) return;

        if (!target.empty()) target += ", ";
        target += names;
    }

    void ResolveDuel(Duel& duel, Player players[])
    {
        Player& playerA = players[duel.playerA];
        Player& playerB = players[duel.playerB];

        duel.rollsPlayed = 0;
        duel.winsA = 0;
        duel.winsB = 0;

        while (duel.rollsPlayed < MAX_COMBAT_ROLLS)
        {
            RollRequest request;
            request.isCombat  = true;
            request.rollIndex = duel.rollsPlayed;

            std::string namesA;
            std::string namesB;

            DiceRoll rollA = RollWithEffects(playerA, &playerB, request, namesA);
            DiceRoll rollB = RollWithEffects(playerB, &playerA, request, namesB);

            AppendEffects(duel.effectsA, namesA);
            AppendEffects(duel.effectsB, namesB);

            duel.rollsA[duel.rollsPlayed] = rollA;
            duel.rollsB[duel.rollsPlayed] = rollB;
            ++duel.rollsPlayed;

            int comparison = CompareRolls(rollA, rollB);
            if (comparison > 0)      ++duel.winsA;
            else if (comparison < 0) ++duel.winsB;

            bool roundsDone = (duel.rollsPlayed >= COMBAT_ROLLS);
            if (roundsDone && duel.winsA != duel.winsB) break;
        }

        duel.winner   = (duel.winsA >= duel.winsB) ? duel.playerA : duel.playerB;
        duel.resolved = true;

        EndCombatEffects(playerA);
        EndCombatEffects(playerB);
    }

    const char* GetDuelTitle(const Duel& duel)
    {
        if (duel.playerA < 0 || duel.playerB < 0) return "Esperando resultados...";

        return TextFormat("Jugador %i  vs  Jugador %i", duel.playerA + 1, duel.playerB + 1);
    }
}
