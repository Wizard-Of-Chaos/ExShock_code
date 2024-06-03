#pragma once
#ifndef TURRETHARDPOINTCOMPONENT_H
#define TURRETHARDPOINTCOMPONENT_H
#include "BaseHeader.h"

//Maximum amount of turrets a ship can have
const u32 MAX_TURRET_HARDPOINTS = 8;

/*
* Turret hardpoint components act like regular hardpoints, but for turrets. They keep track of the current turret entities, their starting positions,
* and their starting rotations (for use with determining how far they can pitch / yaw).
*/

struct TurretTag{};

struct TurretHardpointComponent
{
	u32 turretCount = 0;
	f32 turretScale = 1.f;
	flecs::entity turrets[MAX_TURRET_HARDPOINTS];
	vector3df turretPositions[MAX_TURRET_HARDPOINTS];
	vector3df turretRotations[MAX_TURRET_HARDPOINTS];
	TurretHardpointComponent() {
		for (u32 i = 0; i < MAX_TURRET_HARDPOINTS; ++i) { 
			turrets[i] = INVALID_ENTITY; 
			turretPositions[i] = vector3df(0.f);
			turretRotations[i] = vector3df(0.f);
		}
	}
	const bool turretsDead() const {
		for (u32 i = 0; i < MAX_TURRET_HARDPOINTS; ++i) {
			if (turrets[i].is_alive()) return false;
		}
		return true;
	}
};


#endif 