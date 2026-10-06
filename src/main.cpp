#include "Game.h"
#include "Dice.h"
#include "Ui.h"
#include "Audio.h"

namespace ReyesCuadra
{
    static bool   fullscreenOn    = false;
    static int    windowedWidth   = SCREEN_WIDTH;
    static int    windowedHeight  = SCREEN_HEIGHT;
    static double lastToggleTime  = -1.0;

    // El juego se dibuja siempre sobre un lienzo de 1280x720 y después se escala
    // a la ventana, centrado y conservando la proporción.
    static Rectangle GetCanvasDestination()
    {
        float windowWidth  = static_cast<float>(GetScreenWidth());
        float windowHeight = static_cast<float>(GetScreenHeight());

        if (windowWidth < 1.0f || windowHeight < 1.0f)
        {
            return Rectangle{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH),
                              static_cast<float>(SCREEN_HEIGHT) };
        }

        float scaleX = windowWidth  / static_cast<float>(SCREEN_WIDTH);
        float scaleY = windowHeight / static_cast<float>(SCREEN_HEIGHT);
        float scale  = (scaleX < scaleY) ? scaleX : scaleY;

        float width  = static_cast<float>(SCREEN_WIDTH)  * scale;
        float height = static_cast<float>(SCREEN_HEIGHT) * scale;

        return Rectangle{ (windowWidth - width) * 0.5f, (windowHeight - height) * 0.5f,
                          width, height };
    }

    // Pantalla completa "de ventana": una ventana sin bordes del tamaño del
    // monitor. No toma el control exclusivo de la pantalla, así que Alt+Tab, la
    // tecla Windows y el Administrador de tareas siguen funcionando aunque el
    // juego se cuelgue.
    static void ApplyFullscreen(bool enable)
    {
        if (enable == fullscreenOn) return;

        // Un cambio cada medio segundo como mucho: F11 y Alt+Enter en el mismo
        // frame dejaban la ventana en un estado raro.
        if (GetTime() - lastToggleTime < 0.5) return;
        lastToggleTime = GetTime();

        int monitor = GetCurrentMonitor();
        int monitorWidth  = GetMonitorWidth(monitor);
        int monitorHeight = GetMonitorHeight(monitor);

        if (monitorWidth <= 0 || monitorHeight <= 0) return;

        Vector2 monitorPosition = GetMonitorPosition(monitor);
        int originX = static_cast<int>(monitorPosition.x);
        int originY = static_cast<int>(monitorPosition.y);

        if (enable)
        {
            windowedWidth  = GetScreenWidth();
            windowedHeight = GetScreenHeight();

            SetWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowPosition(originX, originY);
            SetWindowSize(monitorWidth, monitorHeight);
        }
        else
        {
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowSize(windowedWidth, windowedHeight);
            SetWindowPosition(originX + (monitorWidth  - windowedWidth)  / 2,
                              originY + (monitorHeight - windowedHeight) / 2);
        }

        fullscreenOn = enable;
    }

    static void RunGame()
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "King of the Hill - versión base");
        SetWindowMinSize(640, 360);
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

        while (!WindowShouldClose())
        {
            bool altDown = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
            bool toggled = false;

            if (IsKeyPressed(KEY_F11) || (altDown && IsKeyPressed(KEY_ENTER)))
            {
                ApplyFullscreen(!fullscreenOn);
                toggled = true;
            }

            if (!toggled && !IsWindowMinimized()) UpdateGame(game);

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

        if (fullscreenOn) ClearWindowState(FLAG_WINDOW_UNDECORATED);

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
