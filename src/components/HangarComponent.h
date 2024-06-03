#pragma once
#ifndef HANGARCOMPONENT_H
#define HANGARCOMPONENT_H
#include "BaseHeader.h"
#include "ShipInstance.h"

//Max amount of ships the carrier will spawn (i.e. different design patterns).
const u32 MAX_HANGAR_SHIPTYPES = 4;
/*
* The hangar component effectively acts as an extension of the ship component and tracks data about a hangar type ship.
* This includes the spawn rate at which it creates ships, the amount of different types of ship it can spawn, the amount of reserve ships it has
* total, and any turrets that it might have. It also keeps track of the constraints for its turrets and its own personal scaling.
*/
struct HangarComponent
{
	f32 launchRate = 0.f;
	f32 curLaunchTimer = 0.f;
	u32 reserveShips = 0U;

	u32 shipTypeCount = 0U;
	dataId spawnShipArchetypes[MAX_HANGAR_SHIPTYPES];

	HangarComponent() {
		for (u32 i = 0; i < MAX_HANGAR_SHIPTYPES; ++i) {
			spawnShipArchetypes[i] = INVALID_DATA_ID;
		}
	}
};

#endif 