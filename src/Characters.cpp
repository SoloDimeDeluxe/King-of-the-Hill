#include "Characters.h"

namespace ReyesCuadra
{
    // Tres personajes con una pasiva cada uno. El documento los pide como
    // mecánica pero no dice cuáles son: estos son provisionales y cambiarlos
    // es cambiar esta tabla.
    static const CharacterDef CHARACTER_DEFS[static_cast<int>(CharacterId::Count)] =
    {
        {
            "LA ESCALADORA", "conoce la montaña",
            "Suma 2 a las tiradas de evento.",
            2, 0, 0, false, Color{ 120, 200, 235, 255 }
        },
        {
            "EL CAZADOR", "vive de lo que caza",
            "Suma 2 a los golpes contra enemigos.",
            0, 2, 0, false, Color{ 235, 140,  85, 255 }
        },
        {
            "EL AFORTUNADO", "siempre encuentra algo",
            "Arranca con dos ítems y una bendición común.",
            0, 0, 2, true, Color{ 225, 195,  95, 255 }
        }
    };

    const CharacterDef& GetCharacterDef(CharacterId id)
    {
        int index = static_cast<int>(id);

        if (index < 0 || index >= static_cast<int>(CharacterId::Count))
        {
            static const CharacterDef empty{};
            return empty;
        }

        return CHARACTER_DEFS[index];
    }

    int GetCharacterBonus(const Player& player, bool isFight)
    {
        const CharacterDef& def = GetCharacterDef(player.character);
        return isFight ? def.fightBonus : def.eventBonus;
    }
}
