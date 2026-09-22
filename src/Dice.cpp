#include "Dice.h"
#include "Types.h"

#include <ctime>

namespace ReyesCuadra
{
    void InitDice()
    {
        SetRandomSeed(static_cast<unsigned int>(time(nullptr)));
    }

    int RollRawD20()
    {
        return GetRandomValue(1, DICE_FACES);
    }

    int RandomInt(int min, int max)
    {
        return GetRandomValue(min, max);
    }
}
