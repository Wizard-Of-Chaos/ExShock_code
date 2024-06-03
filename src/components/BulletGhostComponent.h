#pragma once
#ifndef BULLETGHOSTCOMPONENT_H
#define BULLETGHOSTCOMPONENT_H
#include "BaseHeader.h"

/*
* Bullet ghost components are for things you can shoot at, but can't collide with.
* An example is a gas cloud; the player can fly through it but can still target it to blow it up.
* The shape is assumed to be a sphere shape (might change depending on what else we add).
* 
* TODO: set it up so that projectiles use ghost components? having projectiles apply actual velocity to a target is funny, but not intended.
*/
struct BulletGhostComponent
{
	btGhostObject* ghost=nullptr;
	btSphereShape* shape=nullptr;
};

#endif 