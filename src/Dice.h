#pragma once

namespace ReyesCuadra
{
    struct DiceRoll
    {
        int  natural      = 0;
        int  naturalRaw   = 0;
        int  dice         = 1;
        int  modifier     = 0;
        int  total        = 0;
        bool criticalHit  = false;
        bool criticalFail = false;
    };

    void InitDice();
    int  RollRawD20();
    int  RandomInt(int min, int max);
}
