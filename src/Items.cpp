#include "Items.h"
#include "Dice.h"

namespace ReyesCuadra
{
    // Los ítems son las "recompensas afuera del personaje" que pide el
    // documento: no modifican cartas, se gastan a mano cuando el jugador quiere.
    static const ItemDef ITEM_DEFS[static_cast<int>(ItemId::Count)] =
    {
        { "Amuleto de la suerte", "Suma 3 a la tirada que estás por hacer.",            true  },
        { "Piedra de afilar",     "Tu próximo golpe a un enemigo le saca 2 heridas.",   true  },
        { "Poción de hierbas",    "Saca de tu mano una maldición que todavía no usaste.", true },
        { "Mapa de la montaña",   "Revela qué hay en todas las casillas del tablero.",  false }
    };

    const ItemDef& GetItemDef(ItemId id)
    {
        int index = static_cast<int>(id);

        if (index < 0 || index >= static_cast<int>(ItemId::Count))
        {
            static const ItemDef empty{};
            return empty;
        }

        return ITEM_DEFS[index];
    }

    bool HasRoomForItem(const Player& player)
    {
        return static_cast<int>(player.items.size()) < MAX_ITEMS;
    }

    bool HasItem(const Player& player, ItemId id)
    {
        for (ItemId owned : player.items)
        {
            if (owned == id) return true;
        }

        return false;
    }

    bool GiveItem(Player& player, ItemId id)
    {
        if (id == ItemId::None) return false;

        // El mapa es pasivo: no ocupa lugar en la mochila, se activa y listo.
        if (id == ItemId::Map)
        {
            if (player.hasMap) return false;
            player.hasMap = true;
            return true;
        }

        if (!HasRoomForItem(player)) return false;

        player.items.push_back(id);
        return true;
    }

    void RemoveItem(Player& player, int slot)
    {
        if (slot < 0 || slot >= static_cast<int>(player.items.size())) return;

        player.items.erase(player.items.begin() + slot);
    }

    ItemId RollTreasureItem(const Player& player)
    {
        // El mapa sale primero, una sola vez: es lo que el documento pide como
        // recompensa de la primera batalla opcional.
        if (!player.hasMap) return ItemId::Map;

        switch (RandomInt(0, 2))
        {
            case 0:  return ItemId::Charm;
            case 1:  return ItemId::Whetstone;
            default: return ItemId::Potion;
        }
    }
}
