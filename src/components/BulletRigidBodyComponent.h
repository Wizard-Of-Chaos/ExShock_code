#pragma once
#ifndef BULLETRIGIDBODYCOMPONENT_H
#define BULLETRIGIDBODYCOMPONENT_H
#include "BaseHeader.h"

struct SpawnInvulnerable {};

/*
* The rigid body component is a wrapper around bullet rigid bodies so that they can be used as part of the ECS.
* It also includes a pointer to the shape of the rigid body, since that needs to be stored and managed as well.
*/
struct BulletRigidBodyComponent
{
	btRigidBody* rigidBody = nullptr;
	btCollisionShape* shape = nullptr;
	f32 timeAlive = 0.f;
	BulletRigidBodyComponent() : rigidBody(nullptr), shape(nullptr) {}
};

#endif
