#include "Cpu.h"
#include "Cards.h"
#include "Dice.h"

namespace ReyesCuadra
{
    // Cuánto le conviene a la CPU tener esta carta. Positivo es bueno para ella.
    static int ScoreCard(const CardDef& def)
    {
        int score = 0;

        switch (def.effect)
        {
            case EffectKind::FlatBonus:
                score = def.targetsRival ? (-def.paramA) : def.paramA;
                break;
            case EffectKind::MaxResult:
                score = def.targetsRival ? (20 - def.paramA) / 2 : -(20 - def.paramA) / 2;
                break;
            case EffectKind::ThresholdBonus:      score = def.paramC / 2; break;
            case EffectKind::ParityBonus:         score = (def.paramB + def.paramC) / 2; break;
            case EffectKind::MultiplyCapped:      score = 4; break;
            case EffectKind::DivideRoundUp:       score = -5; break;
            case EffectKind::ExtraDice:           score = 4; break;
            case EffectKind::ValueConvert:        score = (def.paramB - def.paramA) / 4; break;
            case EffectKind::SafePlay:            score = 2; break;
            case EffectKind::FibonacciBonus:      score = 1; break;
            case EffectKind::HistoryBonus:        score = def.paramC / 2; break;
            case EffectKind::WinRule:             score = (def.kind == CardKind::Blessing) ? 2 : -2; break;
            case EffectKind::PlayerChoice:        score = 3; break;
            case EffectKind::TwistRival:          score = 3; break;
            case EffectKind::ModifyIncoming:      score = (def.paramA == 0) ? 3 : -3; break;
            case EffectKind::BlessingPenalty:     score = -3; break;
            case EffectKind::BlessingRestriction: score = -3; break;
            case EffectKind::TwoPhaseFlat:        score = (def.paramA + def.paramB) / 2; break;
            case EffectKind::CombatRolls:         score = -2; break;
            case EffectKind::HandAction:          score = 5; break;
            case EffectKind::NoRewardOnWin:       score = -2; break;

            case EffectKind::DiceControl:
            {
                if (def.paramA == 0)      score = 4;
                else if (def.paramA == 1) score = -3;
                else if (def.paramA == 2) score = -3;
                else if (def.paramA == 3) score = -2;
                else                      score = 1;
            } break;
        }

        if (score >  20) score =  20;
        if (score < -20) score = -20;
        return score;
    }

    PathKind CpuChoosePath(const Player& player, int stage)
    {
        // Una de cada cinco veces elige al azar, para que no sea siempre igual.
        if (RandomInt(0, 4) == 0) return static_cast<PathKind>(RandomInt(0, PATH_COUNT - 1));

        int blessings = CountCards(player, CardKind::Blessing);
        int curses    = CountCards(player, CardKind::Curse);

        // Si viene cargada de maldiciones juega a lo seguro.
        if (curses > blessings + 1) return PathKind::Easy;

        // En las últimas etapas se juega el todo por el todo: quiere cartas épicas.
        if (stage >= STAGE_COUNT - 2) return PathKind::Hard;

        // Si le está yendo bien se anima al camino difícil.
        if (blessings >= curses + 2) return PathKind::Hard;

        return PathKind::Normal;
    }

    int CpuChooseCard(const int options[], int optionCount)
    {
        if (optionCount <= 0) return 0;

        int best = 0;
        int bestScore = ScoreCard(GetCardDef(options[0]));

        for (int i = 1; i < optionCount; ++i)
        {
            int score = ScoreCard(GetCardDef(options[i]));
            if (score > bestScore) { bestScore = score; best = i; }
        }

        return best;
    }

    int CpuChooseOption(const PendingChoice& choice)
    {
        int count = static_cast<int>(choice.options.size());
        if (count <= 0) return 0;

        int best = 0;
        for (int i = 1; i < count; ++i)
        {
            if (choice.options[i].value > choice.options[best].value) best = i;
        }

        return best;
    }

    // Primero asegura la victoria: va por las partes que cuentan y que menos
    // dificultad piden. Sólo si ya le sobran turnos prueba suerte con la parte
    // de bonus, que es la que da la recompensa grande.
    int CpuChoosePart(const Player& player, const Enemy& enemy, int stage)
    {
        PathKind path = player.path;
        int needed = enemy.neededDowns - CountDownedCounting(enemy);

        int woundsToWin = 0;
        for (int i = 0; i < enemy.partCount; ++i)
        {
            if (enemy.parts[i].counts && enemy.parts[i].woundsLeft > 0)
            {
                woundsToWin += enemy.parts[i].woundsLeft;
            }
        }

        bool comfortable = (needed <= 0) ||
                           (enemy.turnsLeft > woundsToWin + 1);

        if (comfortable)
        {
            for (int i = 0; i < enemy.partCount; ++i)
            {
                if (!enemy.parts[i].counts && enemy.parts[i].woundsLeft > 0) return i;
            }
        }

        int best      = -1;
        int bestValue = 0;

        for (int i = 0; i < enemy.partCount; ++i)
        {
            if (enemy.parts[i].woundsLeft <= 0) continue;
            if (!enemy.parts[i].counts) continue;

            int value = GetPartDifficulty(enemy, i, path, stage) * 4 + enemy.parts[i].woundsLeft;

            if (best < 0 || value < bestValue) { best = i; bestValue = value; }
        }

        return (best >= 0) ? best : FirstAlivePart(enemy);
    }

    // Las batallas opcionales las toma si viene entera: si junta más
    // maldiciones que bendiciones prefiere no arriesgar.
    // Qué ítem le conviene gastar ahora, o -1 si ninguno. Mide la tirada que
    // está por hacer contra la dificultad que tiene enfrente.
    int CpuChooseItem(const Player& player, int difficulty, bool isFight, int woundsLeft)
    {
        int charm = -1, whetstone = -1, potion = -1;

        for (int i = 0; i < static_cast<int>(player.items.size()); ++i)
        {
            switch (player.items[i])
            {
                case ItemId::Charm:     if (charm     < 0) charm     = i; break;
                case ItemId::Whetstone: if (whetstone < 0) whetstone = i; break;
                case ItemId::Potion:    if (potion    < 0) potion    = i; break;
                default: break;
            }
        }

        // La piedra sólo vale la pena si la parte aguanta más de un golpe.
        if (isFight && whetstone >= 0 && woundsLeft >= 2) return whetstone;

        // El amuleto, cuando la tirada está realmente difícil.
        if (charm >= 0 && difficulty >= 13) return charm;

        // La poción, cuando ya arrastra más maldiciones que bendiciones.
        if (potion >= 0 && CountCards(player, CardKind::Curse) > CountCards(player, CardKind::Blessing))
        {
            return potion;
        }

        return -1;
    }

    bool CpuTakesOptionalFight(const Player& player)
    {
        int blessings = CountCards(player, CardKind::Blessing);
        int curses    = CountCards(player, CardKind::Curse);

        if (curses > blessings) return RandomInt(0, 3) == 0;

        return RandomInt(0, 4) != 0;
    }

    // Con el combate ya ganado, sólo sigue si le sobran turnos para tumbar la
    // parte de bonus; si no, se lleva la recompensa y no arriesga maldiciones.
    bool CpuKeepsFighting(const Enemy& enemy)
    {
        for (int i = 0; i < enemy.partCount; ++i)
        {
            if (enemy.parts[i].counts) continue;
            if (enemy.parts[i].woundsLeft <= 0) continue;

            return enemy.turnsLeft > enemy.parts[i].woundsLeft;
        }

        return false;
    }
}
