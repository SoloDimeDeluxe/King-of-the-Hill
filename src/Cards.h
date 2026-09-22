#pragma once

#include "Types.h"

namespace ReyesCuadra
{
    enum class EffectKind
    {
        FlatBonus,
        MaxResult,
        ThresholdBonus,
        ParityBonus,
        MultiplyCapped,
        DivideRoundUp,
        ExtraDice,
        ValueConvert
    };

    enum class EffectScope
    {
        NextRoll,
        NextExplorationRoll,
        NextCombatRoll,
        NextCombat
    };

    struct CardDef
    {
        const char* name         = "";
        const char* text         = "";
        CardKind    kind         = CardKind::Blessing;
        CardLevel   level        = CardLevel::Weak;
        EffectKind  effect       = EffectKind::FlatBonus;
        EffectScope scope        = EffectScope::NextRoll;
        int         paramA       = 0;
        int         paramB       = 0;
        int         paramC       = 0;
        int         rollIndex    = -1;
        bool        targetsRival = false;
    };

    constexpr int CARD_DESIGN_TOTAL = 96;

    int            GetCardDefCount();
    const CardDef& GetCardDef(int defId);

    int CountCards(const Player& player, CardKind kind);
    int CountPendingCards(const Player& player);

    struct Deck
    {
        std::vector<int> available;
        std::vector<int> taken;
    };

    struct DeckSet
    {
        Deck decks[2][3];
    };

    void InitDeckSet(DeckSet& set);
    void ShuffleDeck(Deck& deck);

    int  DrawCardOptions(DeckSet& set, CardKind kind, CardLevel level, int options[]);
    void ReturnCardOptions(DeckSet& set, CardKind kind, CardLevel level,
                           const int options[], int optionCount, int chosenIndex);

    CardLevel GetCardLevelForPath(PathKind path);
}
