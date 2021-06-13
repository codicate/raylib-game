﻿#include "raylib-cpp.hpp"
#include<iostream>
#include <utility>
#include <cmath>
#include <vector>

//angle = atan2(y, x);
//Vector2(cos(angle), sin(angle))
//float radians_to_angles(float radians) {
//  return (radians * 180.0f/M_PI);
//}

//struct Circle
//{
//  raylib::Vector2 center;
//  float radius;
//
//  void Draw(raylib::Color c)
//  {
//    center.DrawCircle(radius, c);
//  }
//
//  bool CheckCollsion(Circle circle)
//  {
//    return (circle.center - center).Length() <= (circle.radius + radius);
//  }
//};
//
//enum class ShapeType
//{
//  Rectangle,
//  Circle
//};
//
//struct Shape
//{
//  ShapeType e;
//
//  union
//  {
//    raylib::Rectangle rect;
//    Circle circle;
//  };
//
//  void draw(raylib::Color c)
//  {
//    switch (e)
//    {
//      case ShapeType::Rectangle:
//      {
//        rect.Draw(c);
//      }
//        break;
//
//      case ShapeType::Circle:
//      {
//        circle.Draw(c);
//      }
//        break;
//    }
//  }
//
//  bool CheckCollision(Shape shape)
//  {
//    switch (shape.e)
//    {
//      case ShapeType::Rectangle:
//        switch (e)
//        {
//          case ShapeType::Rectangle:
//          {
//            return rect.CheckCollision(shape.rect);
//          }
//
//          case ShapeType::Circle:
//          {
//            return rect.CheckCollision(shape.circle.center, shape.circle.radius);
//          }
//        }
//        break;
//
//      case ShapeType::Circle:
//        switch (e)
//        {
//          case ShapeType::Rectangle:
//          {
//            return shape.rect.CheckCollision(circle.center, circle.radius);
//          }
//
//          case ShapeType::Circle:
//          {
//            return circle.CheckCollsion(circle);
//          }
//        }
//        break;
//    }
//  }
//};

class Entity;
class Projectile;
class Player;
using CollisionGroup = std::vector<std::vector<Entity *> *>;

std::vector<Entity *> playerCollisionGroup;
std::vector<Entity *> passiveCollisionGroup;
std::vector<Entity *> hostileCollisionGroup;
std::vector<Entity *> environmentCollisionGroup;
std::vector<Entity *> projectileCollisionGroup;

std::vector<Projectile> projectileList;

class Entity
{
public:
  std::string name;
  raylib::Color color;
  raylib::Rectangle body;
  CollisionGroup scanningCollisionGroups;

  Entity(
    std::string name,
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroup belongingCollisionGroups,
    CollisionGroup scanningCollisionGroups
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
    body.Draw(color);

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
  int speed;
  raylib::Vector2 direction;

  Projectile(
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroup scanningCollisionGroups,
    int speed,
    raylib::Vector2 direction
  ) :
    Entity("bullet", color, shape, {&projectileCollisionGroup}, scanningCollisionGroups),
    speed(speed),
    direction(direction)
  {

  }

  void spawn()
  {
    Entity::spawn();
    raylib::Vector2 normalizedDirection(direction.Normalize());
    body.SetPosition(normalizedDirection * speed + body.GetPosition());
  }
};

class Player : public Entity
{
public:
  Player(
    std::string name,
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroup scanningCollisionGroups
  ) : Entity(name, color, shape, {&playerCollisionGroup}, scanningCollisionGroups)
  {
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
      DrawText(("normalizedInputVector: " + std::to_string(normalizedInputVector.x) + " "
        + std::to_string(normalizedInputVector.y)).c_str(), 0, 0, 10, GOLD);
      body.SetPosition(normalizedInputVector * 10 + body.GetPosition());
    }

    if (IsKeyDown(KEY_SPACE))
    {
      Projectile newProjectile(
        GOLD,
        raylib::Rectangle(body.x, body.y, 10, 5),
        {&hostileCollisionGroup},
        10,
        raylib::Vector2(1, 0)
      );

      projectileList.push_back(newProjectile);
    }
  }
};

int main()
{
  const int screenWidth = 800;
  const int screenHeight = 450;

  raylib::Window window(screenWidth, screenHeight, "raylib game - Henry Liu");
  SetTargetFPS(60);

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

  while (!window.ShouldClose())
  {
    BeginDrawing();

    window.ClearBackground(RAYWHITE);

    player.spawn();
    enemy.spawn();

    for (auto projectile : projectileList)
    {
      projectile.body.Draw(projectile.color);
    }

    EndDrawing();
  }

  return 0;
}
