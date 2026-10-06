#pragma once

#include "raylib.h"

namespace ReyesCuadra
{
    void  DrawBackdrop(float time);
    void  DrawWaveBand(Rectangle area, Color color, float time, int lines);
    void  DrawSoftPanel(Rectangle bounds, Color fill, Color border, float roundness);
    void  DrawCrown(int x, int y, float size, Color color);

    float EaseOutBack(float t);
    float Pulse(float time, float speed);

    int   SpinningValue(float time, int maxValue);
}
