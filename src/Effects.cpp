#include "Effects.h"

#include <algorithm>
#include <vector>

namespace ReyesCuadra
{
    struct ActiveEffect
    {
        CardInstance*  instance  = nullptr;
        const CardDef* def       = nullptr;
        bool           fromRival = false;
    };

    struct Contribution
    {
        const CardDef* def        = nullptr;
        int            delta      = 0;
        bool           myBlessing = false;
        bool           enabled    = true;
    };

    static bool ScopeMatches(const CardDef& def, const RollRequest& request)
    {
        switch (def.scope)
        {
            case EffectScope::NextRoll:            break;
            case EffectScope::NextExplorationRoll: if (request.isCombat)  return false; break;
            case EffectScope::NextCombatRoll:      if (!request.isCombat) return false; break;
            case EffectScope::NextCombat:          if (!request.isCombat) return false; break;
            case EffectScope::NextTwoCombats:      if (!request.isCombat) return false; break;
            case EffectScope::Permanent:           if (def.targetsRival && !request.isCombat) return false; break;
            case EffectScope::OnAcquire:           return false;
        }

        if (def.rollIndex >= 0 && def.rollIndex != request.rollIndex) return false;

        return true;
    }

    static void CollectFrom(Player& player, bool wantTargetsRival, bool fromRival,
                            const RollRequest& request, std::vector<ActiveEffect>& out)
    {
        for (CardInstance& card : player.cards)
        {
            if (card.spent || card.blockedNow) continue;

            const CardDef& def = GetCardDef(card.defId);
            if (def.targetsRival != wantTargetsRival) continue;
            if (!ScopeMatches(def, request)) continue;
            if (def.uses > 0 && def.scope == EffectScope::NextCombat && card.usesLeft <= 0) continue;

            ActiveEffect effect;
            effect.instance  = &card;
            effect.def       = &def;
            effect.fromRival = fromRival;
            out.push_back(effect);
        }
    }

    static std::vector<ActiveEffect> CollectEffects(Player& self, Player* rival,
                                                    const RollRequest& request)
    {
        std::vector<ActiveEffect> effects;
        CollectFrom(self, false, false, request, effects);
        if (rival != nullptr) CollectFrom(*rival, true, true, request, effects);
        return effects;
    }

    static int FibonacciBonusFor(int value, int mode)
    {
        static const int FIB[] = { 1, 2, 3, 5, 8, 13 };
        static const int PREV[] = { 0, 1, 2, 3, 5, 8 };
        static const int PREV2[] = { 0, 1, 3, 5, 8, 13 };

        for (int i = 0; i < 6; ++i)
        {
            if (FIB[i] == value) return (mode == 1) ? PREV[i] : PREV2[i];
        }

        if (mode == 1) return 0;

        int closest = 0;
        for (int i = 0; i < 6; ++i)
        {
            if (FIB[i] < value) closest = FIB[i];
        }
        return -closest;
    }

    static int ScaleDie(int value, int steps)
    {
        if (steps <= 1) return value;
        int size = DICE_FACES / steps;
        int scaled = (value - 1) / size + 1;
        if (scaled > steps) scaled = steps;
        return scaled;
    }

    static int HistoryDelta(const CardDef& def, const CombatMemory* memory, int rollIndex)
    {
        if (memory == nullptr) return 0;

        switch (def.paramA)
        {
            case 0:
            {
                int ref = def.paramB;
                if (ref < memory->rollsPlayed && memory->outcomes[ref] < 0) return def.paramC;
            } break;

            case 1:
            {
                if (rollIndex > 0 && rollIndex - 1 < memory->rollsPlayed &&
                    memory->outcomes[rollIndex - 1] > 0) return def.paramC;
            } break;

            case 2:
            {
                if (memory->rollsPlayed >= def.paramB && memory->losses > memory->wins) return def.paramC;
            } break;

            case 3:
            {
                if (memory->rollsPlayed >= def.paramB && memory->wins > memory->losses) return def.paramC;
            } break;

            default: break;
        }

        return 0;
    }

    static void AppendName(std::string& names, const CardDef& def, bool fromRival)
    {
        std::string entry = def.name;
        if (fromRival) entry += " (del rival)";
        if (names.find(entry) != std::string::npos) return;
        if (!names.empty()) names += ", ";
        names += entry;
    }

    // ---------------------------------------------------------------- dados

    static void PrepareDice(RollSession& session, const std::vector<ActiveEffect>& effects)
    {
        session.diceCount  = 1;
        session.scaleSteps = 0;

        bool blocked = false;
        for (const ActiveEffect& effect : effects)
        {
            if (effect.def->effect == EffectKind::DiceControl && effect.def->paramA == 2) blocked = true;
        }

        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            if (def.effect != EffectKind::DiceControl) continue;

            if (def.paramA == 0 && !blocked)
            {
                if (def.paramB > session.diceCount) session.diceCount = def.paramB;
            }
            else if (def.paramA == 1 && !blocked)
            {
                if (def.paramB > session.diceCount) session.diceCount = def.paramB;
                session.scaleSteps = def.paramC;
            }
            else if (def.paramA == 4 && !blocked)
            {
                if (session.diceCount < 2) session.diceCount = 2;
            }
        }

        if (session.diceCount > MAX_DICE) session.diceCount = MAX_DICE;
    }

    static int ApplyRerollRules(const std::vector<ActiveEffect>& effects, int value)
    {
        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            if (def.effect == EffectKind::DiceControl && def.paramA == 3 && value >= def.paramB)
            {
                value = RollRawD20();
            }
        }
        return value;
    }

    static int ApplyGambler(const std::vector<ActiveEffect>& effects, int first, int second)
    {
        for (const ActiveEffect& effect : effects)
        {
            if (effect.def->effect == EffectKind::DiceControl && effect.def->paramA == 4)
            {
                int coin = RandomInt(0, 1);
                int value = (coin == 0) ? (second + 1) / 2 : second * 2;
                if (value > DICE_FACES) value = DICE_FACES;
                return (value > first) ? value : first;
            }
        }
        return first;
    }

    // ------------------------------------------------------------- elecciones

    static void BuildDiceChoice(RollSession& session)
    {
        session.choice.active = true;
        session.choice.title  = "Elegí con qué resultado te quedás";
        session.choice.options.clear();

        for (int i = 0; i < session.diceCount; ++i)
        {
            ChoiceOption option;
            option.label = TextFormat("%i", session.diceValues[i]);
            option.value = session.diceValues[i];
            session.choice.options.push_back(option);
        }
    }

    static const CardDef* FindChoiceCard(const std::vector<ActiveEffect>& effects, int mode)
    {
        for (const ActiveEffect& effect : effects)
        {
            if (effect.def->effect == EffectKind::PlayerChoice && effect.def->paramA == mode)
            {
                return effect.def;
            }
        }
        return nullptr;
    }

    // ------------------------------------------------------- cuerpo de la tirada

    static void ComputeTotal(RollSession& session, std::vector<ActiveEffect>& effects)
    {
        DiceRoll& roll = session.roll;

        for (const ActiveEffect& effect : effects)
        {
            if (effect.def->effect == EffectKind::ValueConvert && roll.natural == effect.def->paramA)
            {
                roll.natural = effect.def->paramB;
            }
        }

        roll.criticalHit  = (roll.natural == DICE_FACES);
        roll.criticalFail = (roll.natural == 1);

        if (roll.criticalHit || roll.criticalFail)
        {
            roll.modifier = 0;
            roll.total    = roll.natural;
            return;
        }

        // ---- restricciones sobre mis bendiciones -------------------------
        int  maxBlessings   = 99;
        bool noBlessings    = false;
        bool oncePerCombat  = false;

        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            if (def.effect != EffectKind::BlessingRestriction) continue;

            if (def.paramA == 0 && def.paramB < maxBlessings) maxBlessings = def.paramB;
            if (def.paramA == 1) noBlessings = true;
            if (def.paramA == 2) oncePerCombat = true;
        }

        // ---- contribuciones que suman o restan ---------------------------
        std::vector<Contribution> adds;

        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            int delta = 0;

            switch (def.effect)
            {
                case EffectKind::FlatBonus:      delta = def.paramA; break;
                case EffectKind::ThresholdBonus:
                    if (roll.natural >= def.paramA && roll.natural <= def.paramB) delta = def.paramC;
                    break;
                case EffectKind::ParityBonus:
                    delta = (roll.natural % 2 == def.paramA) ? def.paramB : def.paramC;
                    break;
                case EffectKind::FibonacciBonus: delta = FibonacciBonusFor(roll.natural, def.paramA); break;
                case EffectKind::HistoryBonus:   delta = HistoryDelta(def, session.memory, session.request.rollIndex); break;
                case EffectKind::TwoPhaseFlat:   delta = (effect.instance->combatsSeen <= 1) ? def.paramA : def.paramB; break;
                case EffectKind::PlayerChoice:
                {
                    if (def.paramA == 0 && session.betChoice == 1)
                    {
                        delta = (roll.natural <= def.paramB) ? def.paramC : -2;
                    }
                    else if (def.paramA == 1 && session.parityGuess >= 0)
                    {
                        delta = (roll.natural % 2 == session.parityGuess) ? def.paramB : def.paramC;
                    }
                    else if (def.paramA == 2)
                    {
                        delta = session.adjustChoice;
                    }
                } break;
                default: break;
            }

            if (delta == 0) continue;

            Contribution contribution;
            contribution.def        = &def;
            contribution.delta      = delta;
            contribution.myBlessing = (!effect.fromRival && def.kind == CardKind::Blessing);
            adds.push_back(contribution);

            if (contribution.myBlessing && oncePerCombat && effect.instance->usedInCombat)
            {
                adds.back().enabled = false;
            }
        }

        if (noBlessings)
        {
            for (Contribution& c : adds) if (c.myBlessing) c.enabled = false;
        }

        int blessingsAllowed = maxBlessings;
        for (Contribution& c : adds)
        {
            if (!c.enabled || !c.myBlessing) continue;
            if (blessingsAllowed <= 0) c.enabled = false;
            else --blessingsAllowed;
        }

        // ---- cartas del rival que tuercen mis bendiciones ----------------
        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            if (def.effect != EffectKind::TwistRival) continue;

            if (def.paramA == 0 || def.paramA == 1)
            {
                for (Contribution& c : adds)
                {
                    if (c.enabled && c.myBlessing && c.delta > 0)
                    {
                        c.delta = (def.paramA == 0) ? -c.delta : 0;
                    }
                }
            }
            else if (def.paramA == 3)
            {
                Contribution* best = nullptr;
                for (Contribution& c : adds)
                {
                    if (c.enabled && c.myBlessing && c.delta > 0 &&
                        (best == nullptr || c.delta > best->delta)) best = &c;
                }
                if (best != nullptr)
                {
                    best->enabled = false;
                    effect.instance->usesLeft = 0;
                }
            }
        }

        // ---- cartas mías que modifican lo que recibo ---------------------
        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            if (def.effect != EffectKind::ModifyIncoming) continue;

            if (def.paramA == 1)
            {
                for (Contribution& c : adds)
                {
                    if (c.enabled && c.delta > 0) c.delta = c.delta / 2;
                }
            }
            else if (def.paramA == 0)
            {
                for (Contribution& c : adds)
                {
                    if (c.enabled && c.delta < 0)
                    {
                        c.delta += def.paramB;
                        if (c.delta > 0) c.delta = 0;
                        break;
                    }
                }
            }
        }

        // ---- castigos por usar bendiciones --------------------------------
        int blessingsThisRoll = 0;
        for (const Contribution& c : adds)
        {
            if (c.enabled && c.myBlessing && c.delta != 0) ++blessingsThisRoll;
        }

        const CombatMemory* memory = session.memory;
        int totalBlessings = blessingsThisRoll + (memory != nullptr ? memory->blessingsUsed : 0);

        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            if (def.effect != EffectKind::BlessingPenalty) continue;

            int delta = 0;
            if (def.paramA == 0)
            {
                bool firstEver = (memory == nullptr) || !memory->blessingUsedEver;
                if (blessingsThisRoll > 0 && firstEver) delta = def.paramC;
            }
            else if (def.paramA == 1)
            {
                if (memory != nullptr && memory->blessingUsedLastRoll) delta = def.paramC;
            }
            else if (def.paramA == 2)
            {
                if (totalBlessings >= def.paramB) delta = def.paramC;
            }

            if (delta != 0)
            {
                Contribution contribution;
                contribution.def   = &def;
                contribution.delta = delta;
                adds.push_back(contribution);
            }
        }

        int total = roll.natural;
        for (const Contribution& c : adds)
        {
            if (c.enabled) total += c.delta;
        }

        // ---- multiplicar y dividir ----------------------------------------
        bool invertMultiply = false;
        for (const ActiveEffect& effect : effects)
        {
            if (effect.def->effect == EffectKind::TwistRival && effect.def->paramA == 2) invertMultiply = true;
        }

        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;

            if (def.effect == EffectKind::MultiplyCapped)
            {
                if (invertMultiply)
                {
                    total = (total + def.paramA - 1) / def.paramA;
                }
                else
                {
                    total *= def.paramA;
                    if (total > def.paramB) total = def.paramB;
                }
            }
            else if (def.effect == EffectKind::DivideRoundUp)
            {
                total = (total + 1) / 2;
            }
            else if (def.effect == EffectKind::SafePlay)
            {
                if (roll.natural <= def.paramA)      total *= def.paramB;
                else if (roll.natural >= def.paramC) total = (total + 1) / 2;
            }
        }

        // ---- topes ---------------------------------------------------------
        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            if (def.effect == EffectKind::MaxResult && total > def.paramA) total = def.paramA;
        }

        if (total < 1) total = 1;

        roll.total    = total;
        roll.modifier = total - roll.natural;

        session.roll.blessingsUsed = blessingsThisRoll;
    }

    static void FinishSession(RollSession& session, std::vector<ActiveEffect>& effects)
    {
        for (const ActiveEffect& effect : effects)
        {
            const CardDef& def = *effect.def;
            AppendName(session.appliedNames, def, effect.fromRival);

            effect.instance->usedInCombat = true;

            if (def.uses > 0)
            {
                --effect.instance->usesLeft;
                if (effect.instance->usesLeft <= 0) effect.instance->spent = true;
                continue;
            }

            switch (def.scope)
            {
                case EffectScope::NextRoll:
                case EffectScope::NextExplorationRoll:
                case EffectScope::NextCombatRoll:
                {
                    if (effect.instance->rollsLeft > 1) --effect.instance->rollsLeft;
                    else effect.instance->spent = true;
                } break;

                default: break;
            }
        }

        if (session.memory != nullptr)
        {
            session.memory->blessingUsedLastRoll = (session.roll.blessingsUsed > 0);
            if (session.roll.blessingsUsed > 0)
            {
                session.memory->blessingUsedEver = true;
                session.memory->blessingsUsed += session.roll.blessingsUsed;
            }
        }

        session.done = true;
        session.choice.active = false;
    }

    // ------------------------------------------------------------- API pública

    static std::vector<ActiveEffect> g_effects;

    void BeginRoll(RollSession& session, Player& self, Player* rival,
                   const RollRequest& request, CombatMemory* memory)
    {
        session = RollSession{};
        session.self    = &self;
        session.rival   = rival;
        session.request = request;
        session.memory  = memory;
        session.step    = 0;

        AdvanceRoll(session, -1);
    }

    void AdvanceRoll(RollSession& session, int chosenOption)
    {
        if (session.done) return;

        g_effects = CollectEffects(*session.self, session.rival, session.request);

        if (session.step == 0)
        {
            const CardDef* bet = FindChoiceCard(g_effects, 0);
            if (bet != nullptr && session.betChoice < 0)
            {
                if (chosenOption < 0)
                {
                    session.choice.active = true;
                    session.choice.title  = TextFormat("%s: ¿apostás?", bet->name);
                    session.choice.options.clear();
                    session.choice.options.push_back({ TextFormat("Sí: si sacás %i o menos sumás %i, si no restás 2",
                                                                  bet->paramB, bet->paramC), 1 });
                    session.choice.options.push_back({ "No, dejar la tirada como está", 0 });
                    return;
                }
                session.betChoice = session.choice.options[chosenOption].value;
                session.choice.active = false;
                chosenOption = -1;
            }

            const CardDef* parity = FindChoiceCard(g_effects, 1);
            if (parity != nullptr && session.parityGuess < 0)
            {
                if (chosenOption < 0)
                {
                    session.choice.active = true;
                    session.choice.title  = TextFormat("%s: ¿impar o par?", parity->name);
                    session.choice.options.clear();
                    session.choice.options.push_back({ "Impar", 1 });
                    session.choice.options.push_back({ "Par", 0 });
                    return;
                }
                session.parityGuess = session.choice.options[chosenOption].value;
                session.choice.active = false;
                chosenOption = -1;
            }

            session.step = 1;
        }

        if (session.step == 1)
        {
            PrepareDice(session, g_effects);

            for (int i = 0; i < session.diceCount; ++i)
            {
                int value = RollRawD20();
                value = ApplyRerollRules(g_effects, value);
                if (session.scaleSteps > 0) value = ScaleDie(value, session.scaleSteps);
                session.diceValues[i] = value;
            }

            if (session.diceCount >= 2)
            {
                bool gambler = false;
                for (const ActiveEffect& effect : g_effects)
                {
                    if (effect.def->effect == EffectKind::DiceControl && effect.def->paramA == 4) gambler = true;
                }

                if (gambler)
                {
                    session.roll.natural = ApplyGambler(g_effects, session.diceValues[0], session.diceValues[1]);
                    session.step = 3;
                }
                else
                {
                    session.step = 2;
                }
            }
            else
            {
                session.roll.natural = session.diceValues[0];
                session.step = 3;
            }

            session.roll.dice       = session.diceCount;
            session.roll.naturalRaw = session.diceValues[0];
        }

        if (session.step == 2)
        {
            if (chosenOption < 0)
            {
                BuildDiceChoice(session);
                return;
            }

            session.roll.natural = session.choice.options[chosenOption].value;
            session.choice.active = false;
            chosenOption = -1;
            session.step = 3;
        }

        if (session.step == 3)
        {
            // Reserva: usar el resultado guardado en lugar del dado.
            if (session.memory != nullptr && session.memory->saveArmed && session.memory->savedResult > 0)
            {
                session.roll.natural = session.memory->savedResult;
                session.memory->saveArmed   = false;
                session.memory->savedResult = 0;
            }

            ComputeTotal(session, g_effects);
            session.step = 4;
        }

        if (session.step == 4)
        {
            const CardDef* adjust = FindChoiceCard(g_effects, 2);
            if (adjust != nullptr && session.adjustChoice == 0 && !session.roll.criticalHit &&
                !session.roll.criticalFail)
            {
                if (chosenOption < 0)
                {
                    session.choice.active = true;
                    session.choice.title  = TextFormat("%s: tu tirada dio %i", adjust->name, session.roll.total);
                    session.choice.options.clear();
                    session.choice.options.push_back({ TextFormat("Sumar %i", adjust->paramB), adjust->paramB });
                    session.choice.options.push_back({ TextFormat("Restar %i", adjust->paramB), -adjust->paramB });
                    session.choice.options.push_back({ "Dejarla como está", 0 });
                    return;
                }

                session.adjustChoice = session.choice.options[chosenOption].value;
                session.choice.active = false;

                if (session.adjustChoice != 0)
                {
                    ComputeTotal(session, g_effects);
                }
            }

            // Reserva: guardar este resultado para la próxima tirada.
            for (const ActiveEffect& effect : g_effects)
            {
                if (effect.def->effect == EffectKind::PlayerChoice && effect.def->paramA == 3 &&
                    session.memory != nullptr && !session.memory->saveArmed)
                {
                    session.memory->saveArmed   = true;
                    session.memory->savedResult = session.roll.natural;
                }
            }

            FinishSession(session, g_effects);
        }
    }

    static int AutoChoose(const RollSession& session)
    {
        int best = 0;
        for (int i = 1; i < static_cast<int>(session.choice.options.size()); ++i)
        {
            if (session.choice.options[i].value > session.choice.options[best].value) best = i;
        }
        return best;
    }

    DiceRoll RollSimple(Player& self, Player* rival, const RollRequest& request,
                        std::string& appliedNames, CombatMemory* memory)
    {
        RollSession session;
        BeginRoll(session, self, rival, request, memory);

        int guard = 0;
        while (!session.done && guard < 16)
        {
            AdvanceRoll(session, session.choice.active ? AutoChoose(session) : -1);
            ++guard;
        }

        appliedNames = session.appliedNames;
        return session.roll;
    }

    std::string DescribePendingEffects(const Player& self, const Player* rival,
                                       const RollRequest& request)
    {
        std::string names;

        for (const CardInstance& card : self.cards)
        {
            if (card.spent || card.blockedNow) continue;
            const CardDef& def = GetCardDef(card.defId);
            if (def.targetsRival) continue;
            if (!ScopeMatches(def, request)) continue;
            if (!names.empty()) names += ", ";
            names += def.name;
        }

        if (rival != nullptr)
        {
            for (const CardInstance& card : rival->cards)
            {
                if (card.spent || card.blockedNow) continue;
                const CardDef& def = GetCardDef(card.defId);
                if (!def.targetsRival) continue;
                if (!ScopeMatches(def, request)) continue;
                if (!names.empty()) names += ", ";
                names += def.name;
                names += " (del rival)";
            }
        }

        return names;
    }

    WinRules GatherWinRules(const Player& self, const Player& rival, int rollIndex)
    {
        WinRules rules;

        const Player* players[2] = { &self, &rival };
        for (int side = 0; side < 2; ++side)
        {
            for (const CardInstance& card : players[side]->cards)
            {
                if (card.spent || card.blockedNow) continue;

                const CardDef& def = GetCardDef(card.defId);
                if (def.effect != EffectKind::WinRule) continue;
                if (def.scope != EffectScope::NextCombat) continue;
                if (def.rollIndex >= 0 && def.rollIndex != rollIndex) continue;

                if (def.paramA == 0) rules.lowestWins = true;
                else if (def.paramA == 1) { rules.closestTo = true; rules.closestTarget = def.paramB; }
                else if (def.paramA == 2 && side == 1) rules.rivalNeedsAtLeast = def.paramB;
                else if (def.paramA == 3 && side == 0) rules.tieIsLoss = true;
            }
        }

        return rules;
    }

    void StartCombatEffects(Player& player, Player& rival, CombatMemory& memory)
    {
        memory = CombatMemory{};

        for (CardInstance& card : player.cards)
        {
            card.blockedNow   = false;
            card.usedInCombat = false;

            if (card.spent) continue;

            const CardDef& def = GetCardDef(card.defId);

            if (def.uses > 0 && card.usesLeft <= 0) card.usesLeft = def.uses;
            if (def.scope == EffectScope::NextTwoCombats) ++card.combatsSeen;
            if (def.effect == EffectKind::CombatRolls) memory.rollsLimit += def.paramA;
        }

        if (memory.rollsLimit < 1) memory.rollsLimit = 1;

        // Despojo: el rival bloquea tu mejor bendición durante este combate.
        for (CardInstance& card : player.cards)
        {
            if (card.spent) continue;

            const CardDef& def = GetCardDef(card.defId);
            if (def.effect != EffectKind::BlessingRestriction || def.paramA != 3) continue;

            CardInstance* target = nullptr;
            int bestValue = -1;

            for (CardInstance& other : player.cards)
            {
                if (other.spent || other.blockedNow) continue;
                const CardDef& otherDef = GetCardDef(other.defId);
                if (otherDef.kind != CardKind::Blessing) continue;

                int value = static_cast<int>(otherDef.level) * 10 + otherDef.paramA;
                if (value > bestValue) { bestValue = value; target = &other; }
            }

            if (target != nullptr) target->blockedNow = true;
        }

        (void)rival;
    }

    void EndCombatEffects(Player& player)
    {
        for (CardInstance& card : player.cards)
        {
            card.blockedNow = false;

            if (card.spent) continue;

            const CardDef& def = GetCardDef(card.defId);

            if (def.scope == EffectScope::NextCombat)
            {
                card.spent = true;
            }
            else if (def.scope == EffectScope::NextTwoCombats)
            {
                if (card.combatsSeen >= 2) card.spent = true;
            }
        }
    }

    bool BlocksCombatReward(const Player& player)
    {
        for (const CardInstance& card : player.cards)
        {
            if (card.spent) continue;
            if (GetCardDef(card.defId).effect == EffectKind::NoRewardOnWin) return true;
        }
        return false;
    }

    void ClaimCombatReward(Player& winner, Player& loser, std::string& message)
    {
        message.clear();

        if (BlocksCombatReward(winner))
        {
            message = "Corona rota: el ganador no se lleva recompensa.";
            return;
        }

        std::vector<int> candidates;
        for (int i = 0; i < static_cast<int>(loser.cards.size()); ++i)
        {
            const CardInstance& card = loser.cards[i];
            if (card.spent) continue;
            if (GetCardDef(card.defId).kind == CardKind::Blessing) candidates.push_back(i);
        }

        if (candidates.empty()) return;

        int pick = candidates[RandomInt(0, static_cast<int>(candidates.size()) - 1)];
        CardInstance stolen = loser.cards[pick];
        loser.cards.erase(loser.cards.begin() + pick);
        winner.cards.push_back(stolen);

        message = TextFormat("Recompensa: J%i le roba %s a J%i",
                             winner.id + 1, GetCardDef(stolen.defId).name, loser.id + 1);
    }

    void ApplyOnAcquire(Player& player, Player others[], int playerCount, int defId,
                        std::string& message)
    {
        message.clear();

        const CardDef& def = GetCardDef(defId);
        if (def.scope != EffectScope::OnAcquire) return;
        if (def.effect != EffectKind::HandAction) return;

        if (def.paramA == 0 || def.paramA == 1)
        {
            int bestPlayer = -1;
            int bestCard   = -1;
            int bestValue  = -1;

            for (int p = 0; p < playerCount; ++p)
            {
                if (others[p].id == player.id) continue;

                for (int i = 0; i < static_cast<int>(others[p].cards.size()); ++i)
                {
                    const CardInstance& card = others[p].cards[i];
                    if (card.spent) continue;

                    const CardDef& other = GetCardDef(card.defId);
                    if (other.kind != CardKind::Blessing) continue;

                    int value = (def.paramA == 1)
                                ? static_cast<int>(other.level) * 10 + other.paramA
                                : RandomInt(0, 99);

                    if (value > bestValue) { bestValue = value; bestPlayer = p; bestCard = i; }
                }
            }

            if (bestPlayer >= 0)
            {
                CardInstance stolen = others[bestPlayer].cards[bestCard];
                others[bestPlayer].cards.erase(others[bestPlayer].cards.begin() + bestCard);
                player.cards.push_back(stolen);

                message = TextFormat("%s: le roba %s a J%i", def.name,
                                     GetCardDef(stolen.defId).name, others[bestPlayer].id + 1);
            }
        }
        else if (def.paramA == 2)
        {
            for (int i = 0; i < static_cast<int>(player.cards.size()); ++i)
            {
                const CardInstance& card = player.cards[i];
                if (card.spent) continue;

                const CardDef& other = GetCardDef(card.defId);
                if (other.kind != CardKind::Curse) continue;
                if (static_cast<int>(other.level) != def.paramB) continue;

                message = TextFormat("%s: descarta %s", def.name, other.name);
                player.cards.erase(player.cards.begin() + i);
                return;
            }
        }
    }
}
