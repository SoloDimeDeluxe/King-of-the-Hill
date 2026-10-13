#include "Fx.h"
#include "Theme.h"
#include "Types.h"

#include <cmath>

namespace ReyesCuadra
{
    static float Lerp01(float a, float b, float t) { return a + (b - a) * t; }

    static Color MixColor(Color a, Color b, float t)
    {
        Color out;
        out.r = static_cast<unsigned char>(Lerp01(a.r, b.r, t));
        out.g = static_cast<unsigned char>(Lerp01(a.g, b.g, t));
        out.b = static_cast<unsigned char>(Lerp01(a.b, b.b, t));
        out.a = 255;
        return out;
    }

    static void DrawRidge(float baseY, float height, float step, float phase, Color color)
    {
        int width = SCREEN_WIDTH;

        for (int x = 0; x < width; x += 4)
        {
            float fx = static_cast<float>(x);
            float h = std::sin(fx * step + phase) * height
                    + std::sin(fx * step * 2.3f + phase * 1.7f) * height * 0.35f;

            float top = baseY - height - h;
            DrawRectangle(x, static_cast<int>(top), 5,
                          static_cast<int>(SCREEN_HEIGHT - top), color);
        }
    }

    void DrawBackdrop(float time)
    {
        int width  = SCREEN_WIDTH;
        int height = SCREEN_HEIGHT;

        // Cielo: degradado de azul noche con un resplandor magenta cerca del horizonte.
        DrawRectangleGradientV(0, 0, width, height, COL_INK_DEEP, COL_INK);
        DrawRectangleGradientV(0, static_cast<int>(height * 0.32f), width,
                               static_cast<int>(height * 0.30f),
                               Fade(COL_MAGENTA, 0.0f), Fade(COL_MAGENTA, 0.22f));

        // Tres cadenas de montañas, de la más lejana a la más cercana.
        DrawRidge(height * 0.62f, 26.0f, 0.0075f, 1.3f, MixColor(COL_INK, COL_MAGENTA, 0.10f));
        DrawRidge(height * 0.72f, 34.0f, 0.0052f, 4.1f, MixColor(COL_INK_DEEP, COL_MAGENTA, 0.07f));
        DrawRidge(height * 0.86f, 30.0f, 0.0039f, 0.4f, COL_INK_DEEP);

        // Nieve: puntos lentos, pocos, sin librería de partículas.
        for (int i = 0; i < 70; ++i)
        {
            float seed = static_cast<float>(i);
            float speed = 14.0f + std::fmod(seed * 7.3f, 20.0f);
            float x = std::fmod(seed * 137.0f + std::sin(seed) * 40.0f, static_cast<float>(width));
            float y = std::fmod(seed * 53.0f + time * speed, static_cast<float>(height));
            float drift = std::sin(time * 0.6f + seed) * 6.0f;
            float alpha = 0.10f + 0.12f * std::fmod(seed, 3.0f);

            DrawCircleV(Vector2{ x + drift, y }, 1.6f, Fade(COL_CREAM, alpha));
        }
    }

    void DrawWaveBand(Rectangle area, Color color, float time, int lines)
    {
        for (int i = 0; i < lines; ++i)
        {
            float offset = area.height * (static_cast<float>(i) + 0.5f) / static_cast<float>(lines);
            float amplitude = 6.0f + 2.0f * static_cast<float>(i % 3);
            float phase = time * 0.35f + static_cast<float>(i) * 0.6f;

            Vector2 previous{ area.x, area.y + offset };

            for (int x = 4; x <= static_cast<int>(area.width); x += 4)
            {
                float fx = area.x + static_cast<float>(x);
                float y = area.y + offset + std::sin(fx * 0.012f + phase) * amplitude;
                Vector2 current{ fx, y };
                DrawLineEx(previous, current, 3.0f, color);
                previous = current;
            }
        }
    }

    void DrawSoftPanel(Rectangle bounds, Color fill, Color border, float roundness)
    {
        Rectangle shadow = bounds;
        shadow.x += 4.0f;
        shadow.y += 5.0f;

        DrawRectangleRounded(shadow, roundness, 8, Fade(BLACK, 0.35f));
        DrawRectangleRounded(bounds, roundness, 8, fill);
        DrawRectangleRoundedLinesEx(bounds, roundness, 8, 2.0f, border);
    }

    // Ojo: raylib descarta los triángulos que no estén en sentido antihorario
    // en pantalla. El orden correcto es pico, base izquierda, base derecha.
    void DrawCrown(int x, int y, float size, Color color)
    {
        float w = size;
        float h = size * 0.7f;

        float centerX = static_cast<float>(x);
        float baseY   = static_cast<float>(y) + h * 0.5f;
        float leftX   = centerX - w * 0.5f;
        float rightX  = centerX + w * 0.5f;
        float innerL  = centerX - w * 0.17f;
        float innerR  = centerX + w * 0.17f;

        DrawTriangle(Vector2{ leftX + w * 0.14f, baseY - h },
                     Vector2{ leftX, baseY },
                     Vector2{ innerL, baseY }, color);

        DrawTriangle(Vector2{ centerX, baseY - h * 1.4f },
                     Vector2{ innerL, baseY },
                     Vector2{ innerR, baseY }, color);

        DrawTriangle(Vector2{ rightX - w * 0.14f, baseY - h },
                     Vector2{ innerR, baseY },
                     Vector2{ rightX, baseY }, color);

        DrawRectangleRounded(Rectangle{ leftX, baseY - 3.0f, w, h * 0.36f }, 0.4f, 6, color);
    }

    float EaseOutBack(float t)
    {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;

        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        float p = t - 1.0f;
        return 1.0f + c3 * p * p * p + c1 * p * p;
    }

    float Pulse(float time, float speed)
    {
        return 0.5f + 0.5f * std::sin(time * speed);
    }

    int SpinningValue(float time, int maxValue)
    {
        int step = static_cast<int>(time * 26.0f);
        return 1 + ((step * 7 + step / 3) % maxValue);
    }
}
