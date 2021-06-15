﻿#include <algorithm>
#include <vector>
#include <ctime>

#include "raylib-cpp.hpp"

#define PHYSAC_IMPLEMENTATION
#define PHYSAC_NO_THREADS
#include "Physics.hpp"

raylib::Vector2 screenDimension = {1280, 720};

class Entity;
class DynamicEntity;
class CollisionBody;
class Subject;
class Projectile;
class Player;

// alias for a list of objects in a collision group to be looped through to scan for collision
using CollisionGroup = std::vector<Entity *>;
// alias for a list of `CollisionGroup`, as any object can belong to and scan multiple `CollisionGroup`s
using CollisionGroups = std::vector<CollisionGroup *>;

CollisionGroup playerCollisionGroup;
CollisionGroup passiveCollisionGroup;
CollisionGroup hostileCollisionGroup;
CollisionGroup environmentCollisionGroup;
CollisionGroup projectileCollisionGroup;

// projectiles are created in the heap dynamically, this is used to store their pointers and loop through to spawn them
std::vector<Projectile *> projectileList;

// pointer to the physics simulation initialization that will be used for all `PhysicsBody`s
raylib::Physics *physics;
// pointer to main camera object created inside the player class that will be used to render things properly
raylib::Camera2D *camera;

class Entity
{
public:
  std::string name;
  raylib::Color color;
  raylib::Vector2 size;
  raylib::Rectangle shape;
  // `CollisionGroups` that the entity belonged to, and will be discovered by object scanning for any of these `CollisionGroups`
  CollisionGroups belongingCollisionGroups;
  // `CollisionGroups` that the entity will scan for, and will react if collision occurred
  CollisionGroups scanningCollisionGroups;

  Entity(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 position,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups
  ) :
    name(name),
    color(color),
    size(size),
    belongingCollisionGroups(belongingCollisionGroups),
    scanningCollisionGroups(scanningCollisionGroups)
  {
    shape = raylib::Rectangle(position, size);

    // add self to every `belongingCollisionGroup`
    for (auto belongingCollisionGroup : belongingCollisionGroups)
    {
      belongingCollisionGroup->push_back(this);
    }
  };

  std::vector<Entity *> getCollidedBodies()
  {
    std::vector<Entity *> collidedBodies = {};

    // scan for and react to every object in every `belongingCollisionGroup`
    for (auto scanningCollisionGroup: scanningCollisionGroups)
      for (auto collidedBody: *scanningCollisionGroup)
        if (shape.CheckCollision(collidedBody->shape))
          collidedBodies.push_back(collidedBody);

    return collidedBodies;
  }

  void spawn()
  {
    // render the entity on screen, reactive to camera position and will move in and out of viewport
    BeginMode2D(*camera);
    shape.Draw(color);
    EndMode2D();
    // everything rendered below will stay on screen at fixed position
  }

  void despawn()
  {
    // loop every `belongingCollisionGroup` and remove self, saving unnecessary scans
    for (auto belongingCollisionGroup : belongingCollisionGroups)
    {
      auto position = std::find(belongingCollisionGroup->begin(), belongingCollisionGroup->end(), this);
      belongingCollisionGroup->erase(position);
    }
  }
};

class DynamicEntity : public Entity
{
public:
  // since `DynamicEntity` will move naturally with acceleration and deceleration, its velocity needs to be stored
  raylib::Vector2 velocity;

  DynamicEntity(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 position,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups,
    raylib::Vector2 initialVelocity = raylib::Vector2(0, 0)
  ) :
    Entity(
      name,
      color,
      size,
      position,
      belongingCollisionGroups,
      scanningCollisionGroups
    ),
    velocity(initialVelocity)
  {
  }

  void accelerate(raylib::Vector2 direction, float acceleration, float maxSpeed)
  {
    // normalize the vector so that the object will not move faster diagonally
    raylib::Vector2 normalizedInputVector(direction.Normalize());
    // gradually gaining velocity towards `maxSpeed` by `acceleration`
    velocity = velocity.MoveTowards(normalizedInputVector * maxSpeed, acceleration);
  }

  void decelerate(float deceleration)
  {
    // gradually losing velocity towards stopping point by `deceleration`
    velocity = velocity.MoveTowards(raylib::Vector2(0, 0), deceleration);
  }

  void spawn()
  {
    Entity::spawn();
    shape.SetPosition(velocity + shape.GetPosition());
  }
};

class CollisionBody : public DynamicEntity
{
public:
  PhysicsBody body;

  CollisionBody(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 position,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups,
    bool isDynamic,
    raylib::Vector2 velocity = raylib::Vector2(0, 0)
  ) :
    DynamicEntity(
      name,
      color,
      size,
      position,
      belongingCollisionGroups,
      scanningCollisionGroups,
      velocity
    )
  {
    body = physics->CreateBodyRectangle(position, size.x, size.y, 100);
    body->enabled = isDynamic;
  }

  void spawn()
  {
    Entity::spawn();
    body->position = velocity + body->position;
    shape.SetPosition(body->position);
  }

  void despawn()
  {
    DynamicEntity::despawn();
    physics->DestroyBody(physics->GetBody(body->id));
  }
};

class Subject : public CollisionBody
{
public:
  int health;
  int damage;

  Subject(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 position,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups,
    int health,
    int damage
  ) :
    CollisionBody(
      name,
      color,
      size,
      position,
      belongingCollisionGroups,
      scanningCollisionGroups,
      true
    ),
    health(health),
    damage(damage)
  {
  }

  void takeDamage(Subject damageSubject)
  {
    health -= damageSubject.damage;
  }
};

class Projectile : public DynamicEntity
{
private:
  std::clock_t spawnStartTime;

public:

  double lifetime;

  Projectile(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 position,
    CollisionGroups scanningCollisionGroups,
    raylib::Vector2 direction,
    float speed,
    double lifetime
  ) :
    DynamicEntity(
      name,
      color,
      size,
      position,
      {&projectileCollisionGroup},
      scanningCollisionGroups,
      direction * speed
    ),
    lifetime(lifetime)
  {
    // record the time when the projectile is constructed to be used for despawning
    spawnStartTime = clock();
    // since projectile will be created in the heap, it needs to be stored outside of the place where it will be constructed
    projectileList.push_back(this);
  }

  void spawn()
  {
    DynamicEntity::spawn();

    std::vector<Entity *> collidedBodies = getCollidedBodies();
    for (auto collidedBody : collidedBodies)
    {
      DrawText(("collided with " + collidedBody->name + " !").c_str(), 0, 10, 10, RED);
    }

    // Find the time passed since the projectile was first constructed
    double timePassed = (clock() - spawnStartTime) / (double) CLOCKS_PER_SEC;
    DrawText(("num of projectile: " + std::to_string(projectileCollisionGroup.size())).c_str(), 0, 50, 10, PINK);

    if (timePassed >= lifetime)
    {
      DynamicEntity::despawn();

      // loop through and remove self from `projectileList`, and delete its memory allocation, and will no longer be rendered
      auto position = std::find(projectileList.begin(), projectileList.end(), this);
      projectileList.erase(position);
      delete (this);
    }
  }
};

class Player : public Subject
{
public:
  // The initial player position offset from the viewport origin, which will be used to calculate global mouse position, or the mouse position relational to the view port origin
  raylib::Vector2 mouseInitialOffset;

  Player(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 position,
    CollisionGroups scanningCollisionGroups,
    int health,
    int damage
  ) :
    Subject(
      name,
      color,
      size,
      position,
      {&playerCollisionGroup},
      scanningCollisionGroups,
      health,
      damage
    ),
    mouseInitialOffset(body->position)
  {
    camera = new raylib::Camera2D(
      mouseInitialOffset,
      screenDimension / 2.0,
      0.0,
      1.0
    );
  }

  void spawn()
  {
    const float projectileSpeed = 40.0;

    Subject::spawn();

    raylib::Vector2 inputVector(
      (float) (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
      (float) (IsKeyDown(KEY_S) - IsKeyDown(KEY_W))
    );

    // if any key is pressed
    if (!(inputVector == raylib::Vector2(0, 0)))
      Subject::accelerate(inputVector, 0.7, 20.0);
    else
      Subject::decelerate(1.5);

    // camera follows player
    camera->target = body->position;

    // find and set the actual global mouse position offset relational to viewport origin by subtracting player offset from viewport origin
    raylib::Vector2 actualMouseOffset = -mouseInitialOffset + body->position;
    SetMouseOffset(actualMouseOffset.x, actualMouseOffset.y);

    DrawText(("velocity: " + std::to_string(velocity.x) + " "
      + std::to_string(velocity.y)).c_str(), 0, 0, 10, GOLD);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
      // find the direction in form of vector of the direction of mouse click relational to the player
      raylib::Vector2 normalizedDirection((raylib::Vector2(GetMousePosition()) - body->position).Normalize());

      // create new projectile near the player
      new Projectile(
        "player bullet",
        GOLD,
        raylib::Vector2(10, 5),
        body->position,
        {&hostileCollisionGroup},
        normalizedDirection,
        projectileSpeed,
        1
      );
    }
  }

  void despawn()
  {
    Subject::despawn();
    delete (camera);
  }
};

int main()
{
  InitWindow(screenDimension.x, screenDimension.y, "raylib game - Henry Liu");
  *physics = raylib::Physics(0);

  Player player(
    "player",
    BLUE,
    raylib::Vector2(100, 150),
    raylib::Vector2(200, 100),
    {&hostileCollisionGroup, &environmentCollisionGroup},
    10,
    2
  );

  Subject enemy(
    "enemy",
    RED,
    raylib::Vector2(100, 150),
    raylib::Vector2(300, 300),
    {&hostileCollisionGroup},
    {&playerCollisionGroup, &environmentCollisionGroup},
    10,
    2
  );

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    physics->UpdateStep();

    BeginDrawing();

    ClearBackground(RAYWHITE);
    DrawFPS(screenDimension.x * 2 - 100, 0);

    player.spawn();
    enemy.spawn();

    for (auto &projectile : projectileList)
    {
      projectile->spawn();
    }

    EndDrawing();
  }

//  physics->Close();
  CloseWindow();

  return 0;
}
