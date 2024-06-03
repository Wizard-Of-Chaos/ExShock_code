#pragma once

#ifndef SHIPCONTROLSYSTEM_H
#define SHIPCONTROLSYSTEM_H
#include "BaseHeader.h"

struct InputComponent;
struct HardpointComponent;
struct ShipComponent;
struct ThrustComponent;
struct PlayerComponent;
struct BulletRigidBodyComponent;
struct IrrlichtComponent;
struct SensorComponent;
/*
* The way that the actual buttons on the keyboard talk to the player entity. This function updates the ship component
* to pitch, yaw, and roll according to keyboard inputs.
* 
* TODO: Abstract keyboard inputs to be mapped to player-set keybinds.
*/
void shipControlSystem(flecs::iter it,
	InputComponent* inc, HardpointComponent* hardsc, ShipComponent* shpc, ThrustComponent* thrc, PlayerComponent* plyc, 
	BulletRigidBodyComponent* rbcs, IrrlichtComponent* irrc, SensorComponent* snsc);

#endif