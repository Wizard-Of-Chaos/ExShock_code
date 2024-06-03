#pragma once
#ifndef AIUPDATESYSTEM_H
#define AIUPDATESYSTEM_H
#include "BaseHeader.h"

struct AIComponent;
struct IrrlichtComponent;
struct BulletRigidBodyComponent;
struct ThrustComponent;
struct HardpointComponent;
struct SensorComponent;
struct HealthComponent;

//Updates all AI components in the scene (allied, friendly, and neutral ships).
void AIUpdateSystem(flecs::iter it, 
	AIComponent* aic, IrrlichtComponent* irrc, BulletRigidBodyComponent* rbcs, ThrustComponent* thrc, HardpointComponent* hardsc, SensorComponent* sensc, HealthComponent* hpc);

#endif 