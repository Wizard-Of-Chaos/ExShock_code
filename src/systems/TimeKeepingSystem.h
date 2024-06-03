#pragma once
#ifndef SPAWNINVULNERABLESYSTEM_H
#define SPAWNINVULNERABLESYSTEM_H
#include "BaseHeader.h"
struct SpawnInvulnerable;
struct BulletRigidBodyComponent;
void timeKeepingSystem(flecs::iter it, BulletRigidBodyComponent* rbcs);
#endif