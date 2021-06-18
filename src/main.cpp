#include <algorithm>
#include <vector>
#include <ctime>
#include <iostream>

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

// pointer to the physics simulation initialization that will be used for all `PhysicsBody`s
raylib::Physics *physics;
// pointer to main camera object created inside the player class that will be used to render things properly
raylib::Camera2D *camera;

template <typename EntityType>
std::vector<EntityType *> *entitySpawner(EntityType entity, int quantity, std::vector<EntityType *> *spawnedEntities = new std::vector<EntityType *>)
{
  class spawnedEntity : public EntityType
  {
  private:
    std::vector<EntityType *> *spawnedEntities;

  public:
    spawnedEntity(
      const EntityType &entity,
      std::vector<EntityType *> *spawnedEntities
    ) :
      EntityType(entity),
      spawnedEntities(spawnedEntities) {}

    void deinit() override
    {
      EntityType::deinit();
      auto position = std::find(spawnedEntities->begin(), spawnedEntities->end(), this);
      spawnedEntities->erase(position);
      delete this;
    }
  };

  for (int i = 0; i < quantity; ++i)
  {
    auto *newEnemy = new spawnedEntity(
      entity,
      spawnedEntities
    );

    newEnemy->init();
    spawnedEntities->push_back(newEnemy);
  }

  return spawnedEntities;
};

class Entity
{
public:
  bool isAlive = true;
  std::string name;
  raylib::Color color;
  raylib::Vector2 size;
  raylib::Rectangle shape;
  raylib::Vector2 initialPosition;
  // `CollisionGroups` that the entity belonged to, and will be discovered by object scanning for any of these `CollisionGroups`
  CollisionGroups belongingCollisionGroups;
  // `CollisionGroups` that the entity will scan for, and will react if collision occurred
  CollisionGroups scanningCollisionGroups;

  Entity(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 initialPosition,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups
  ) :
    name(name),
    color(color),
    size(size),
    initialPosition(initialPosition),
    belongingCollisionGroups(belongingCollisionGroups),
    scanningCollisionGroups(scanningCollisionGroups) {};

  virtual void init()
  {
    shape = raylib::Rectangle(initialPosition, size);

    // add self to every `belongingCollisionGroup`
    for (auto belongingCollisionGroup : belongingCollisionGroups)
    {
      belongingCollisionGroup->push_back(this);
    }
  }

  virtual CollisionGroup getCollidedBodies()
  {
    CollisionGroup collidedBodies;

    // scan for and react to every object in every `belongingCollisionGroup`
    for (auto scanningCollisionGroup: scanningCollisionGroups)
      for (auto collidedBody: *scanningCollisionGroup)
        if (shape.CheckCollision(collidedBody->shape))
          collidedBodies.push_back(collidedBody);

    return collidedBodies;
  }

  virtual void update() {}

  virtual void render()
  {
    shape.Draw(color);
  }

  virtual void spawn()
  {
    if (isAlive)
    {
      // render the entity on screen, reactive to camera position and will move in and out of viewport
      BeginMode2D(*camera);
      render();
      EndMode2D();

      // everything rendered below will stay on screen at fixed position
      update();
    }
  }

  virtual void deinit()
  {
    // loop every `belongingCollisionGroup` and remove self, saving unnecessary scans
    for (auto belongingCollisionGroup : belongingCollisionGroups)
    {
      auto position = std::find(belongingCollisionGroup->begin(), belongingCollisionGroup->end(), this);
      belongingCollisionGroup->erase(position);
    }

    isAlive = false;
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
    raylib::Vector2 initialPosition,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups,
    raylib::Vector2 initialVelocity = raylib::Vector2(0, 0)
  ) :
    Entity(
      name,
      color,
      size,
      initialPosition,
      belongingCollisionGroups,
      scanningCollisionGroups
    ),
    velocity(initialVelocity) {}

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

  void update() override
  {
    Entity::update();
    shape.SetPosition(velocity + shape.GetPosition());
  }
};

class CollisionBody : public DynamicEntity
{
public:
  float acceleration;
  float deceleration;
  float maxSpeed;
  bool isDynamic;
  PhysicsBody body;

  CollisionBody(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 initialPosition,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups,
    float acceleration,
    float deceleration,
    float maxSpeed,
    bool isDynamic,
    raylib::Vector2 velocity = raylib::Vector2(0, 0)
  ) :
    DynamicEntity(
      name,
      color,
      size,
      initialPosition,
      belongingCollisionGroups,
      scanningCollisionGroups,
      velocity
    ),
    acceleration(acceleration),
    deceleration(deceleration),
    maxSpeed(maxSpeed),
    isDynamic(isDynamic) {}

  void init() override
  {
    DynamicEntity::init();
    body = physics->CreateBodyRectangle(initialPosition, size.x, size.y, 100);
    body->enabled = isDynamic;
  };

  void update() override
  {
    Entity::update();
    body->position = velocity + body->position;
    shape.SetPosition(body->position);
  }

  void deinit() override
  {
    DynamicEntity::deinit();
    physics->DestroyBody(physics->GetBody(body->id));
  }
};

class Subject : public CollisionBody
{
public:
  float health;
  float damage;

  Subject(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 initialPosition,
    CollisionGroups belongingCollisionGroups,
    CollisionGroups scanningCollisionGroups,
    float acceleration,
    float deceleration,
    float maxSpeed,
    float health,
    float damage
  ) :
    CollisionBody(
      name,
      color,
      size,
      initialPosition,
      belongingCollisionGroups,
      scanningCollisionGroups,
      acceleration,
      deceleration,
      maxSpeed,
      true
    ),
    health(health),
    damage(damage) {}

  void takeDamage(float incomingDamage)
  {
    health -= incomingDamage;
  }

  void update() override
  {
    CollisionBody::update();

    if (health <= 0)
    {
      deinit();
    }
  }
};

class Projectile : public DynamicEntity
{
private:
  std::clock_t spawnStartTime;
  float lifetime;

public:
  float damage;

  Projectile(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 initialPosition,
    CollisionGroups scanningCollisionGroups,
    raylib::Vector2 velocity,
    float damage,
    float lifetime
  ) :
    DynamicEntity(
      name,
      color,
      size,
      initialPosition,
      {&projectileCollisionGroup},
      scanningCollisionGroups,
      velocity
    ),
    damage(damage),
    lifetime(lifetime) {}

  void init() override
  {
    DynamicEntity::init();

    // record the time when the projectile is constructed to be used for despawning
    spawnStartTime = clock();
  }

  void update() override
  {
    DynamicEntity::update();

    std::vector<Entity *> bodiesHit = Entity::getCollidedBodies();
    if (!bodiesHit.empty())
    {
      for (auto body : bodiesHit)
        dynamic_cast<Subject *>(body)->takeDamage(damage);

      deinit();
    }

    // Find the time passed since the projectile was first constructed
    float timePassed = (clock() - spawnStartTime) / (float) CLOCKS_PER_SEC;
    if (timePassed >= lifetime)
      deinit();
  }
};

class Player : public Subject
{
private:
  std::vector<Projectile *> projectileList;
public:
  Player(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 initialPosition,
    CollisionGroups scanningCollisionGroups,
    float acceleration,
    float deceleration,
    float maxSpeed,
    int health,
    int damage
  ) :
    Subject(
      name,
      color,
      size,
      initialPosition,
      {&playerCollisionGroup},
      scanningCollisionGroups,
      acceleration,
      deceleration,
      maxSpeed,
      health,
      damage
    ) {}

  void init() override
  {
    Subject::init();

    // The initial player position offset from the viewport origin, which will be used to calculate global mouse position, or the mouse position relational to the view port origin
    raylib::Vector2 playerOffset = screenDimension / 2.0 - raylib::Vector2(size) / 2.0;

    camera = new raylib::Camera2D(
      playerOffset,
      raylib::Vector2(0, 0),
      0.0,
      1.0
    );

    SetMouseOffset(-playerOffset.x, -playerOffset.y);
  }

  void update() override
  {
    const float projectileSpeed = 40.0;

    Subject::update();

    raylib::Vector2 inputVector(
      (float) (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)),
      (float) (IsKeyDown(KEY_S) - IsKeyDown(KEY_W))
    );

    // if any key is pressed
    if (!(inputVector == raylib::Vector2(0, 0)))
      Subject::accelerate(inputVector, acceleration, maxSpeed);
    else
      Subject::decelerate(deceleration);

    // camera follows player
    camera->target = body->position;

    DrawText(("position: " + std::to_string(body->position.x) + " " + std::to_string(body->position.y)).c_str(), 0, 0, 10, GOLD);
    DrawText(("velocity: " + std::to_string(velocity.x) + " " + std::to_string(velocity.y)).c_str(), 0, 10, 10, GOLD);
    DrawText(("health: " + std::to_string(health)).c_str(), 0, 20, 10, RED);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
      // find the direction in form of vector of the direction of mouse click relational to the player
      raylib::Vector2 normalizedDirection((raylib::Vector2(GetMousePosition())).Normalize());

      // create new projectile near the player
      entitySpawner<Projectile>(
        {
          "player bullet",
          GOLD,
          raylib::Vector2(10, 5),
          body->position,
          {&hostileCollisionGroup},
          normalizedDirection * projectileSpeed,
          damage,
          1
        },
        1,
        &projectileList
      );
    }

    for (auto projectile : projectileList)
      projectile->spawn();
  }
};

class Enemy : public Subject
{
private:
  int isInCooldown = false;
  std::clock_t lastAttackTime = clock();

public:
  Enemy(
    std::string name,
    raylib::Color color,
    raylib::Vector2 size,
    raylib::Vector2 initialPosition,
    CollisionGroups scanningCollisionGroups,
    float acceleration,
    float deceleration,
    float maxSpeed,
    int health,
    int damage
  ) :
    Subject(
      name,
      color,
      size,
      initialPosition,
      {&hostileCollisionGroup},
      scanningCollisionGroups,
      acceleration,
      deceleration,
      maxSpeed,
      health,
      damage
    ) {}

  void update() override
  {
    Subject::update();

    float timePassed = (clock() - lastAttackTime) / (float) CLOCKS_PER_SEC;
    if (timePassed >= 1)
      isInCooldown = false;

    if (!isInCooldown)
    {
      std::vector<Entity *> bodiesHit = Entity::getCollidedBodies();
      if (!bodiesHit.empty())
      {
        for (auto body : bodiesHit)
          dynamic_cast<Subject *>(body)->takeDamage(damage);

        isInCooldown = true;
        lastAttackTime = clock();
      }
    }

    if (!playerCollisionGroup.empty())
    {
      const auto player = dynamic_cast<Player *>(playerCollisionGroup[0]);
      const auto distanceToPlayer = raylib::Vector2(player->body->position).Distance(body->position);

      if (distanceToPlayer <= 800)
      {
        const auto angleToPlayer = (raylib::Vector2(player->body->position) - body->position).Normalize();
        Subject::accelerate(angleToPlayer, acceleration, maxSpeed);
      }
      else
      {
        Subject::decelerate(deceleration);
      }
    }
  }
};

void DrawCenteredText(const char *text, int fontSize, raylib::Color color)
{
  int centerOffset = MeasureText(text, fontSize);
  DrawText(
    text,
    screenDimension.x / 2 + centerOffset,
    screenDimension.y / 2 + centerOffset / 2.0 + fontSize,
    fontSize,
    color
  );
}

int main()
{
  InitWindow(screenDimension.x, screenDimension.y, "raylib game - Henry Liu");
  *physics = raylib::Physics(0);

  Player player(
    "player",
    BLUE,
    raylib::Vector2(100, 150),
    raylib::Vector2(0, 0),
    {&hostileCollisionGroup},
    0.7,
    1.5,
    20.0,
    10,
    2
  );
  player.init();

  auto *enemies = entitySpawner<Enemy>(
    {
      "enemy",
      RED,
      raylib::Vector2(100, 150),
      raylib::Vector2(300, 300),
      {&playerCollisionGroup},
      0.2,
      1.0,
      10.0,
      10,
      2,
    },
    1
  );

  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    physics->UpdateStep();

    BeginDrawing();

    ClearBackground(RAYWHITE);
    DrawFPS(screenDimension.x * 2 - 100, 0);

    player.spawn();

    if (!player.isAlive)
      DrawCenteredText("YOU DIED!", 40, RED);

    for (auto enemy : *enemies)
      enemy->spawn();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
