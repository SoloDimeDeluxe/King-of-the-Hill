#pragma once

#include "Types.h"
#include "Effects.h"
#include "Enemy.h"
#include "Items.h"

namespace ReyesCuadra
{
    PathKind CpuChoosePath(const Player& player, int stage);
    int      CpuChooseCard(const int options[], int optionCount);
    int      CpuChooseOption(const PendingChoice& choice);
    int      CpuChoosePart(const Player& player, const Enemy& enemy, int stage);
    int      CpuChooseItem(const Player& player, int difficulty, bool isFight, int woundsLeft);
    bool     CpuKeepsFighting(const Enemy& enemy);
    bool     CpuTakesOptionalFight(const Player& player);
}
