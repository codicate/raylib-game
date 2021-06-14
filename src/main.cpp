#include "raylib-cpp.hpp"
#include<iostream>
#include <cmath>
#include <vector>

const int screenWidth = 800;
const int screenHeight = 450;

class Entity;
class Projectile;
class Player;

using CollisionGroup = std::vector<Entity *>;
using CollisionGroups = std::vector<CollisionGroup *>;

CollisionGroup playerCollisionGroup;
CollisionGroup passiveCollisionGroup;
CollisionGroup hostileCollisionGroup;
CollisionGroup environmentCollisionGroup;
CollisionGroup projectileCollisionGroup;

std::vector<Projectile> projectileList;

raylib::Camera2D camera;

class Entity
{
public:
  std::string name;
  raylib::Color color;
  raylib::Rectangle body;
  CollisionGroups scanningCollisionGroups;

  Entity(
    std::string name,
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups
  ) :
    name(name),
    color(color),
    body(shape),
    scanningCollisionGroups(scanningCollisionGroups)
  {
    for (auto belongingCollisionGroup : belongingCollisionGroups)
    {
      std::cout << belongingCollisionGroup << std::endl;
      belongingCollisionGroup->push_back(this);
    }
  };

  void spawn()
  {
    BeginMode2D(camera);
    body.Draw(color);
    EndMode2D();

    for (auto scanningCollisionGroup: scanningCollisionGroups)
    {
      for (auto collision: *scanningCollisionGroup)
      {
        if (body.CheckCollision(collision->body))
        {
          DrawText(("collided with " + collision->name + " !").c_str(), 0, 10, 10, RED);
        }
      }
    }
  }
};

class Projectile : public Entity
{
public:
  float speed;
  raylib::Vector2 direction;

  Projectile(
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroups scanningCollisionGroups,
    float speed,
    raylib::Vector2 direction
  ) :
    Entity(
      "bullet",
      color,
      shape,
      {&projectileCollisionGroup},
      scanningCollisionGroups
    ),
    speed(speed),
    direction(direction)
  {
  }

  void spawn()
  {
    Entity::spawn();

    body.SetPosition(direction * speed + body.GetPosition());
  }
};

class Player : public Entity
{
public:
  raylib::Vector2 velocity = {0, 0};
  raylib::Vector2 mouseInitialOffset;

  Player(
    std::string name,
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroups scanningCollisionGroups
  ) :
    Entity(
      name,
      color,
      shape,
      {&playerCollisionGroup},
      scanningCollisionGroups
    ),
    mouseInitialOffset(body.GetPosition())
  {
    camera = raylib::Camera2D(
      mouseInitialOffset,
      {screenWidth / 2.0, screenHeight / 2.0},
      0.0,
      1.0
    );
  }

  void spawn()
  {
    Entity::spawn();

    raylib::Vector2 inputVector(
      (float) (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
      (float) (IsKeyDown(KEY_S) - IsKeyDown(KEY_W))
    );

    if (!(inputVector == raylib::Vector2(0, 0)))
    {
      raylib::Vector2 normalizedInputVector(inputVector.Normalize());
      velocity = velocity.MoveTowards(normalizedInputVector * 40, 1.0);
    }
    else
    {
      velocity = velocity.MoveTowards(raylib::Vector2(0, 0), 2.0);
    }

    body.SetPosition(velocity + raylib::Vector2(body.GetPosition()));
    camera.target = body.GetPosition();
    raylib::Vector2 actualMouseOffset = -mouseInitialOffset + body.GetPosition();
    SetMouseOffset(actualMouseOffset.x, actualMouseOffset.y);

    DrawText(("player position: " + std::to_string(body.x) + " "
      + std::to_string(body.y)).c_str(), 0, 0, 10, GOLD);

    DrawText(("mouse position: " + std::to_string((raylib::Vector2(GetMousePosition())).x) + " "
      + std::to_string((raylib::Vector2(GetMousePosition())).y)).c_str(), 0, 20, 10, BLUE);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {

      raylib::Vector2 normalizedDirection((raylib::Vector2(GetMousePosition()) - body.GetPosition()).Normalize());

      Projectile newProjectile(
        GOLD,
        raylib::Rectangle(body.x, body.y, 10, 5),
        {&hostileCollisionGroup},
        10,
        normalizedDirection
      );

      projectileList.push_back(newProjectile);
    }
  }
};

int main()
{
  InitWindow(screenWidth, screenHeight, "raylib game - Henry Liu");

  Player player(
    "player", BLUE,
    raylib::Rectangle(200, 100, 100, 150),
    {&hostileCollisionGroup, &environmentCollisionGroup}
  );

  Entity enemy(
    "enemy", RED,
    raylib::Rectangle(200, 300, 100, 150),
    {&hostileCollisionGroup},
    {&playerCollisionGroup}
  );

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    BeginDrawing();

    ClearBackground(RAYWHITE);

    player.spawn();
    enemy.spawn();

    for (auto &projectile : projectileList)
    {
      projectile.spawn();
    }

    EndDrawing();
  }

  return 0;
}
