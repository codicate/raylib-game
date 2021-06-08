#include "include/raylib-cpp.hpp"

#include <iostream>
using namespace std;

int main()
{
  const int screenWidth = 800;
  const int screenHeight = 450;

  raylib::Window window(screenWidth, screenHeight, "raylib game - Henry Liu");
  SetTargetFPS(60);

  raylib::Rectangle player(0, 0, 20, 150);

  while (!window.ShouldClose())
  {
    BeginDrawing();

    window.ClearBackground(RAYWHITE);

    raylib::Vector2 inputVector(
      (float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
      (float)(IsKeyDown(KEY_S) - IsKeyDown(KEY_W))
    );

    if (!(inputVector == raylib::Vector2(0, 0)))
    {
      raylib::Vector2 normalizedInputVector(inputVector.Normalize());
      DrawText(("normalizedInputVector: " + to_string(normalizedInputVector.x) + " " + to_string(normalizedInputVector.y)).c_str(), 0, 0, 10, RED);
      player.SetPosition(normalizedInputVector * 10 + player.GetPosition());
    }

    player.Draw(GOLD);

    EndDrawing();
  }

  return 0;
}
