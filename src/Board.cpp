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

    // El mapa de la montaña. Las etapas 4 y 6 son los dos hitos del documento
    // (mini jefe a la mitad, jefe en la cima) y están en los tres carriles; las
    // otras cuatro cambian según el camino, así que elegir carril ya no es sólo
    // elegir dificultad.
    static const TileKind TILE_MAP[STAGE_COUNT][PATH_COUNT] =
    {
        { TileKind::Event,    TileKind::Enemy,    TileKind::Enemy    },
        { TileKind::Rest,     TileKind::Event,    TileKind::Treasure },
        { TileKind::Enemy,    TileKind::Treasure, TileKind::Optional },
        { TileKind::MiniBoss, TileKind::MiniBoss, TileKind::MiniBoss },
        { TileKind::Event,    TileKind::Optional, TileKind::Treasure },
        { TileKind::Boss,     TileKind::Boss,     TileKind::Boss     }
    };

    TileKind GetTileKind(PathKind path, int stage)
    {
        if (stage < 0 || stage >= STAGE_COUNT) return TileKind::Event;

        int lane = static_cast<int>(path);
        if (lane < 0 || lane >= PATH_COUNT) lane = 0;

        return TILE_MAP[stage][lane];
    }

    Color GetTileColor(TileKind kind)
    {
        switch (kind)
        {
            case TileKind::Event:    return Color{ 150, 165, 200, 255 };
            case TileKind::Enemy:    return Color{ 225,  90,  90, 255 };
            case TileKind::Treasure: return COL_GOLD;
            case TileKind::Rest:     return Color{ 120, 205, 150, 255 };
            case TileKind::Optional: return Color{ 190, 130, 235, 255 };
            case TileKind::MiniBoss: return Color{ 235, 125,  60, 255 };
            case TileKind::Boss:     return COL_MAGENTA_LIT;
        }
        return COL_CREAM;
    }

    // Un dibujito por tipo de casilla. Nada sofisticado: formas planas que se
    // leen de un vistazo y no necesitan imágenes.
    static void DrawTileGlyph(TileKind kind, float cx, float cy, Color color)
    {
        switch (kind)
        {
            case TileKind::Event:
                DrawRectangleRounded(Rectangle{ cx - 8.0f, cy - 9.0f, 16.0f, 18.0f }, 0.3f, 4, color);
                DrawRectangleRounded(Rectangle{ cx - 4.0f, cy - 5.0f, 8.0f, 2.0f }, 0.5f, 3, COL_INK_SOFT);
                DrawRectangleRounded(Rectangle{ cx - 4.0f, cy - 1.0f, 8.0f, 2.0f }, 0.5f, 3, COL_INK_SOFT);
                break;

            case TileKind::Enemy:
            case TileKind::MiniBoss:
            {
                DrawCircle(static_cast<int>(cx), static_cast<int>(cy) + 1, 9.0f, color);
                DrawTriangle(Vector2{ cx - 9.0f, cy - 4.0f },
                             Vector2{ cx - 11.0f, cy - 13.0f },
                             Vector2{ cx - 2.0f, cy - 8.0f }, color);
                DrawTriangle(Vector2{ cx + 9.0f, cy - 4.0f },
                             Vector2{ cx + 2.0f, cy - 8.0f },
                             Vector2{ cx + 11.0f, cy - 13.0f }, color);
                DrawCircle(static_cast<int>(cx) - 3, static_cast<int>(cy), 2.0f, COL_INK_DEEP);
                DrawCircle(static_cast<int>(cx) + 3, static_cast<int>(cy), 2.0f, COL_INK_DEEP);
            } break;

            case TileKind::Treasure:
                DrawRectangleRounded(Rectangle{ cx - 10.0f, cy - 6.0f, 20.0f, 14.0f }, 0.25f, 4, color);
                DrawRectangle(static_cast<int>(cx) - 10, static_cast<int>(cy) - 1,
                              20, 3, Fade(COL_INK_DEEP, 0.75f));
                DrawRectangle(static_cast<int>(cx) - 2, static_cast<int>(cy) - 6,
                              4, 14, Fade(COL_INK_DEEP, 0.75f));
                break;

            case TileKind::Rest:
                DrawTriangle(Vector2{ cx, cy - 11.0f },
                             Vector2{ cx - 9.0f, cy + 7.0f },
                             Vector2{ cx + 9.0f, cy + 7.0f }, color);
                DrawRectangle(static_cast<int>(cx) - 10, static_cast<int>(cy) + 6, 20, 3, color);
                break;

            case TileKind::Optional:
                DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 10.0f, color);
                DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 9.0f, color);
                UiTextCentered("?", static_cast<int>(cx), static_cast<int>(cy) - 10, 19,
                               color, FontStyle::Bold);
                break;

            case TileKind::Boss:
                DrawCrown(static_cast<int>(cx), static_cast<int>(cy), 22.0f, color);
                break;
        }
    }

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

    void DrawBoard(int currentStage, bool revealAll)
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

                TileKind kind = GetTileKind(path, stage);

                // Sin el mapa de la montaña sólo se ve lo que ya pisaste y la
                // etapa en la que estás; el resto queda tapado.
                bool known = revealAll || stage <= currentStage;

                Color tileColor = known ? GetTileColor(kind) : Fade(color, 0.45f);

                Rectangle shadow = tile;
                shadow.x += 3.0f; shadow.y += 4.0f;
                DrawRectangleRounded(shadow, 0.28f, 6, Fade(BLACK, 0.35f));

                DrawRectangleRounded(tile, 0.28f, 6, COL_INK_SOFT);
                DrawRectangleRounded(Rectangle{ tile.x, tile.y, tile.width, tile.height * 0.45f },
                                     0.4f, 6, Fade(tileColor, 0.22f));
                DrawRectangleRoundedLinesEx(tile, 0.28f, 6, 2.0f, Fade(tileColor, 0.85f));

                if (known)
                {
                    DrawTileGlyph(kind, center.x, center.y - 5.0f, tileColor);

                    if (TileIsFight(kind) || kind == TileKind::Event)
                    {
                        UiTextCentered(TextFormat("%i", GetEventDifficulty(path, stage)),
                                       static_cast<int>(center.x), static_cast<int>(center.y) + 9,
                                       16, Fade(COL_CREAM, 0.8f), FontStyle::Bold);
                    }
                }
                else
                {
                    UiTextCentered("?", static_cast<int>(center.x),
                                   static_cast<int>(center.y) - 14, 26,
                                   Fade(COL_CREAM, 0.35f), FontStyle::Bold);
                }
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

        UiText("LA CIMA", static_cast<int>(COMBAT_X) + 96, static_cast<int>(COMBAT_Y) + 14,
               21, COL_GOLD, FontStyle::Bold);
        UiText("el jefe final y la corona", static_cast<int>(COMBAT_X) + 96,
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
