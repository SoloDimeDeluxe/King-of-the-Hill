#pragma once

#include "Types.h"

namespace ReyesCuadra
{
    struct ItemDef
    {
        const char* name  = "";
        const char* text  = "";
        bool        usable = true;
    };

    const ItemDef& GetItemDef(ItemId id);

    bool HasRoomForItem(const Player& player);
    bool GiveItem(Player& player, ItemId id);
    void RemoveItem(Player& player, int slot);
    bool HasItem(const Player& player, ItemId id);

    ItemId RollTreasureItem(const Player& player);
}
