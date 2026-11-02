#pragma once

#include "Types.h"

namespace ReyesCuadra
{
    int GetEventDifficulty(PathKind path, int stage);

    TileKind GetTileKind(PathKind path, int stage);
    Color    GetTileColor(TileKind kind);

    Vector2 GetTilePosition(PathKind path, int stage);
    Vector2 GetTilePositionSmooth(float lane, float stage);

    void DrawBoard(int currentStage, bool revealAll);

    void DrawPlayerTokens(const Player players[], int activePlayer);
}
