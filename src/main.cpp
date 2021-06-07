#include "raylib-cpp.hpp"

#include <iostream>
using namespace std;

int main()
{
  const int screenWidth = 800;
  const int screenHeight = 450;

  raylib::Window window(screenWidth, screenHeight, "raylib game - Henry Liu");
  SetTargetFPS(60);

  raylib::Rectangle player(0, 0, 20, 50);

//  cout << (float)(IsKeyDown(KEY_D)) << endl;


//  func move(delta):
//  var inputVector = Vector2(
//    Input.get_action_strength("right") - Input.get_action_strength("left"),
//    Input.get_action_strength("down") - Input.get_action_strength("up")
//  ).normalized()
//
//  if inputVector != Vector2.ZERO:
//  body.play("run")
//  hand.play("run")
//  velocity = velocity.move_toward(maxSpeed * inputVector, acceleration * delta)
//
//  else:
//  body.play("idle")
//  hand.play("idle")
//  velocity = velocity.move_toward(Vector2.ZERO, decceleration * delta)
//
//  if dodging:
//    velocity = velocity.move_toward(maxDodgeSpeed * inputVector, acceleration * dodgeModifier * delta)
//
//  velocity = move_and_slide(velocity)




  while (!window.ShouldClose())
  {
    BeginDrawing();

    window.ClearBackground(RAYWHITE);

    player.Draw(GOLD);

    raylib::Vector2 positionVector((float) (IsKeyDown(KEY_D) - IsKeyDown(KEY_A)), (float) (IsKeyDown(KEY_S) - IsKeyDown(KEY_W)));
    DrawText(positionVector.GetX(), 523, gr, )
    positionVector.Normalize();
//    player.SetPosition();


    if (IsKeyDown(KEY_A)) player.x -= 10;
    if (IsKeyDown(KEY_D)) player.x += 10;
    if (IsKeyDown(KEY_W)) player.y -= 10;
    if (IsKeyDown(KEY_S)) player.y += 10;

    EndDrawing();
  }

  return 0;
}
