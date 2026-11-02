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

    const char* GetTileName(TileKind kind)
    {
        switch (kind)
        {
            case TileKind::Event:    return "EVENTO";
            case TileKind::Enemy:    return "ENEMIGO";
            case TileKind::Treasure: return "TESORO";
            case TileKind::Rest:     return "DESCANSO";
            case TileKind::Optional: return "BATALLA OPCIONAL";
            case TileKind::MiniBoss: return "MINI JEFE";
            case TileKind::Boss:     return "JEFE DE LA CIMA";
        }
        return "?";
    }

    const char* GetTileShortName(TileKind kind)
    {
        switch (kind)
        {
            case TileKind::Event:    return "evento";
            case TileKind::Enemy:    return "enemigo";
            case TileKind::Treasure: return "tesoro";
            case TileKind::Rest:     return "descanso";
            case TileKind::Optional: return "opcional";
            case TileKind::MiniBoss: return "mini jefe";
            case TileKind::Boss:     return "jefe";
        }
        return "?";
    }

    bool TileIsFight(TileKind kind)
    {
        return kind == TileKind::Enemy ||
               kind == TileKind::Optional ||
               kind == TileKind::MiniBoss ||
               kind == TileKind::Boss;
    }
}
