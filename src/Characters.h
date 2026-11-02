#pragma once

#include "Types.h"

namespace ReyesCuadra
{
    struct CharacterDef
    {
        const char* name       = "";
        const char* title      = "";
        const char* passive    = "";
        int         eventBonus = 0;
        int         fightBonus = 0;
        int         startItems = 0;
        bool        startCard  = false;
        Color       color      = Color{ 255, 255, 255, 255 };
    };

    const CharacterDef& GetCharacterDef(CharacterId id);

    int GetCharacterBonus(const Player& player, bool isFight);
}
