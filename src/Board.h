#pragma once

#include "Types.h"

namespace ReyesCuadra
{
    int GetEventDifficulty(PathKind path, int stage);

    Vector2 GetTilePosition(PathKind path, int stage);

    void DrawBoard();

    void DrawPlayerTokens(const Player players[], int activePlayer);
}
