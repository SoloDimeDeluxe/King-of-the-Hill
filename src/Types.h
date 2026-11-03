#pragma once

#include <vector>
#include "raylib.h"

namespace ReyesCuadra
{
    constexpr int SCREEN_WIDTH  = 1280;
    constexpr int SCREEN_HEIGHT = 720;

    constexpr int PLAYER_COUNT = 4;
    constexpr int PATH_COUNT   = 3;
    constexpr int STAGE_COUNT  = 6;
    constexpr int CARD_OPTIONS = 3;
    constexpr int DICE_FACES   = 20;
    constexpr int DUEL_COUNT   = 4;

    constexpr int COMBAT_ROLLS     = 3;
    constexpr int MAX_COMBAT_ROLLS = 9;
    constexpr int MAX_DICE         = 5;
    constexpr int MAX_CHOICES      = 5;

    constexpr int MAX_ITEMS = 4;

    enum class PathKind
    {
        Easy   = 0,
        Normal = 1,
        Hard   = 2
    };

    // Qué hay en una casilla. El tablero deja de ser un camino lineal porque
    // cada carril mezcla tipos distintos en las mismas etapas.
    enum class TileKind
    {
        Event    = 0,
        Enemy    = 1,
        Treasure = 2,
        Rest     = 3,
        Optional = 4,
        MiniBoss = 5,
        Boss     = 6
    };

    enum class ItemId
    {
        None      = -1,
        Charm     = 0,
        Whetstone = 1,
        Potion    = 2,
        Map       = 3,
        Count     = 4
    };

    enum class CharacterId
    {
        Climber = 0,
        Hunter  = 1,
        Lucky   = 2,
        Count   = 3
    };

    enum class CardKind
    {
        Blessing = 0,
        Curse    = 1
    };

    enum class CardLevel
    {
        Weak   = 0,
        Normal = 1,
        Epic   = 2
    };

    struct CardInstance
    {
        int  defId        = -1;
        bool spent        = false;
        int  rollsLeft    = 0;
        int  usesLeft     = 0;
        int  combatsSeen  = 0;
        bool blockedNow   = false;
        bool usedInCombat = false;
    };

    struct Player
    {
        int      id             = 0;
        Color    color          = Color{ 255, 255, 255, 255 };
        PathKind path           = PathKind::Normal;
        int      stage          = 0;
        int      initiativeRoll = 0;
        bool     isCpu          = false;

        // Posición dibujada: va persiguiendo a stage y path para que la ficha
        // se deslice en lugar de saltar de casilla.
        float    visualStage    = 0.0f;
        float    visualLane     = 1.0f;

        CharacterId character = CharacterId::Climber;

        std::vector<CardInstance> cards;
        std::vector<ItemId>       items;

        bool hasMap      = false;
        int  extraWounds = 0;
        int  rollBonus   = 0;

        // Cómo le fue contra el jefe de la cima: de ahí sale la corona.
        bool bossBeaten    = false;
        int  bossParts     = 0;
        int  bossTurnsLeft = 0;
    };

    const char* GetPathName(PathKind path);
    Color       GetPathColor(PathKind path);
    const char* GetCardLevelName(CardLevel level);

    const char* GetTileName(TileKind kind);
    const char* GetTileShortName(TileKind kind);
    bool        TileIsFight(TileKind kind);
}
