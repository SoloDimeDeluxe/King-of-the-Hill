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

    enum class PathKind
    {
        Easy   = 0,
        Normal = 1,
        Hard   = 2
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
        int  defId = -1;
        bool spent = false;
    };

    struct Player
    {
        int      id             = 0;
        Color    color          = Color{ 255, 255, 255, 255 };
        PathKind path           = PathKind::Normal;
        int      stage          = 0;
        int      initiativeRoll = 0;

        std::vector<CardInstance> cards;
    };

    const char* GetPathName(PathKind path);
    Color       GetPathColor(PathKind path);
    const char* GetCardLevelName(CardLevel level);
}
