#pragma once

#include "Types.h"

namespace ReyesCuadra
{
    constexpr int MAX_ENEMY_PARTS = 4;

    enum class EnemyRank
    {
        Common   = 0,
        MiniBoss = 1,
        Boss     = 2
    };

    struct EnemyPart
    {
        const char* name       = "";
        int         offset     = 0;
        int         wounds     = 1;
        int         woundsLeft = 1;
        bool        counts     = true;
        float       width      = 200.0f;
    };

    struct Enemy
    {
        const char* name        = "";
        const char* subtitle    = "";
        EnemyRank   rank        = EnemyRank::Common;
        EnemyPart   parts[MAX_ENEMY_PARTS];
        int         partCount   = 0;
        int         turns       = 3;
        int         turnsLeft   = 3;
        int         neededDowns = 2;
    };

    bool        StageHasEnemy(int stage);
    EnemyRank   GetStageEnemyRank(int stage);
    const char* GetEnemyRankName(EnemyRank rank);

    Enemy MakeEnemy(int stage);

    int  GetPartDifficulty(const Enemy& enemy, int part, PathKind path, int stage);
    bool IsPartDown(const Enemy& enemy, int part);
    int  CountDownedCounting(const Enemy& enemy);
    bool EnemyBeaten(const Enemy& enemy);
    bool BonusPartDown(const Enemy& enemy);
    int  FirstAlivePart(const Enemy& enemy);
}
