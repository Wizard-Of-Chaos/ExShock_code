#pragma once
#ifndef HARDPOINTCOMPONENT_H
#define HARDPOINTCOMPONENT_H
#include "BaseHeader.h"

/*
* The hardpoint component tracks the weapons for a given entity as well as their rotations and the positions of the various weapons.
* This is typically used in conjunction with an actual ship component, but also gets used with turret components and is sometimes initialized
* as an empty component if the AI piece in question doesn't require weapons (i.e., a ship missile).
*/
struct HardpointComponent
{
	HardpointComponent() {
		for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
			hardpoints[i] = vector3df(0.f);
			hardpointRotations[i] = vector3df(0.f);
			weapons[i] = INVALID_ENTITY;
		}
	}
	u32 hardpointCount = 0;
	//This and the weapons array are initialized to the maximum number.
	vector3df hardpoints[MAX_HARDPOINTS];
	vector3df hardpointRotations[MAX_HARDPOINTS];
	flecs::entity weapons[MAX_HARDPOINTS];

	vector3df physWeaponHardpoint = vector3df(0.f);
	vector3df physHardpointRot = vector3df(0.f);
	flecs::entity physWeapon = INVALID_ENTITY;

	vector3df heavyWeaponHardpoint = vector3df(0.f);
	vector3df heavyWeaponRot = vector3df(0.f);
	flecs::entity heavyWeapon = INVALID_ENTITY;
};

#endif 