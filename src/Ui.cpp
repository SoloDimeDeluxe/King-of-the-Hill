#include "Ui.h"
#include "Assets.h"

#include <string>
#include <vector>

namespace ReyesCuadra
{
    constexpr int UI_FONT_BASE_SIZE = 64;

    static Font uiRegularFont;
    static Font uiBoldFont;
    static bool uiFontsLoaded = false;

    static Font LoadUiFont(const char* relativePath, const std::vector<int>& codepoints)
    {
        Font font{};

        const char* path = FindAssetPath(relativePath);
        if (path == nullptr) return font;

        font = LoadFontEx(path, UI_FONT_BASE_SIZE,
                          const_cast<int*>(codepoints.data()),
                          static_cast<int>(codepoints.size()));

        if (font.texture.id != 0 && font.glyphCount > 0)
        {
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        }

        return font;
    }

    void LoadUiFonts()
    {
        std::vector<int> codepoints;
        for (int c = 32; c <= 126; ++c)  codepoints.push_back(c);
        for (int c = 161; c <= 255; ++c) codepoints.push_back(c);

        uiRegularFont = LoadUiFont("assets/fonts/WorkSans-Regular.ttf", codepoints);
        uiBoldFont    = LoadUiFont("assets/fonts/WorkSans-Bold.ttf", codepoints);

        uiFontsLoaded = (uiRegularFont.glyphCount > 0 && uiBoldFont.glyphCount > 0);
    }

    void UnloadUiFonts()
    {
        if (!uiFontsLoaded) return;

        UnloadFont(uiRegularFont);
        UnloadFont(uiBoldFont);
        uiFontsLoaded = false;
    }

    static float GetUiSpacing(int size)
    {
        return static_cast<float>(size) / 20.0f;
    }

    void UiText(const char* text, int x, int y, int size, Color color, FontStyle style)
    {
        if (!uiFontsLoaded)
        {
            DrawText(text, x, y, size, color);
            return;
        }

        const Font& font = (style == FontStyle::Bold) ? uiBoldFont : uiRegularFont;

        DrawTextEx(font, text,
                   Vector2{ static_cast<float>(x), static_cast<float>(y) },
                   static_cast<float>(size), GetUiSpacing(size), color);
    }

    int UiMeasure(const char* text, int size, FontStyle style)
    {
        if (!uiFontsLoaded) return MeasureText(text, size);

        const Font& font = (style == FontStyle::Bold) ? uiBoldFont : uiRegularFont;

        Vector2 measured = MeasureTextEx(font, text, static_cast<float>(size), GetUiSpacing(size));
        return static_cast<int>(measured.x);
    }

    void UiTextCentered(const char* text, int centerX, int y, int size, Color color, FontStyle style)
    {
        int width = UiMeasure(text, size, style);
        UiText(text, centerX - width / 2, y, size, color, style);
    }

    int UiTextWrapped(const char* text, int x, int y, int maxWidth, int size, Color color,
                      int maxLines, FontStyle style)
    {
        std::string words = text;
        std::string line;
        int lineCount = 0;
        size_t cursor = 0;

        while (cursor <= words.size() && lineCount < maxLines)
        {
            size_t space = words.find(' ', cursor);
            std::string word = words.substr(cursor, space - cursor);

            std::string candidate = line.empty() ? word : line + " " + word;

            if (!line.empty() && UiMeasure(candidate.c_str(), size, style) > maxWidth)
            {
                UiText(line.c_str(), x, y + lineCount * (size + 4), size, color, style);
                ++lineCount;
                line = word;
            }
            else
            {
                line = candidate;
            }

            if (space == std::string::npos) break;
            cursor = space + 1;
        }

        if (!line.empty() && lineCount < maxLines)
        {
            UiText(line.c_str(), x, y + lineCount * (size + 4), size, color, style);
            ++lineCount;
        }

        return lineCount;
    }
}
