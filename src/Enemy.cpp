#include "Enemy.h"
#include "Board.h"

namespace ReyesCuadra
{
    // ---------------------------------------------------------------------
    // Versión provisional del combate PvE descrito en docs/propuesta-combate-pve.md.
    // Todo lo que el grupo todavía está decidiendo vive en esta tabla: qué
    // etapas son combate, qué partes tiene cada enemigo, cuántos turnos dura y
    // cuántas partes hay que derribar. Cambiar el juego es cambiar números acá.
    // ---------------------------------------------------------------------

    bool StageHasEnemy(int stage)
    {
        return (stage == 1 || stage == 3 || stage == 5);
    }

    EnemyRank GetStageEnemyRank(int stage)
    {
        if (stage >= 5) return EnemyRank::Boss;
        if (stage >= 3) return EnemyRank::MiniBoss;
        return EnemyRank::Common;
    }

    const char* GetEnemyRankName(EnemyRank rank)
    {
        switch (rank)
        {
            case EnemyRank::MiniBoss: return "MINI JEFE";
            case EnemyRank::Boss:     return "JEFE";
            default:                  return "ENEMIGO";
        }
    }

    static EnemyPart MakePart(const char* name, int offset, int wounds, bool counts, float width)
    {
        EnemyPart part;
        part.name       = name;
        part.offset     = offset;
        part.wounds     = wounds;
        part.woundsLeft = wounds;
        part.counts     = counts;
        part.width      = width;
        return part;
    }

    Enemy MakeEnemy(int stage)
    {
        Enemy enemy;
        enemy.rank = GetStageEnemyRank(stage);

        switch (enemy.rank)
        {
            case EnemyRank::MiniBoss:
            {
                enemy.name     = "GUARDIÁN DE LA LADERA";
                enemy.subtitle = "derribá patas, cuerpo y ala para ganar";

                enemy.parts[0] = MakePart("Patas",  -5, 1, true,  200.0f);
                enemy.parts[1] = MakePart("Cuerpo", -3, 1, true,  268.0f);
                enemy.parts[2] = MakePart("Ala",    -2, 1, true,  232.0f);
                enemy.parts[3] = MakePart("Cabeza",  2, 1, false, 176.0f);

                enemy.partCount   = 4;
                enemy.turns       = 5;
                enemy.neededDowns = 3;
            } break;

            case EnemyRank::Boss:
            {
                enemy.name     = "SEÑOR DE LA CIMA";
                enemy.subtitle = "derribá tres de sus cuatro partes";

                enemy.parts[0] = MakePart("Cola",   -5, 1, true,  200.0f);
                enemy.parts[1] = MakePart("Cuerpo", -4, 1, true,  276.0f);
                enemy.parts[2] = MakePart("Garra",  -3, 1, true,  236.0f);
                enemy.parts[3] = MakePart("Cabeza",  1, 2, false, 180.0f);

                enemy.partCount   = 4;
                enemy.turns       = 5;
                enemy.neededDowns = 3;
            } break;

            default:
            {
                enemy.name     = "BESTIA DEL CAMINO";
                enemy.subtitle = "derribá cola y cuerpo para ganar";

                enemy.parts[0] = MakePart("Cola",   -4, 1, true,  196.0f);
                enemy.parts[1] = MakePart("Cuerpo", -2, 1, true,  264.0f);
                enemy.parts[2] = MakePart("Cabeza",  2, 1, false, 172.0f);

                enemy.partCount   = 3;
                enemy.turns       = 3;
                enemy.neededDowns = 2;
            } break;
        }

        enemy.turnsLeft = enemy.turns;
        return enemy;
    }

    int GetPartDifficulty(const Enemy& enemy, int part, PathKind path, int stage)
    {
        if (part < 0 || part >= enemy.partCount) return 0;
        return GetEventDifficulty(path, stage) + enemy.parts[part].offset;
    }

    bool IsPartDown(const Enemy& enemy, int part)
    {
        if (part < 0 || part >= enemy.partCount) return true;
        return enemy.parts[part].woundsLeft <= 0;
    }

    int CountDownedCounting(const Enemy& enemy)
    {
        int total = 0;

        for (int i = 0; i < enemy.partCount; ++i)
        {
            if (enemy.parts[i].counts && enemy.parts[i].woundsLeft <= 0) ++total;
        }

        return total;
    }

    bool EnemyBeaten(const Enemy& enemy)
    {
        return CountDownedCounting(enemy) >= enemy.neededDowns;
    }

    bool BonusPartDown(const Enemy& enemy)
    {
        for (int i = 0; i < enemy.partCount; ++i)
        {
            if (!enemy.parts[i].counts && enemy.parts[i].woundsLeft <= 0) return true;
        }

        return false;
    }

    int FirstAlivePart(const Enemy& enemy)
    {
        for (int i = 0; i < enemy.partCount; ++i)
        {
            if (enemy.parts[i].woundsLeft > 0) return i;
        }

        return 0;
    }
}
