#include <raylib.h>
#include "common.h"
#include "game.h"


int main() {

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    globals::appSettings.name = "Breakout";

#if DEVELOPER
    globals::appSettings.windowWidth = 1920;
    globals::appSettings.windowHeight = 1080;
    InitWindow(globals::appSettings.windowWidth, globals::appSettings.windowHeight, globals::appSettings.name);
#else
    int monitor = GetCurrentMonitor();
    globals::appSettings.windowWidth = GetMonitorWidth(monitor);
    globals::appSettings.windowHeight = GetMonitorHeight(monitor);
    InitWindow(globals::appSettings.windowWidth, globals::appSettings.windowHeight, globals::appSettings.name);
    ToggleFullscreen();
#endif

    SetWindowMinSize(640, 480);
    SetExitKey(0);
    SetTargetFPS(120);
    DisableCursor();

    if (!IsWindowReady()) {
        TraceLog(LOG_ERROR, "Window initialization is failed!");
        return 1;
    }

    globals::appSettings.screenWidth = 1920;
    globals::appSettings.screenHeight = 1080;

    RenderTexture2D target = LoadRenderTexture(globals::appSettings.screenWidth, globals::appSettings.screenHeight);

    breakout::Initialize();

    while (!WindowShouldClose()) {

        bool exitRequested = false;
        f32 dt = GetFrameTime();
        
        breakout::Update(dt, exitRequested);

        if (exitRequested) {
            break;
        }

        BeginTextureMode(target);
        ClearBackground(DARKGRAY);

        breakout::Draw();

        EndTextureMode();

        BeginDrawing();

        ClearBackground(BLACK);

        f32 resolutionScale = globals::appSettings.GetResolutionScale();
        DrawTexturePro(target.texture,
            Rectangle{ 0, 0, static_cast<f32>(target.texture.width), static_cast<f32>(-target.texture.height)},
            Rectangle{ (GetScreenWidth() - (globals::appSettings.screenWidth * resolutionScale)) * 0.5f,
                    (GetScreenHeight() - (globals::appSettings.screenHeight * resolutionScale)) * 0.5f,
                    globals::appSettings.screenWidth * resolutionScale,
                    globals::appSettings.screenHeight * resolutionScale },
            Vector2{ 0,0 },
            0.0f, WHITE);

        EndDrawing();

    }

    CloseWindow();

    return 0;
}