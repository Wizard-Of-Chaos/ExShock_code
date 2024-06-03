#pragma once

#ifndef SHIPCOMPONENT_H
#define SHIPCOMPONENT_H
#include "BaseHeader.h"

//The max amount of engines you can stick on a ship, used for particle effects.
const u32 MAX_ENGINES = 4;
const s32 CHAOS_THEORY_ID = 7;
const s32 TRANSPORT_ID = 9;
const s32 ARTILLERY_ID = 10;
/*
* The ship component is the basic piece for spaceships. It includes the data id of the ship itself,
* and the positioning of various jets and particle effects for itself. The mesh for a ship is handled by the IrrlichtComponent,
* the rigid body is handled by BulletRigidBodyComponent, the guns are handled by a HardpointComponent and the thrust values are handled
* by a ThrustComponent. Really, the ship component serves primarily as a useful indicator that this is indeed a spaceship.
*/
struct ShipComponent {
	u32 shipDataId;

	vector3df upJetPos[2];
	vector3df downJetPos[2];
	vector3df leftJetPos[2];
	vector3df rightJetPos[2];
	vector3df reverseJetPos[2];

	u32 engineCount = 1;
	vector3df engineJetPos[MAX_ENGINES];
};

#endif