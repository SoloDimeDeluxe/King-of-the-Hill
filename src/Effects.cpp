#include "Effects.h"

#include <vector>

namespace ReyesCuadra
{
    struct ActiveEffect
    {
        CardInstance* instance = nullptr;
        const CardDef* def     = nullptr;
        bool fromRival         = false;
    };

    static bool ScopeMatches(const CardDef& def, const RollRequest& request)
    {
        switch (def.scope)
        {
            case EffectScope::NextRoll:            break;
            case EffectScope::NextExplorationRoll: if (request.isCombat)  return false; break;
            case EffectScope::NextCombatRoll:      if (!request.isCombat) return false; break;
            case EffectScope::NextCombat:          if (!request.isCombat) return false; break;
        }

        if (def.rollIndex >= 0 && def.rollIndex != request.rollIndex) return false;

        return true;
    }

    static void CollectFrom(Player& player, bool wantTargetsRival, bool fromRival,
                            const RollRequest& request, std::vector<ActiveEffect>& out)
    {
        for (CardInstance& card : player.cards)
        {
            if (card.spent) continue;

            const CardDef& def = GetCardDef(card.defId);
            if (def.targetsRival != wantTargetsRival) continue;
            if (!ScopeMatches(def, request)) continue;

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

    static void AppendName(std::string& names, const ActiveEffect& effect)
    {
        if (!names.empty()) names += ", ";
        names += effect.def->name;
        if (effect.fromRival) names += " (del rival)";
    }

    DiceRoll RollWithEffects(Player& self, Player* rival, const RollRequest& request,
                             std::string& appliedNames)
    {
        std::vector<ActiveEffect> effects = CollectEffects(self, rival, request);

        appliedNames.clear();

        int diceCount = 1;
        for (const ActiveEffect& effect : effects)
        {
            if (effect.def->effect == EffectKind::ExtraDice && effect.def->paramA > diceCount)
            {
                diceCount = effect.def->paramA;
            }
        }

        DiceRoll roll;
        roll.dice = diceCount;

        int best = 0;
        for (int i = 0; i < diceCount; ++i)
        {
            int value = RollRawD20();
            if (value > best) best = value;
        }

        roll.naturalRaw = best;
        roll.natural    = best;

        for (const ActiveEffect& effect : effects)
        {
            if (effect.def->effect == EffectKind::ValueConvert &&
                roll.natural == effect.def->paramA)
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
        }
        else
        {
            int total = roll.natural;

            for (const ActiveEffect& effect : effects)
            {
                const CardDef& def = *effect.def;

                switch (def.effect)
                {
                    case EffectKind::FlatBonus:
                    {
                        total += def.paramA;
                    } break;

                    case EffectKind::ThresholdBonus:
                    {
                        if (roll.natural >= def.paramA && roll.natural <= def.paramB)
                        {
                            total += def.paramC;
                        }
                    } break;

                    case EffectKind::ParityBonus:
                    {
                        total += (roll.natural % 2 == def.paramA) ? def.paramB : def.paramC;
                    } break;

                    default: break;
                }
            }

            for (const ActiveEffect& effect : effects)
            {
                const CardDef& def = *effect.def;

                if (def.effect == EffectKind::MultiplyCapped)
                {
                    total *= def.paramA;
                    if (total > def.paramB) total = def.paramB;
                }
                else if (def.effect == EffectKind::DivideRoundUp)
                {
                    total = (total + 1) / 2;
                }
            }

            for (const ActiveEffect& effect : effects)
            {
                const CardDef& def = *effect.def;

                if (def.effect == EffectKind::MaxResult && total > def.paramA)
                {
                    total = def.paramA;
                }
            }

            if (total < 1) total = 1;

            roll.total    = total;
            roll.modifier = total - roll.natural;
        }

        for (const ActiveEffect& effect : effects)
        {
            AppendName(appliedNames, effect);

            if (effect.def->scope != EffectScope::NextCombat)
            {
                effect.instance->spent = true;
            }
        }

        return roll;
    }

    std::string DescribePendingEffects(const Player& self, const Player* rival,
                                       const RollRequest& request)
    {
        std::string names;

        for (const CardInstance& card : self.cards)
        {
            if (card.spent) continue;

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
                if (card.spent) continue;

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

    void EndCombatEffects(Player& player)
    {
        for (CardInstance& card : player.cards)
        {
            if (card.spent) continue;

            if (GetCardDef(card.defId).scope == EffectScope::NextCombat)
            {
                card.spent = true;
            }
        }
    }
}
