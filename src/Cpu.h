#pragma once

#include "Types.h"
#include "Effects.h"

namespace ReyesCuadra
{
    PathKind CpuChoosePath(const Player& player, int stage);
    int      CpuChooseCard(const int options[], int optionCount);
    int      CpuChooseOption(const PendingChoice& choice);
}
