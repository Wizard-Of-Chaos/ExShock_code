#pragma once
#ifndef CARRIERUTILS_H
#define CARRIERUTILS_H
#include "BaseHeader.h"

//Sets up a default alien carrier.
flecs::entity createAlienCarrier(dataId id, vector3df pos, vector3df rot, const dataId weaponId=15, const s32 turretId=1, const s32 turretWepId=1);

flecs::entity createHumanCarrier(dataId id, vector3df pos, vector3df rot, const dataId weaponId, const s32 turretId=0, const s32 turretWepId=0);

//Creates the player carrier.
flecs::entity createChaosTheory(vector3df pos, vector3df rot, bool holdPos=false);

flecs::entity createTroopTransport(vector3df pos);

//The death callback for carriers; causes a large explosion and a different noise.
void carrierDeathExplosionCallback(flecs::entity id);
#endif