#include <cmath>

#include "chunkManager.hpp"
#include "raylib.h"
#include "raymath.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

int screenWidth = 1280;
int screenHeight = 720;
Camera2D camera;

struct Player {
  Vector2 position = {0.0f, 0.0f};
  Vector2 lastPosition = {0.0f, 0.0f};
  float moveSpeed = 100.0f;
  float sprintSpeed = 300.0f;
  int mineRadius = 8;
};

Player player;

void UpdateDrawFrame(void);  // Update and Draw one frame
void BetterDrawFPS(Vector2 position);

int main() {
  // Initialization
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, "ooga booga minin' pixels");
  SetWindowIcon(GenImageGradientLinear(128, 128, 230, MAGENTA, SKYBLUE)); 
  InitializeChunks();

  UpdateChunks(camera.target);  // *debug (since initialize chunks is empty)

  camera.target = player.position;
  camera.zoom = 4.0f;
  camera.offset = (Vector2){(float)screenWidth / 2, (float)screenHeight / 2};

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else

  // SetTargetFPS(60);

  // Main game loop
  while (!WindowShouldClose())  // Detect window close button or ESC key
  {
    UpdateDrawFrame();
  }
#endif

  // De-Initialization
  CloseWindow();  // Close window and OpenGL context

  return 0;
}

void UpdateDrawFrame(void) {
  // Update
  if (IsKeyPressed(KEY_F11)) {
    ToggleBorderlessWindowed();
  }
  if (IsWindowResized()) {
    camera.offset =
        (Vector2){(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2};
  }

  player.lastPosition = player.position;

  // Input (placeholder)
  Vector2 playerMoveDir = {0.0f, 0.0f};
  if (IsKeyDown(KEY_D)) {
    playerMoveDir.x += 1.0f;
  }
  if (IsKeyDown(KEY_A)) {
    playerMoveDir.x -= 1.0f;
  }
  if (IsKeyDown(KEY_W)) {
    playerMoveDir.y -= 1.0f;
  }
  if (IsKeyDown(KEY_S)) {
    playerMoveDir.y += 1.0f;
  }
  playerMoveDir = Vector2Normalize(playerMoveDir);

  if (IsKeyDown(KEY_LEFT_SHIFT)) {
    playerMoveDir *= player.sprintSpeed * GetFrameTime();
  } else {
    playerMoveDir *= player.moveSpeed * GetFrameTime();
  }

  // Apply movement vector and check collision (X and Y separately)
  player.position.x += playerMoveDir.x;
  if (CheckCollisionRect(Rectangle{player.position.x - 5.0f,
                                   player.position.y - 5.0f, 10.0f, 10.0f})) {
    player.position.x = player.lastPosition.x;
  }
  player.position.y += playerMoveDir.y;
  if (CheckCollisionRect(Rectangle{player.position.x - 5.0f,
                                   player.position.y - 5.0f, 10.0f, 10.0f})) {
    player.position.y = player.lastPosition.y;
  }

  camera.target = player.position;

  if (IsKeyPressed(KEY_R)) {
    ResetAllChunks();
  }
  if (IsKeyDown(KEY_SPACE)) {
    for (int i = -player.mineRadius; i < player.mineRadius; i++) {
      for (int j = -player.mineRadius; j < player.mineRadius; j++) {
        WorldDestroyPixelAt((Vector2){
            player.position.x + i, player.position.y + j});  // * destroy brush
      }
    }
  }
  UpdateChunks(camera.target);

  // * This fixes visual glitches happening due camera position being a float
  // * for some reason floor() isn't enough while maximizing the window and
  // * 0.001f bs is necessary
  // camera.target = {std::floor(camera.target.x) +
  // 0.001f,std::floor(camera.target.y) + 0.001f};

  // Draw
  BeginDrawing();
  ClearBackground(RAYWHITE);

  BeginMode2D(camera);
  DrawChunks();

  DrawCircleV(player.position, 5.0f, RED);

  //DrawChunkBorders(camera.target); //*Debug
  EndMode2D();

  BetterDrawFPS((Vector2){5.0f, 5.0f});
  EndDrawing();
}

void BetterDrawFPS(Vector2 position) {
  static bool shown = true;

  static int currentFPS;
  currentFPS = (int)std::roundf(1.0f / GetFrameTime());

  static int lowestFPS = INT_MAX;

  static Color textColor;

  int fontSize = 10;

  if (IsKeyPressed(KEY_F9)) {
    shown = !shown;
  }

  if (IsKeyPressed(KEY_F10)) {
    lowestFPS = currentFPS;
  }

  if (!shown) {
    return;
  }

  DrawRectangleRounded(
      (Rectangle){
          position.x, position.y,
          (float)MeasureText("(F9 - hide / show)(F10 - reset)", fontSize) +
              fontSize - 4.0f,
          fontSize * 4.0f},
      0.365f, 3, (Color){0, 0, 0, 127});

  if (currentFPS >= 180) {
    textColor = GREEN;
  } else if (currentFPS >= 60) {
    textColor = YELLOW;
  } else {
    textColor = RED;
  }

  DrawText(TextFormat("Current FPS: %d", currentFPS),
           position.x + fontSize * 0.5f, position.y + fontSize * 0.5f, fontSize,
           textColor);

  if (lowestFPS > currentFPS && currentFPS > 0 && GetTime() > 3.0f) {
    lowestFPS = currentFPS;
  }

  if (lowestFPS >= 180) {
    textColor = GREEN;
  } else if (lowestFPS >= 60) {
    textColor = YELLOW;
  } else {
    textColor = RED;
  }

  DrawText(TextFormat("Lowest FPS: %d ", lowestFPS),
           position.x + fontSize * 0.5f, position.y + fontSize * 1.5f, fontSize,
           textColor);
  DrawText("(F9 - hide/show) (F10 - reset)", position.x + fontSize * 0.5f,
           position.y + fontSize * 2.625f, fontSize, WHITE);
}