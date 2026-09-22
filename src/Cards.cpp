#include "Cards.h"
#include "Dice.h"

namespace ReyesCuadra
{
    static const CardDef CARD_CATALOG[] =
    {
        { "Impulso",
          "En tu próximo lanzamiento, sumás 2 al resultado final.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextRoll, 2, 0, 0, -1, false },

        { "Multiplicar",
          "En tu próximo lanzamiento, duplicás el resultado. El resultado final no puede superar 20.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::MultiplyCapped, EffectScope::NextRoll, 2, 20, 0, -1, false },

        { "Dos caminos",
          "En tu próximo lanzamiento, si el resultado es impar, sumás 3. Si es par, restás 3.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::ParityBonus, EffectScope::NextRoll, 1, 3, -3, -1, false },

        { "Dos caminos",
          "En tu próximo lanzamiento, si el resultado es par, sumás 3. Si es impar, restás 3.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, 3, -3, -1, false },

        { "No tan rápido",
          "En el próximo lanzamiento de tu rival, restale 3 al resultado final.",
          CardKind::Blessing, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextCombatRoll, -3, 0, 0, -1, true },

        { "Paso firme",
          "En tu próximo lanzamiento, si sacás entre 1 y 9, sumás 4. Si sacás 10 o más, el resultado no cambia.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::ThresholdBonus, EffectScope::NextRoll, 1, 9, 4, -1, false },

        { "Multiplicar +",
          "En tu próximo lanzamiento, duplicás el resultado. El resultado final no puede superar 25.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::MultiplyCapped, EffectScope::NextRoll, 2, 25, 0, -1, false },

        { "Dos caminos +",
          "En tu próximo lanzamiento, si el resultado es impar, sumás 5. Si es par, restás 5.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::ParityBonus, EffectScope::NextRoll, 1, 5, -5, -1, false },

        { "Dos caminos +",
          "En tu próximo lanzamiento, si el resultado es par, sumás 5. Si es impar, restás 5.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, 5, -5, -1, false },

        { "No tan rápido +",
          "En el próximo lanzamiento de tu rival, restale 5 al resultado final.",
          CardKind::Blessing, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextCombatRoll, -5, 0, 0, -1, true },

        { "Golpe perfecto",
          "En tu próximo combate, si sacás 18, 19 o 20 en una tirada, sumás 3 al resultado.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::ThresholdBonus, EffectScope::NextCombat, 18, 20, 3, -1, false },

        { "Doble o nada",
          "En tu próxima tirada de combate, tirás 2 D20 y elegís el resultado que prefieras.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::ExtraDice, EffectScope::NextCombatRoll, 2, 0, 0, -1, false },

        { "Dos caminos ++",
          "En tu próximo lanzamiento, si el resultado es impar, sumás 10. Si es par, restás 10.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::ParityBonus, EffectScope::NextRoll, 1, 10, -10, -1, false },

        { "Dos caminos ++",
          "En tu próximo lanzamiento, si el resultado es par, sumás 10. Si es impar, restás 10.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, 10, -10, -1, false },

        { "No tan rápido ++",
          "En el próximo lanzamiento de tu rival, restale 10 al resultado final.",
          CardKind::Blessing, CardLevel::Epic,
          EffectKind::FlatBonus, EffectScope::NextCombatRoll, -10, 0, 0, -1, true },

        { "Fatiga",
          "En tu próximo combate, restás 2 a tu primera tirada.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextCombat, -2, 0, 0, 0, false },

        { "Desgaste",
          "En tu próximo combate, restás 2 a tu segunda tirada.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::FlatBonus, EffectScope::NextCombat, -2, 0, 0, 1, false },

        { "Tropiezo",
          "En tu próximo lanzamiento, el resultado final no puede ser mayor que 15.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::MaxResult, EffectScope::NextRoll, 15, 0, 0, -1, false },

        { "Duda",
          "En tu próximo lanzamiento, si el resultado es par, restás 3.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::ParityBonus, EffectScope::NextRoll, 0, -3, 0, -1, false },

        { "Mal paso",
          "En tu próximo lanzamiento, si sacás 5 o menos, restás 3 adicionales.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::ThresholdBonus, EffectScope::NextRoll, 1, 5, -3, -1, false },

        { "Pulso inestable",
          "En tu próximo lanzamiento, si sacás 16 o más, restás 4.",
          CardKind::Curse, CardLevel::Weak,
          EffectKind::ThresholdBonus, EffectScope::NextRoll, 16, 20, -4, -1, false },

        { "Herida abierta",
          "En tu próximo combate, restás 4 a tu primera tirada.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextCombat, -4, 0, 0, 0, false },

        { "Lastre",
          "En tu próximo combate, restás 2 a todas tus tiradas.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::FlatBonus, EffectScope::NextCombat, -2, 0, 0, -1, false },

        { "Temor",
          "En tu próximo combate, cada resultado de 15 o más se convierte en 14.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::MaxResult, EffectScope::NextCombat, 14, 0, 0, -1, false },

        { "Dividir",
          "En tu próximo lanzamiento, dividí el resultado entre 2 y redondeá hacia arriba.",
          CardKind::Curse, CardLevel::Normal,
          EffectKind::DivideRoundUp, EffectScope::NextRoll, 0, 0, 0, -1, false },

        { "Impuesto",
          "En tu próximo combate, restás 3 al resultado final de cada una de tus 3 tiradas.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::FlatBonus, EffectScope::NextCombat, -3, 0, 0, -1, false },

        { "Tormenta",
          "En tu próximo combate, tu primera tirada no puede superar 10 después de aplicar todos los efectos.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::MaxResult, EffectScope::NextCombat, 10, 0, 0, 0, false },

        { "Punto muerto",
          "En tu próximo combate, tu tercera tirada no puede superar 8 después de aplicar todos los efectos.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::MaxResult, EffectScope::NextCombat, 8, 0, 0, 2, false },

        { "Maldición del veinte",
          "En tu próximo combate, cada vez que saques 20, ese resultado se convierte en 1.",
          CardKind::Curse, CardLevel::Epic,
          EffectKind::ValueConvert, EffectScope::NextCombat, 20, 1, 0, -1, false },
    };

    static constexpr int CARD_CATALOG_COUNT =
        static_cast<int>(sizeof(CARD_CATALOG) / sizeof(CARD_CATALOG[0]));

    int GetCardDefCount()
    {
        return CARD_CATALOG_COUNT;
    }

    const CardDef& GetCardDef(int defId)
    {
        if (defId < 0 || defId >= CARD_CATALOG_COUNT) return CARD_CATALOG[0];
        return CARD_CATALOG[defId];
    }

    int CountCards(const Player& player, CardKind kind)
    {
        int count = 0;
        for (const CardInstance& card : player.cards)
        {
            if (GetCardDef(card.defId).kind == kind) ++count;
        }
        return count;
    }

    int CountPendingCards(const Player& player)
    {
        int count = 0;
        for (const CardInstance& card : player.cards)
        {
            if (!card.spent) ++count;
        }
        return count;
    }

    static Deck& GetDeck(DeckSet& set, CardKind kind, CardLevel level)
    {
        return set.decks[static_cast<int>(kind)][static_cast<int>(level)];
    }

    void ShuffleDeck(Deck& deck)
    {
        for (int i = static_cast<int>(deck.available.size()) - 1; i > 0; --i)
        {
            int j = RandomInt(0, i);
            int temp = deck.available[i];
            deck.available[i] = deck.available[j];
            deck.available[j] = temp;
        }
    }

    void InitDeckSet(DeckSet& set)
    {
        for (int kindIndex = 0; kindIndex < 2; ++kindIndex)
        {
            for (int levelIndex = 0; levelIndex < 3; ++levelIndex)
            {
                Deck& deck = set.decks[kindIndex][levelIndex];
                deck.available.clear();
                deck.taken.clear();
            }
        }

        for (int defId = 0; defId < CARD_CATALOG_COUNT; ++defId)
        {
            const CardDef& def = CARD_CATALOG[defId];
            GetDeck(set, def.kind, def.level).available.push_back(defId);
        }

        for (int kindIndex = 0; kindIndex < 2; ++kindIndex)
        {
            for (int levelIndex = 0; levelIndex < 3; ++levelIndex)
            {
                ShuffleDeck(set.decks[kindIndex][levelIndex]);
            }
        }
    }

    int DrawCardOptions(DeckSet& set, CardKind kind, CardLevel level, int options[])
    {
        Deck& deck = GetDeck(set, kind, level);

        if (static_cast<int>(deck.available.size()) < CARD_OPTIONS)
        {
            for (int defId : deck.taken) deck.available.push_back(defId);
            deck.taken.clear();
            ShuffleDeck(deck);
        }

        int offered = 0;
        while (offered < CARD_OPTIONS && !deck.available.empty())
        {
            options[offered] = deck.available.back();
            deck.available.pop_back();
            ++offered;
        }

        return offered;
    }

    void ReturnCardOptions(DeckSet& set, CardKind kind, CardLevel level,
                           const int options[], int optionCount, int chosenIndex)
    {
        Deck& deck = GetDeck(set, kind, level);

        for (int i = 0; i < optionCount; ++i)
        {
            if (i == chosenIndex) deck.taken.push_back(options[i]);
            else                  deck.available.push_back(options[i]);
        }

        ShuffleDeck(deck);
    }

    CardLevel GetCardLevelForPath(PathKind path)
    {
        switch (path)
        {
            case PathKind::Easy:   return CardLevel::Weak;
            case PathKind::Normal: return CardLevel::Normal;
            case PathKind::Hard:   return CardLevel::Epic;
        }
        return CardLevel::Weak;
    }
}
