#pragma once
#ifndef SHIPUTILS_H
#define SHIPUTILS_H
#include "BaseHeader.h"
#include "FactionComponent.h"
#include "LoadoutData.h"

struct OutputDeathInfo {}; //flecs tag

//Builds the given ship with set weapons and position / rotation. Does not include AI or faction.
flecs::entity createShip(u32 shipDataId, u32 wepDataId, vector3df pos, vector3df rot, bool initializeParticles=true, bool isPlayer=false);
//Builds the given ship with set weapons and position / rotation. Sets the faction to hostile. If "decloaking" is set, this will play a de-cloak animation when spawned.
flecs::entity createHostileShip(u32 shipDataId, u32 wepDataId, vector3df pos, vector3df rot, bool decloaking=false);
//Builds the given ship with set weapons and position / rotation. Sets the faction to friendly. If "decloaking" is set, this will play a de-cloak animation when spawned.
flecs::entity createFriendlyShip(u32 shipDataId, u32 wepDataId, vector3df pos, vector3df rot, bool decloaking=false);

flecs::entity loadShipFromArchetype(dataId archetypeId, vector3df pos, vector3df rot, FACTION_TYPE fac, bool decloaking = false);

//Takes an existing ship and applies turret modules onto it, if applicable.
void setGunship(flecs::entity ship, s32 turretId, s32 turretWepId);
//Takes an existing ship and sets the AI functions to stay as far back as possible (sniper behavior).
void setArtillery(flecs::entity ship);

//Initializes a weapon component on the ship entity, set to the hardpoint specified. Phys bool is for physics weapons.
//Example: The "Tsunami LMG" has the weapon id 3. Pass in 3 to this function to load a Tsunami LMG onto the hardpoint.
//If you want to load a physics weapon set the bool to "true".
bool initializeWeaponFromId(u32 id, flecs::entity shipId, int hardpoint, HARDPOINT_TYPE type=HRDP_REGULAR, NetworkId networkId=INVALID_NETWORK_ID);

bool initializeWeaponfromArchetype(dataId archetype, flecs::entity shipId, int hardpoint, HARDPOINT_TYPE type = HRDP_REGULAR);

//Creates a faction component on the entity with the given hostilities, friendlies, and type.
void initializeFaction(flecs::entity id, FACTION_TYPE type, u32 hostiles, u32 friendlies, bool important = true);
//Creates a faction component on the entity with the default values for the given faction type.
void initializeFaction(flecs::entity id, FACTION_TYPE type, bool important=true);
//Creates a faction component on the given entity set to neutral.
void initializeNeutralFaction(flecs::entity id, bool important = false);
//Creates a faction component on the given entity set to hostile.
void initializeHostileFaction(flecs::entity id, bool important = true);
//Creates a faction component on the given entity set to the player faction.
void initializePlayerFaction(flecs::entity id, bool important = true);
//Changes the faction type and hostilities/friendlies of the given faction. Allegiances should be passed in as a bitmask.
void setFaction(FactionComponent* fac, FACTION_TYPE type, unsigned int hostilities, unsigned int friendlies, bool important=true);

//Adds sensors to the given entity. Requires a bullet and a faction component.
bool initializeSensors(flecs::entity id, f32 range, f32 updateInterval, bool onlyShips = false);
//Adds sensors to the given entity with default values. Requires a bullet component and a faction component.
bool initializeDefaultSensors(flecs::entity id);

//Adds shield component to the given entity.
void initializeShields(flecs::entity id, f32 amount, f32 delay, f32 recharge);
//Adds default shield values.
void initializeDefaultShields(flecs::entity objectId);

//Adds the particle system to a given ship.
void initializeShipParticles(flecs::entity id);

void initializeRegularWeapons(flecs::entity id, dataId wepDataId);

//Builds a ship from the given ship instance with its health, weapons, ship component and all the rest. Returns the ID.
flecs::entity createShipFromInstance(ShipInstance& inst, vector3df pos, vector3df rot, bool carrier=false, bool player=false);
//Creates a player ship from the current player instance.
flecs::entity createPlayerShip(vector3df pos, vector3df rot);

void initWingmanAI(flecs::entity player, flecs::entity id, WingmanInstance* wingData);
//Creates the given wingman, slots 1-3.
flecs::entity createWingman(u32 num, flecs::entity player, vector3df pos, vector3df rot);

//Spawns a ship from a carrier and applies the same faction component that the carrier has.
flecs::entity hangarLaunchShip(ShipInstance& inst, vector3df spawnPos, vector3df spawnRot, FactionComponent* carrFac);

//Builds out a wing of ships and returns a list of entities. The "ace" is at the first position in the list.
std::list<flecs::entity> spawnShipWing(
	vector3df pos, vector3df rot, dataId wingSize,
	dataId shipId = 1, dataId wepId = 1, dataId aceShipId = 1, dataId aceWepId = 1, dataId turretId=0, dataId turretWepId=1, bool friendly=false,
	bool decloaking=false);
//Builds out a wing of ships and returns a list of entities. The "ace" is at the first position in the list.
//This function is used to spawn ships *in the middle of a scenario*, i.e. when a spawn point is triggered.
std::list<flecs::entity> spawnScenarioWing(vector3df pos, vector3df rot, u32 shipId, u32 wepId, u32 aceShipId, u32 aceWepId, u32 turretId, u32 turretWepId);

//Builds a ship from a map generating object.
flecs::entity createShipFromMapGen(MapGenShip ship);

void fighterDeathSpiralCallback(flecs::entity id);
//The death callback used by fighters, a loud noise and a small bang.
void fighterDeathExplosionCallback(flecs::entity id);
#endif 
