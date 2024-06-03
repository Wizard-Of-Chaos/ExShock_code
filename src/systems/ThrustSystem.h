#pragma once
#ifndef THRUSTSYSTEM_H
#define THRUSTSYSTEM_H
#include "BaseHeader.h"

struct ThrustComponent;
struct HealthComponent;
struct BulletRigidBodyComponent;
struct IrrlichtComponent;

//Handles thrusts from any entity that includes a thrust component and adjusts their rigid body accordingly.
void thrustSystem(flecs::iter it, ThrustComponent* thrc, HealthComponent* hpc, BulletRigidBodyComponent* rbcs, IrrlichtComponent* irrc);

#endif 