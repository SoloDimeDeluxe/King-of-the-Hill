#include "Game.h"
#include "Dice.h"
#include "Ui.h"
#include "Audio.h"

namespace ReyesCuadra
{
    // El juego se dibuja siempre sobre un lienzo de 1280x720 y después se escala
    // a la ventana. Así la pantalla completa no descoloca nada.
    static Rectangle GetCanvasDestination()
    {
        float windowWidth  = static_cast<float>(GetScreenWidth());
        float windowHeight = static_cast<float>(GetScreenHeight());

        float scaleX = windowWidth  / static_cast<float>(SCREEN_WIDTH);
        float scaleY = windowHeight / static_cast<float>(SCREEN_HEIGHT);
        float scale  = (scaleX < scaleY) ? scaleX : scaleY;

        float width  = static_cast<float>(SCREEN_WIDTH)  * scale;
        float height = static_cast<float>(SCREEN_HEIGHT) * scale;

        return Rectangle{ (windowWidth - width) * 0.5f, (windowHeight - height) * 0.5f,
                          width, height };
    }

    static void RunGame()
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "King of the Hill - versión base");
        SetWindowMinSize(640, 360);
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);

        RenderTexture2D canvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
        SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

        LoadUiFonts();
        LoadAudio();
        InitDice();

        GameState game{};
        InitGame(game);

        Rectangle source{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH),
                          -static_cast<float>(SCREEN_HEIGHT) };

        bool quit = false;

        while (!WindowShouldClose() && !quit)
        {
            if (IsKeyPressed(KEY_F11) ||
                (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))))
            {
                ToggleBorderlessWindowed();
            }

            if (IsKeyPressed(KEY_ESCAPE))
            {
                if (IsWindowFullscreen() || IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
                else quit = true;
            }

            UpdateGame(game);
            UpdateAudio();

            BeginTextureMode(canvas);
            DrawGame(game);
            EndTextureMode();

            BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(canvas.texture, source, GetCanvasDestination(),
                           Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
            EndDrawing();
        }

        UnloadRenderTexture(canvas);
        UnloadAudio();
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
