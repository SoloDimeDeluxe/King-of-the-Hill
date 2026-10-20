#include "Board.h"
#include "Fx.h"
#include "Theme.h"
#include "Ui.h"

#include <cmath>

namespace ReyesCuadra
{
    // Tablero vertical: tres carriles en columnas y las etapas subiendo desde
    // la base de la montaña hasta la zona de combate.
    constexpr float LANE_FIRST_X = 152.0f;
    constexpr float LANE_STEP_X  = 164.0f;
    constexpr float TILE_FIRST_Y = 626.0f;
    constexpr float TILE_STEP_Y  = -86.0f;
    constexpr float TILE_SIZE    = 58.0f;

    constexpr float COMBAT_X = 56.0f;
    constexpr float COMBAT_Y = 84.0f;
    constexpr float COMBAT_W = 520.0f;
    constexpr float COMBAT_H = 72.0f;

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

    Vector2 GetTilePositionSmooth(float lane, float stage)
    {
        Vector2 position{};
        position.x = LANE_FIRST_X + LANE_STEP_X * lane;
        position.y = TILE_FIRST_Y + TILE_STEP_Y * stage;
        return position;
    }

    Vector2 GetTilePosition(PathKind path, int stage)
    {
        return GetTilePositionSmooth(static_cast<float>(static_cast<int>(path)),
                                     static_cast<float>(stage));
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

            DrawLineEx(Vector2{ first.x + 2.0f, first.y + 46.0f },
                       Vector2{ last.x + 2.0f, COMBAT_Y + COMBAT_H }, 11.0f, Fade(BLACK, 0.30f));
            DrawLineEx(Vector2{ first.x, first.y + 44.0f },
                       Vector2{ last.x, COMBAT_Y + COMBAT_H }, 9.0f, Fade(color, 0.28f));

            UiTextCentered(GetPathName(path), static_cast<int>(first.x),
                           static_cast<int>(first.y) + 52, 18, color, FontStyle::Bold);

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
                UiTextCentered(difficultyText, static_cast<int>(center.x),
                               static_cast<int>(center.y) - 12, 22, COL_CREAM, FontStyle::Bold);
            }
        }

        // Número de etapa al costado del tablero.
        for (int stage = 0; stage < STAGE_COUNT; ++stage)
        {
            Vector2 center = GetTilePosition(PathKind::Easy, stage);
            UiText(TextFormat("%i", stage + 1), 62, static_cast<int>(center.y) - 9, 17,
                   Fade(COL_CREAM, 0.35f), FontStyle::Bold);
        }

        Rectangle combatZone{ COMBAT_X, COMBAT_Y, COMBAT_W, COMBAT_H };
        DrawSoftPanel(combatZone, Fade(COL_MAGENTA, 0.20f), Fade(COL_GOLD, 0.75f), 0.3f);

        float glow = 0.55f + 0.25f * Pulse(time, 1.6f);
        DrawCrown(static_cast<int>(COMBAT_X) + 56, static_cast<int>(COMBAT_Y) + 30,
                  40.0f, Fade(COL_GOLD, glow));

        UiText("ZONA DE COMBATE", static_cast<int>(COMBAT_X) + 96, static_cast<int>(COMBAT_Y) + 14,
               21, COL_GOLD, FontStyle::Bold);
        UiText("el torneo por la corona", static_cast<int>(COMBAT_X) + 96,
               static_cast<int>(COMBAT_Y) + 42, 15, Fade(COL_CREAM, 0.7f));
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
                center.x = COMBAT_X + 300.0f + static_cast<float>(i % 2) * 48.0f;
                center.y = COMBAT_Y + 24.0f + static_cast<float>(i / 2) * 30.0f;
            }
            else
            {
                center = GetTilePositionSmooth(player.visualLane, player.visualStage);
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

            UiTextCentered(TextFormat("%i", i + 1), static_cast<int>(center.x),
                           static_cast<int>(center.y) - 8, 14, COL_INK_DEEP, FontStyle::Bold);
        }
    }
}
