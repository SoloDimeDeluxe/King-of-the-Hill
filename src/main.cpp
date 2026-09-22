#include "Game.h"
#include "Dice.h"
#include "Ui.h"

namespace ReyesCuadra
{
    static void RunGame()
    {
        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "King of the Hill - versión base");
        SetTargetFPS(60);

        LoadUiFonts();
        InitDice();

        GameState game{};
        InitGame(game);

        while (!WindowShouldClose())
        {
            UpdateGame(game);

            BeginDrawing();
            DrawGame(game);
            EndDrawing();
        }

        UnloadUiFonts();
        CloseWindow();
    }
}

using namespace ReyesCuadra;

int main()
{
    RunGame();
    return 0;
}
