#include "Board.h"
#include "Ui.h"

namespace ReyesCuadra
{
    constexpr float TILE_FIRST_X = 140.0f;
    constexpr float TILE_STEP_X  = 130.0f;
    constexpr float LANE_FIRST_Y = 150.0f;
    constexpr float LANE_STEP_Y  = 92.0f;
    constexpr float TILE_SIZE    = 54.0f;

    constexpr float COMBAT_X = 940.0f;
    constexpr float COMBAT_Y = 100.0f;
    constexpr float COMBAT_W = 270.0f;
    constexpr float COMBAT_H = 290.0f;

    int GetEventDifficulty(PathKind path, int stage)
    {
        int base = 0;
        switch (path)
        {
            case PathKind::Easy:   base = 8;  break;
            case PathKind::Normal: base = 12; break;
            case PathKind::Hard:   base = 16; break;
        }

        return base + (stage / 2);
    }

    Vector2 GetTilePosition(PathKind path, int stage)
    {
        Vector2 position{};
        position.x = TILE_FIRST_X + TILE_STEP_X * static_cast<float>(stage);
        position.y = LANE_FIRST_Y + LANE_STEP_Y * static_cast<float>(static_cast<int>(path));
        return position;
    }

    void DrawBoard()
    {
        for (int pathIndex = 0; pathIndex < PATH_COUNT; ++pathIndex)
        {
            PathKind path  = static_cast<PathKind>(pathIndex);
            Color    color = GetPathColor(path);
            Vector2  first = GetTilePosition(path, 0);
            Vector2  last  = GetTilePosition(path, STAGE_COUNT - 1);

            DrawLineEx(Vector2{ first.x - 60.0f, first.y },
                       Vector2{ COMBAT_X, last.y },
                       6.0f, Fade(color, 0.35f));

            UiText(GetPathName(path), 40, static_cast<int>(first.y) - 40, 20, color, FontStyle::Bold);

            for (int stage = 0; stage < STAGE_COUNT; ++stage)
            {
                Vector2 center = GetTilePosition(path, stage);
                Rectangle tile{ center.x - TILE_SIZE * 0.5f, center.y - TILE_SIZE * 0.5f,
                                TILE_SIZE, TILE_SIZE };

                DrawRectangleRounded(tile, 0.25f, 6, Fade(color, 0.20f));
                DrawRectangleLinesEx(tile, 2.0f, color);

                const char* difficultyText = TextFormat("%i", GetEventDifficulty(path, stage));
                int textWidth = UiMeasure(difficultyText, 22, FontStyle::Bold);
                UiText(difficultyText,
                       static_cast<int>(center.x) - textWidth / 2,
                       static_cast<int>(center.y) - 11,
                       22, RAYWHITE, FontStyle::Bold);
            }
        }

        Rectangle combatZone{ COMBAT_X, COMBAT_Y, COMBAT_W, COMBAT_H };
        DrawRectangleRounded(combatZone, 0.10f, 8, Fade(GOLD, 0.15f));
        DrawRectangleLinesEx(combatZone, 2.0f, GOLD);

        UiText("ZONA DE",  static_cast<int>(COMBAT_X) + 70, static_cast<int>(COMBAT_Y) + 100, 24, GOLD, FontStyle::Bold);
        UiText("COMBATE",  static_cast<int>(COMBAT_X) + 62, static_cast<int>(COMBAT_Y) + 130, 28, GOLD, FontStyle::Bold);
        UiText("Rey de la colina", static_cast<int>(COMBAT_X) + 52, static_cast<int>(COMBAT_Y) + 180, 16, Fade(GOLD, 0.8f));
    }

    void DrawPlayerTokens(const Player players[], int activePlayer)
    {
        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = players[i];

            Vector2 center;
            if (player.stage >= STAGE_COUNT)
            {
                center.x = COMBAT_X + 60.0f + (i % 2) * 90.0f;
                center.y = COMBAT_Y + 40.0f + (i / 2) * 60.0f;
            }
            else
            {
                center = GetTilePosition(player.path, player.stage);

                center.x += static_cast<float>(i % 2) * 20.0f - 10.0f;
                center.y += static_cast<float>(i / 2) * 20.0f - 10.0f;
            }

            DrawCircleV(center, 12.0f, player.color);
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 12.0f, BLACK);

            if (i == activePlayer)
            {
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 18.0f, RAYWHITE);
            }

            const char* label = TextFormat("%i", i + 1);
            int labelWidth = UiMeasure(label, 14, FontStyle::Bold);
            UiText(label, static_cast<int>(center.x) - labelWidth / 2,
                   static_cast<int>(center.y) - 7, 14, BLACK, FontStyle::Bold);
        }
    }
}
