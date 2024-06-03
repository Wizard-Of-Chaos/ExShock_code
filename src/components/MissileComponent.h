#pragma once
#ifndef MISSILECOMPONENT_H
#define MISSILECOMPONENT_H
#include "BaseHeader.h"
#include "ThrustComponent.h"

/*
* The missile info component is used alongside the WeaponInfoComponent to track data about missile weapons. Note that it's
* ALONGSIDE OF; this is just the extra data required for missiles to properly track. The two variables included are the rotational
* speed of the missile (e.g., how fast it can turn to track a target) and how fast the missile is allowed to go.
* It also includes how long it takes to lock onto a given ship.
*/
struct MissileInfoComponent
{
	f32 timeToLock; 
	ThrustComponent missThrust;
	IMesh* missileMesh;
	ITexture* missileTexture;
};
#endif 