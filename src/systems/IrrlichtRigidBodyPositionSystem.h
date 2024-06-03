#pragma once

#ifndef IRRLICHTRIGIDBODYPOSITIONSYSTEM_H
#define IRRLICHTRIGIDBODYPOSITIONSYSTEM_H
#include "BaseHeader.h"

struct BulletRigidBodyComponent;
struct IrrlichtComponent;
//This function allows Irrlicht and Bullet to talk to each other. It checks all entities with Bullet components and Irrlicht components
//and makes it so that the Bullet calculations (for rotation, movement, and such) affect the Irrlicht component and makes the thing actually
//move.
void irrlichtRigidBodyPositionSystem(flecs::entity e, BulletRigidBodyComponent& rbc, IrrlichtComponent& irr);

#endif
