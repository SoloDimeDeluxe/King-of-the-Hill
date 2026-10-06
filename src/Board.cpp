#include "Board.h"
#include "Fx.h"
#include "Theme.h"
#include "Ui.h"

#include <cmath>

namespace ReyesCuadra
{
    constexpr float TILE_FIRST_X = 140.0f;
    constexpr float TILE_STEP_X  = 130.0f;
    constexpr float LANE_FIRST_Y = 150.0f;
    constexpr float LANE_STEP_Y  = 92.0f;
    constexpr float TILE_SIZE    = 56.0f;

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
        float time = static_cast<float>(GetTime());

        for (int pathIndex = 0; pathIndex < PATH_COUNT; ++pathIndex)
        {
            PathKind path  = static_cast<PathKind>(pathIndex);
            Color    color = GetPathColor(path);
            Vector2  first = GetTilePosition(path, 0);
            Vector2  last  = GetTilePosition(path, STAGE_COUNT - 1);

            DrawLineEx(Vector2{ first.x - 62.0f, first.y + 2.0f },
                       Vector2{ COMBAT_X, last.y + 2.0f }, 10.0f, Fade(BLACK, 0.30f));
            DrawLineEx(Vector2{ first.x - 62.0f, first.y },
                       Vector2{ COMBAT_X, last.y }, 8.0f, Fade(color, 0.30f));

            UiText(GetPathName(path), 40, static_cast<int>(first.y) - 42, 19, color, FontStyle::Bold);

            for (int stage = 0; stage < STAGE_COUNT; ++stage)
            {
                Vector2 center = GetTilePosition(path, stage);
                Rectangle tile{ center.x - TILE_SIZE * 0.5f, center.y - TILE_SIZE * 0.5f,
                                TILE_SIZE, TILE_SIZE };

                Rectangle shadow = tile;
                shadow.x += 3.0f; shadow.y += 4.0f;
                DrawRectangleRounded(shadow, 0.28f, 6, Fade(BLACK, 0.35f));

                DrawRectangleRounded(tile, 0.28f, 6, COL_INK_SOFT);
                DrawRectangleRounded(Rectangle{ tile.x, tile.y, tile.width, tile.height * 0.45f },
                                     0.4f, 6, Fade(color, 0.22f));
                DrawRectangleRoundedLinesEx(tile, 0.28f, 6, 2.0f, Fade(color, 0.85f));

                const char* difficultyText = TextFormat("%i", GetEventDifficulty(path, stage));
                int textWidth = UiMeasure(difficultyText, 22, FontStyle::Bold);
                UiText(difficultyText,
                       static_cast<int>(center.x) - textWidth / 2,
                       static_cast<int>(center.y) - 12, 22, COL_CREAM, FontStyle::Bold);
            }
        }

        Rectangle combatZone{ COMBAT_X, COMBAT_Y, COMBAT_W, COMBAT_H };
        DrawSoftPanel(combatZone, Fade(COL_MAGENTA, 0.18f), Fade(COL_GOLD, 0.75f), 0.08f);

        float glow = 0.55f + 0.25f * Pulse(time, 1.6f);
        DrawCrown(static_cast<int>(COMBAT_X + COMBAT_W * 0.5f), static_cast<int>(COMBAT_Y) + 70,
                  52.0f, Fade(COL_GOLD, glow));

        UiTextCentered("ZONA DE COMBATE", static_cast<int>(COMBAT_X + COMBAT_W * 0.5f),
                       static_cast<int>(COMBAT_Y) + 128, 22, COL_GOLD, FontStyle::Bold);
        UiTextCentered("Rey de la colina", static_cast<int>(COMBAT_X + COMBAT_W * 0.5f),
                       static_cast<int>(COMBAT_Y) + 158, 15, Fade(COL_CREAM, 0.75f));
    }

    void DrawPlayerTokens(const Player players[], int activePlayer)
    {
        float time = static_cast<float>(GetTime());

        for (int i = 0; i < PLAYER_COUNT; ++i)
        {
            const Player& player = players[i];

            Vector2 center;
            if (player.stage >= STAGE_COUNT)
            {
                center.x = COMBAT_X + 58.0f + static_cast<float>(i % 2) * 96.0f;
                center.y = COMBAT_Y + 212.0f + static_cast<float>(i / 2) * 44.0f;
            }
            else
            {
                center = GetTilePosition(player.path, player.stage);
                center.x += static_cast<float>(i % 2) * 22.0f - 11.0f;
                center.y += static_cast<float>(i / 2) * 22.0f - 11.0f;
            }

            bool isActive = (i == activePlayer);
            if (isActive) center.y -= 3.0f + 2.0f * Pulse(time, 4.0f);

            DrawEllipse(static_cast<int>(center.x), static_cast<int>(center.y) + 13,
                        11.0f, 4.0f, Fade(BLACK, 0.35f));

            if (isActive)
            {
                float ring = 17.0f + 3.0f * Pulse(time, 4.0f);
                DrawCircleV(center, ring, Fade(COL_CREAM, 0.18f));
                DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), ring, COL_CREAM);
            }

            DrawCircleV(center, 12.0f, player.color);
            DrawCircleV(Vector2{ center.x, center.y - 3.0f }, 8.0f, Fade(WHITE, 0.18f));
            DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), 12.0f, COL_INK_DEEP);

            const char* label = TextFormat("%i", i + 1);
            int labelWidth = UiMeasure(label, 14, FontStyle::Bold);
            UiText(label, static_cast<int>(center.x) - labelWidth / 2,
                   static_cast<int>(center.y) - 8, 14, COL_INK_DEEP, FontStyle::Bold);
        }
    }
}
