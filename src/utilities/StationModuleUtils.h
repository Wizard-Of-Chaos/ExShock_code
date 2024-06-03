#pragma once
#ifndef STATIONMODULEUTILS_H
#define STATIONMODULEUTILS_H
#include "BaseHeader.h"
#include "FactionComponent.h"
#include "StationModuleComponent.h"

const extern std::unordered_map<STATION_PIECE, s32> stationPieceIds;

//Builds a modular human station with the set number of pieces, the set scale, and the set turrets.
std::vector<flecs::entity> createModularHumanStation(
	vector3df position, vector3df rotation,
	u32 numPieces = 2, FACTION_TYPE which=FACTION_PLAYER, vector3df scale = vector3df(10.f, 10.f, 10.f), s32 turretId = 0, s32 wepId = 3);

struct StationModuleData;

//Creates a random loose human station module. No turrets, factions, hangars or anything else is added. Effectively debris.
flecs::entity createLooseHumanModule(vector3df pos, vector3df rot, vector3df scale);

//Builds a "branch" for a given station. This will add on appropriate randomly generated modules on the given slot. Called recursively on itself until it's out of pieces.
void generateStationBranch(std::vector<flecs::entity>& retList, flecs::entity oldModule, s32 oldSlot, s32 numPieces, vector3df scale, FACTION_TYPE fac, bool hasShield = false, bool hasTurret = false, s32 turretId = 0, s32 wepId = 3);

//Gets the position of a new module on a currently existing station as a vector based on rotation, position, and slot alignments.
vector3df getNewModulePos(flecs::entity oldModule, s32 oldSlot, StationModuleData* newModule, s32 newSlot, vector3df scale=vector3df(1,1,1));
//Gets the rotation of a new module based on slot alignments.
vector3df getNewModuleRotRadians(flecs::entity oldModule, s32 oldSlot, StationModuleData* newModule, s32 newSlot);
//Gets the rotation of a new module based on slot alignments.
vector3df getNewModuleRotDegrees(flecs::entity oldModule, s32 oldSlot, StationModuleData* newModule, s32 newSlot);

//This removes all connected entities from a given station. It kills their power and removes them from the station module owner's list of modules.
void snapOffStationBranch(flecs::entity mod);

std::vector<flecs::entity> createModularStationFromFile(vector3df position, vector3df rotation, std::string fname, bool overrideFac=false, FACTION_TYPE fac=FACTION_UNCONTROLLED);

//Adds on a module to the given slot. Returns the ID. Used as part of generateStationBranch.
flecs::entity addModuleToSlot(flecs::entity owner, s32 newModuleId, s32 ownerSlot, s32 newModuleSlot, vector3df scale, 
	FACTION_TYPE fac = FACTION_PLAYER, s32 turretId =0, s32 wepId=3);
#endif 