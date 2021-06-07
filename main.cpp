#include "raylib-cpp.hpp"

int main()
{
  const int screenWidth = 800;
  const int screenHeight = 450;

  raylib::Window window(screenWidth, screenHeight, "raylib [shapes] example - collision area");
  SetTargetFPS(60);

  while (!window.ShouldClose())
  {    // Detect window close button or ESC key
    BeginDrawing();

    window.ClearBackground(RAYWHITE);

    EndDrawing();
  }

  return 0;
}
