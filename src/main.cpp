#include "raylib-cpp.hpp"

#include <vector>
using namespace std;

class Entity
{
public:
  const string &name;
  const raylib::Color &color;
  raylib::Rectangle body;
  const vector<Entity*> &scanningCollisionGroup;

  Entity(
    const string &name,
    const raylib::Color &color,
    raylib::Rectangle shape,
    const vector<Entity*> &scanningCollisionGroup,
    vector<Entity*> &belongingCollisionGroup

  ) :
    name(name),
    color(color),
    body(shape),
    scanningCollisionGroup(scanningCollisionGroup)
  {
    belongingCollisionGroup.push_back(this);
  };

  void spawn()
  {
    body.Draw(color);

    for (auto *collision: scanningCollisionGroup)
    {
      if (body.CheckCollision(collision->body))
      {
        DrawText(("collided with " + collision->name + " !").c_str(), 0, 10, 10, RED);
      }
    }
  }
};

vector<Entity*> playerGroup;
vector<Entity*> passiveGroup;
vector<Entity*> hostileGroup;
vector<Entity*> environmentGroup;

int main()
{
  const int screenWidth = 800;
  const int screenHeight = 450;

  raylib::Window window(screenWidth, screenHeight, "raylib game - Henry Liu");
  SetTargetFPS(60);

  Entity player(
    "player", BLUE,
    raylib::Rectangle(200, 100, 100, 150),
    playerGroup,
    hostileGroup
  );

  Entity enemy(
    "enemy", RED,
    raylib::Rectangle(400, 400, 100, 150),
    hostileGroup,
    playerGroup
  );

  while (!window.ShouldClose())
  {
    BeginDrawing();

    window.ClearBackground(RAYWHITE);

    raylib::Vector2 inputVector(
      (float) (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
      (float) (IsKeyDown(KEY_S) - IsKeyDown(KEY_W))
    );

    if (!(inputVector == raylib::Vector2(0, 0)))
    {
      raylib::Vector2 normalizedInputVector(inputVector.Normalize());
      DrawText(("normalizedInputVector: " + to_string(normalizedInputVector.x) + " "
        + to_string(normalizedInputVector.y)).c_str(), 0, 0, 10, GOLD);
      player.body.SetPosition(normalizedInputVector * 10 + player.body.GetPosition());
    }

    player.spawn();
    enemy.spawn();

    EndDrawing();
  }

  return 0;
}
