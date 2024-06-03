#pragma once

#ifndef PLAYERUPDATESYSTEM_H
#define PLAYERUPDATESYSTEM_H
#include "BaseHeader.h"

struct IrrlichtComponent;
struct PlayerComponent;
struct BulletRigidBodyComponent;
struct SensorComponent;

//Calls the update on the player components in the scene. Does things like move the HUD around and rotates the camera.
//It also moves the audio "listener" around to be where the camera currently is.
void playerUpdateSystem(flecs::entity e, IrrlichtComponent& irrc, PlayerComponent& plyc, BulletRigidBodyComponent& rbcs);
#endif
