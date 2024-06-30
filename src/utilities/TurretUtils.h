#pragma once
#ifndef TURRETUTILS_H
#define TURRETUTILS_H
#include "BaseHeader.h"
#include "FactionComponent.h"

flecs::entity createTurret(const s32 id, const s32 wepId, const vector3df pos, const vector3df rot, FACTION_TYPE faction, 
	const vector3df scale = vector3df(5, 5, 5), s32 slot = -1, flecs::entity owner=INVALID_ENTITY, NetworkId net=INVALID_NETWORK_ID);

flecs::entity createTurretFromArchetype(const dataId archId, const vector3df pos, const vector3df rot, FACTION_TYPE fac,
	const vector3df scale = vector3df(5, 5, 5), s32 slot = -1, flecs::entity owner = INVALID_ENTITY, NetworkId net = INVALID_NETWORK_ID);

vector3df getTurretPosition(const vector3df& turretPos, const ISceneNode* parentNode);

void setTurretConstraints(flecs::entity turret, flecs::entity owner, u32 hardpoint);

void initializeTurretsOnOwner(flecs::entity owner, const s32 turretId=1, const s32 wepId=1);

void initializeTurretsOnOwnerFromArchetypes(flecs::entity owner, const ShipArchetype* ship);
#endif