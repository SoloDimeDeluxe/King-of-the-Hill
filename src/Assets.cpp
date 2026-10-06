#include "Assets.h"
#include "raylib.h"

namespace ReyesCuadra
{
    const char* FindAssetPath(const char* relativePath)
    {
        if (FileExists(relativePath)) return relativePath;

        const char* nextToExe = TextFormat("%s%s", GetApplicationDirectory(), relativePath);
        if (FileExists(nextToExe)) return nextToExe;

        const char* upFromExe = TextFormat("%s../../../%s", GetApplicationDirectory(), relativePath);
        if (FileExists(upFromExe)) return upFromExe;

        return nullptr;
    }
}
