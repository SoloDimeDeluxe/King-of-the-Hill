#include "Types.h"

namespace ReyesCuadra
{
    const char* GetPathName(PathKind path)
    {
        switch (path)
        {
            case PathKind::Easy:   return "FÁCIL";
            case PathKind::Normal: return "NORMAL";
            case PathKind::Hard:   return "DIFÍCIL";
        }
        return "?";
    }

    Color GetPathColor(PathKind path)
    {
        switch (path)
        {
            case PathKind::Easy:   return Color{ 165, 172, 185, 255 };
            case PathKind::Normal: return Color{ 225,  75,  75, 255 };
            case PathKind::Hard:   return Color{ 170, 115, 225, 255 };
        }
        return GRAY;
    }

    const char* GetCardLevelName(CardLevel level)
    {
        switch (level)
        {
            case CardLevel::Weak:   return "Común";
            case CardLevel::Normal: return "Peligrosa";
            case CardLevel::Epic:   return "Épica";
        }
        return "?";
    }
}
