#pragma once
#ifndef SHIPINSTANCE_H
#define SHIPINSTANCE_H
#include "BaseHeader.h"
#include "ShipComponent.h"
#include "WeaponInfoComponent.h"
#include "HealthComponent.h"
#include "HardpointComponent.h"

struct WingmanInstance;
struct ShipInstance;
class ShipUpgradeInstance;
class WeaponUpgradeInstance;

const u32 MAX_WEP_UPGRADES = 2;

struct WeaponInstance
{
	instId id;
	WeaponInfoComponent wep;
	WeaponFiringComponent fire;
	instId upgrades[MAX_WEP_UPGRADES];
	instId usedBy = -1;
	WeaponInstance() {
		for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
			upgrades[i] = -1;
		}
	}
};

/*
* This "instance" effectively tracks a given instance of a ship - its fuel, its ammunition, its health, et cetera - so that it can persist between
* scenarios. It doubles as a loadout structure where you can quickly load weapons and ship information from a given loadout. This is used both on the campaign
* and by carriers.
*/
const u32 MAX_SHIP_UPGRADES = 4;

struct ShipInstance
{
	instId id; //unique ID for this particular ship instance, assigned by the campaign
	std::string overrideName = ""; //player-set name
	ShipComponent ship;
	HardpointComponent hards;
	HealthComponent hp;
	instId weps[MAX_HARDPOINTS]; //since weapon info components have their ids, we should be able to pull any necessary data back out when loading
	instId physWep= -1;
	instId heavyWep= -1;
	instId inUseBy = -1;
	instId upgrades[MAX_SHIP_UPGRADES];
	ShipInstance() {
		for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
			upgrades[i] = -1;
		}
		for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
			weps[i] = -1;
		}
	}
};

#endif 