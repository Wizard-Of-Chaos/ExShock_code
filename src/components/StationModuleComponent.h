#pragma once
#ifndef STATIONMODULECOMPONENT_H
#define STATIONMODULECOMPONENT_H
#include "BaseHeader.h"

const u32 MAX_STATION_CONNECTION_POINTS = 4;

const u32 MAX_STATION_MODULES = 60;

struct StationModuleOwnerComponent
{
	flecs::entity self = INVALID_ENTITY;
	flecs::entity modules[MAX_STATION_MODULES];
	u32 modCount = 0;
	flecs::entity docks[MAX_STATION_MODULES];
	u32 dockCount = 0;
	std::list<flecs::entity> currentlyDockedShips;
	void toggleShields(bool shieldsUp = false);
	//checks to see if there is a shield module present on the list
	bool hasShieldModule();
	//This does not delete the associated entities; just removes it from the list.
	void removeModule(flecs::entity mod);
	//Sets all faction components on the station to neutral.
	void neutralizeStation();
	//Sets faction component on the given module to neutral.
	void neutralizeModule(flecs::entity mod);
	StationModuleOwnerComponent() {
		for (u32 i = 0; i < MAX_STATION_MODULES; ++i) {
			modules[i] = INVALID_ENTITY;
			docks[i] = INVALID_ENTITY;
		}
	}
};

enum STATION_PIECE
{
	SPIECE_TURRET_L,
	SPIECE_FILL_T,
	SPIECE_FILL_I,
	SPIECE_FILL_L,
	SPIECE_SENSOR,
	SPIECE_MAX_HUMAN_ROLLS, //max for rolling
	SPIECE_SHIELD,
	SPIECE_DOCK,
	SPIECE_FUEL_TANK,
	SPIECE_LARGE_GENERATOR,
	SPIECE_SMALL_GENERATOR,
	SPIECE_MAX_HUMAN, //max for humans
	SPIECE_ALIEN_FILL_I,
	SPIECE_ALIEN_TURRET_Y,
	SPIECE_MAX_ALIEN_ROLLS,
	SPIECE_ALIEN_SHIELD,
	SPIECE_ALIEN_DOCK,
	SPIECE_ALIEN_X_GENERATOR, //where's ben 10 when you need him
	SPIECE_MAX_ALIEN, //max for aliens
	SPIECE_BOSS_CENTER,
	SPIECE_BOSS_ARM,
	SPIECE_BOSS_SHIELD
};

struct StationModuleComponent
{
	STATION_PIECE type;
	bool hasDock = false;
	s32 connections = 0;
	s32 connectedOn = -1;
	flecs::entity ownedBy = INVALID_ENTITY;
	vector3df connectionPoint[MAX_STATION_CONNECTION_POINTS];
	bool upFlipped[MAX_STATION_CONNECTION_POINTS];
	vector3df connectionUp[MAX_STATION_CONNECTION_POINTS];
	vector3df connectionDirection[MAX_STATION_CONNECTION_POINTS];
	flecs::entity connectedEntities[MAX_STATION_CONNECTION_POINTS];
	StationModuleComponent() {
		for (u32 i = 0; i < MAX_STATION_CONNECTION_POINTS; ++i) {
			upFlipped[i] = false;
			connectedEntities[i] = INVALID_ENTITY;
		}
	}
};

#endif 