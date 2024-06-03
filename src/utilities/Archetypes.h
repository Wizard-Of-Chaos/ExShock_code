#pragma once
#ifndef ARCHETYPES_H
#define ARCHETYPES_H
#include "BaseHeader.h"
#include "HardpointComponent.h"
#include "TurretHardpointComponent.h"
#include "ShipUpgrades.h"
#include "WeaponUpgrades.h"

struct WeaponArchetype
{
	//The ID of this archetype.
	dataId archetypeId = -1;
	//The data ID of the weapon being used for the archetype.
	dataId wepId = -1;

	//Upgrade IDs for the archetype (if applicable).
	WEAPONUPGRADE_TYPE upgradeIds[MAX_WEP_UPGRADES];
	//Values for the archetype (if applicable).
	f32 wepUpValues[MAX_WEP_UPGRADES];

	WeaponArchetype() {
		for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
			upgradeIds[i] = WUP_INVALID;
			wepUpValues[i] = 0.f;
		}
	}
};

struct TurretArchetype
{
	//The ID of this archetype.
	dataId archetypeId = -1;
	//The name of this archetype.
	std::string name = "";
	//The data ID of the turret being used for the archetype.
	dataId turretDataId = -1;

	//Does this weapon use an archetype or not? If yes, true.
	bool usesWepArchetype[MAX_HARDPOINTS];
	//If this weapon has the same usesWepArchetype set, the ID is a weapon archetype id. If false it's a weapon data id.
	dataId weps[MAX_HARDPOINTS];

	TurretArchetype() {
		for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
			usesWepArchetype[i] = false;
			weps[i] = INVALID_DATA_ID;
		}
	}
};

struct ShipArchetype
{
	//The ID of this archetype.
	dataId archetypeId = -1;
	//The name of this archetype.
	std::string name= "";
	//The data ID of the ship being used for the archetype.
	dataId shipDataId = -1;
	//Upgrade IDs for the archetype (if applicable).
	SHIPUPGRADE_TYPE upgradeIds[MAX_SHIP_UPGRADES];
	//Values for the archetype (if applicable).
	f32 upgradeValues[MAX_SHIP_UPGRADES];

	//Does this weapon use an archetype or not? If yes, true.
	bool usesWepArchetype[MAX_HARDPOINTS];
	//If this weapon has the same usesWepArchetype set, the ID is a weapon archetype id. If false it's a weapon data id.
	dataId weps[MAX_HARDPOINTS];

	//Does this weapon use an archetype or not? If yes, true.
	bool usesPhysArchetype = false;
	//If this weapon has the same usesWepArchetype set, the ID is a weapon archetype id. If false it's a weapon data id.
	dataId physWep = INVALID_DATA_ID;
	//Does this weapon use an archetype or not? If yes, true.
	bool usesHeavyArchetype = false;
	//If this weapon has the same usesWepArchetype set, the ID is a weapon archetype id. If false it's a weapon data id.
	dataId heavyWep = INVALID_DATA_ID;

	//Does this turret use an archetype or not? If yes, true.
	bool usesTurretArchetype[MAX_TURRET_HARDPOINTS];
	//If this turret has the same usesTurretArchetype set, the ID is a turret archetype id. If false it's a turret data id.
	dataId turretArchetypes[MAX_TURRET_HARDPOINTS];
	//unused if turret archetypes are not used. If used, corresponds to a weapon ID for the turret ID.
	dataId turretWeps[MAX_TURRET_HARDPOINTS];

	//IDs for archetypes in the ship's hangar (if applicable).
	dataId hangarArchetypes[MAX_HANGAR_SHIPTYPES];

	ShipArchetype() {
		for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
			upgradeIds[i] = SUP_INVALID;
			upgradeValues[i] = 0.f;
		}
		for (u32 i = 0; i < MAX_HANGAR_SHIPTYPES; ++i) {
			hangarArchetypes[i] = INVALID_DATA_ID;
		}
		for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
			usesWepArchetype[i] = false;
			weps[i] = INVALID_DATA_ID;
		}
		for (u32 i = 0; i < MAX_TURRET_HARDPOINTS; ++i) {
			usesTurretArchetype[i] = false;
			turretArchetypes[i] = INVALID_DATA_ID;
			turretWeps[i] = INVALID_DATA_ID;
		}
	}
};

#endif 