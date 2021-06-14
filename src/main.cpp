#include "raylib-cpp.hpp"
#include <algorithm>
#include <vector>
#include <ctime>

const int screenWidth = 1280;
const int screenHeight = 720;

class Entity;
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

// pointer to main camera object created inside the player class that will be used to render things properly
raylib::Camera2D *camera;

class Entity
{
public:
  std::string name;
  raylib::Color color;
  raylib::Rectangle body;
  // `CollisionGroups` that the entity belonged to, and will be discovered by object scanning for any of these `CollisionGroups`
  CollisionGroups belongingCollisionGroups;
  // `CollisionGroups` that the entity will scan for, and will react if collision occurred
  CollisionGroups scanningCollisionGroups;

  Entity(
    std::string name,
    raylib::Color color,
    raylib::Rectangle body,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups
  ) :
    name(name),
    color(color),
    body(body),
    belongingCollisionGroups(belongingCollisionGroups),
    scanningCollisionGroups(scanningCollisionGroups)
  {
    // add self to every `belongingCollisionGroup`
    for (auto belongingCollisionGroup : belongingCollisionGroups)
    {
      belongingCollisionGroup->push_back(this);
    }
  };

  void spawn()
  {
    // render the entity on screen, reactive to camera position and will move in and out of viewport
    BeginMode2D(*camera);
    body.Draw(color);
    EndMode2D();
    // everything rendered below will stay on screen at fixed position

    // scan for and react to every object in every `belongingCollisionGroup`
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

class Projectile : public Entity
{
private:
  std::clock_t spawnStartTime;

public:
  float speed;
  raylib::Vector2 direction;
  double lifetime;

  Projectile(
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroups scanningCollisionGroups,
    float speed,
    raylib::Vector2 direction,
    double lifetime
  ) :
    Entity(
      "bullet",
      color,
      shape,
      {&projectileCollisionGroup},
      scanningCollisionGroups
    ),
    speed(speed),
    direction(direction),
    lifetime(lifetime)
  {
    // record the time when the projectile is constructed to be used for despawning
    spawnStartTime = clock();
    // since projectile will be created in the heap, it needs to be stored outside of the place where it will be constructed
    projectileList.push_back(this);
  }

  void spawn()
  {
    Entity::spawn();

    body.SetPosition(direction * speed + body.GetPosition());

    // Find the time passed since the projectile was first constructed
    double timePassed = (clock() - spawnStartTime) / (double) CLOCKS_PER_SEC;
    DrawText(("num of projectile: " + std::to_string(projectileCollisionGroup.size())).c_str(), 0, 50, 10, PINK);

    if (timePassed >= lifetime)
    {
      Entity::despawn();

      // loop through and remove self from `projectileList`, and delete its memory allocation, and will no longer be rendered
      auto position = std::find(projectileList.begin(), projectileList.end(), this);
      projectileList.erase(position);
      delete (this);
    }
  }
};

class Subject : public Entity
{
public:
  // since subject will move naturally with acceleration and deceleration, its velocity needs to be stored
  raylib::Vector2 velocity = {0, 0};
  // raylib::Vector2 direction = {0, 0};
  int health;
  int damage;

  Subject(
    std::string name,
    raylib::Color color,
    raylib::Rectangle shape,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups,
    int health,
    int damage
  ) :
    Entity(
      name,
      color,
      shape,
      belongingCollisionGroups,
      scanningCollisionGroups
    ),
    health(health),
    damage(damage)
  {
  }

  void spawn()
  {
    Entity::spawn();
    body.SetPosition(velocity + body.GetPosition());
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

  void takeDamage(Subject damageSubject)
  {
    health -= damageSubject.damage;
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
    raylib::Rectangle shape,
    CollisionGroups scanningCollisionGroups,
    int health,
    int damage
  ) :
    Subject(
      name,
      color,
      shape,
      {&playerCollisionGroup},
      scanningCollisionGroups,
      health,
      damage
    ),
    mouseInitialOffset(body.GetPosition())
  {
    camera = new raylib::Camera2D(
      mouseInitialOffset,
      {screenWidth / 2.0, screenHeight / 2.0},
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
    {
      Subject::accelerate(inputVector, 0.7, 20.0);
    }
    else
    {
      Subject::decelerate(1.5);
    }

    // camera follows player
    camera->target = body.GetPosition();

    // find and set the actual global mouse position offset relational to viewport origin by subtracting player offset from viewport origin
    raylib::Vector2 actualMouseOffset = -mouseInitialOffset + body.GetPosition();
    SetMouseOffset(actualMouseOffset.x, actualMouseOffset.y);

    DrawText(("player position: " + std::to_string(body.x) + " "
      + std::to_string(body.y)).c_str(), 0, 0, 10, GOLD);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
      // find the direction in form of vector of the direction of mouse click relational to the player
      raylib::Vector2 normalizedDirection((raylib::Vector2(GetMousePosition()) - body.GetPosition()).Normalize());

      // create new projectile near the player
      new Projectile(
        GOLD,
        raylib::Rectangle(body.x, body.y, 10, 5),
        {&hostileCollisionGroup},
        projectileSpeed,
        normalizedDirection,
        1
      );
    }
  }

  void despawn()
  {
    Entity::despawn();
    delete (camera);
  }
};

int main()
{
  InitWindow(screenWidth, screenHeight, "raylib game - Henry Liu");

  Player player(
    "player",
    BLUE,
    raylib::Rectangle(200, 100, 100, 150),
    {&hostileCollisionGroup, &environmentCollisionGroup},
    10,
    2
  );

  Entity enemy(
    "enemy", RED,
    raylib::Rectangle(200, 300, 100, 150),
    {&hostileCollisionGroup},
    {&playerCollisionGroup, &environmentCollisionGroup}
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
      projectile->spawn();
    }

    EndDrawing();
  }

  return 0;
}
