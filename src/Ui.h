#pragma once

#include "Types.h"

namespace ReyesCuadra
{
    enum class FontStyle
    {
        Regular = 0,
        Bold    = 1
    };

    void LoadUiFonts();
    void UnloadUiFonts();

    void UiText(const char* text, int x, int y, int size, Color color,
                FontStyle style = FontStyle::Regular);

    int UiMeasure(const char* text, int size, FontStyle style = FontStyle::Regular);

    void UiTextCentered(const char* text, int centerX, int y, int size, Color color,
                        FontStyle style = FontStyle::Regular);

    int UiTextWrapped(const char* text, int x, int y, int maxWidth, int size, Color color,
                      int maxLines = 3, FontStyle style = FontStyle::Regular);
}
