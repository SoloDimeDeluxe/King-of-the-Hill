#pragma once

#include "Types.h"
#include "Effects.h"
#include "Enemy.h"

namespace ReyesCuadra
{
    PathKind CpuChoosePath(const Player& player, int stage);
    int      CpuChooseCard(const int options[], int optionCount);
    int      CpuChooseOption(const PendingChoice& choice);
    int      CpuChoosePart(const Enemy& enemy, PathKind path, int stage);
    bool     CpuKeepsFighting(const Enemy& enemy);
}
