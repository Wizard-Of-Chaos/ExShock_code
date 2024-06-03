#pragma once
#ifndef OBSTACLEUTILS_H
#define OBSTACLEUTILS_H
#include "BaseHeader.h"
#include "FactionComponent.h"
/*
* Creates a dynamic obstacle from the given ID. Usually called from other functions. Pulls the data out of the obstacle
* data and gives it the given scale, position, rotation, mass, and sets it up with rigid body data if applicable.
*/
flecs::entity createDynamicObstacle(u32 id, vector3df position, vector3df rotation, vector3df scale, f32 mass, f32 startLinVel=0, f32 startRotVel=0, bool startActivated = false);
//Creates a static obstacle. Effectively the same as a dynamic obstacle, except that with a mass of 0, the obstacle doesn't move.
flecs::entity createStaticObstacle(u32 id, vector3df position, vector3df rotation, vector3df scale);

//Creates an asteroid at the given position. Includes health, irrlicht, and rigid body components. Returns the ID.
flecs::entity createAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 mass, f32 startLinVel=0, f32 startRotVel=0);
//Creates an explosive asteroid at the given position. Includes health, irrlicht, and rigid body components. Returns the ID.
flecs::entity createExplosiveAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 mass);
//Creates a gigantic asteroid with mass 0 at the given position. This means this asteroid will never move. Apply rings around these.
flecs::entity createHugeAsteroid(vector3df position, vector3df rotation, vector3df scale);
//Creates a swarm of asteroids at the given position and if velocities are set kicks them in the appropriate direction. Mildly randomized.
void createAsteroidSwarm(vector3df position, vector3df rotation, vector3df scale, f32 mass, f32 startLinVel= 0, f32 startRotVel=0);
flecs::entity createIceAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel=0, f32 startRotVel=0, bool split=true);
flecs::entity createIceSpikeAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel, f32 startRotVel);
flecs::entity createRadioactiveAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel, f32 startRotVel);
//I FUCKING LOVE PHILOSOPHER'S STONE!
flecs::entity createMoneyAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel = 0, f32 startRotVel = 0);
flecs::entity createCashNugget(vector3df position, vector3df rotation, vector3df scale, f32 startVel = 0, f32 startRotVel = 0);

//Creates engine debris at the given position. Includes health, irrlicht, rigid body and thrust components. Returns the ID.
flecs::entity createEngineDebris(vector3df position, vector3df rotation, vector3df scale, f32 mass);
//Creates a supply box at the given position and sets up the supplies callback. Includes health, rigid body, and irrlicht components. Returns the ID.
flecs::entity createSupplyBox(vector3df position, vector3df rotation, vector3df scale);
//Creates a fuel tank at the given position and sets up the explosive death callback. Includes health, rigid body, and irrlicht components. Returns the ID.
flecs::entity createFuelTank(vector3df position, vector3df rotation, vector3df scale);
//Creates a mine at the given position and sets up an explosive death callback. Includes health, rigid body, AI, and irrlicht components. Returns the ID.
flecs::entity createMine(vector3df position, vector3df rotation, vector3df scale, FACTION_TYPE triggeredBy=FACTION_NEUTRAL, bool flashing=true);
//Creates a stray ship warhead at the given position and sets up AI and explosive death callbacks.  Includes health, rigid body, AI, and irrlicht components. Returns the ID.
flecs::entity createDebrisMissile(vector3df position, vector3df rotation, vector3df scale);
//Creates a free-floating turret at the given position and sets up a universally hostile AI. Includes health, rigid body, AI, and irrlicht components. Returns the ID.
flecs::entity createDebrisTurret(vector3df position, vector3df rotation, vector3df scale);
//Picks a debris element at random and sets it at the given position. Returns the ID.
flecs::entity createRandomDebrisElement(vector3df position, vector3df rotation, vector3df scale);
flecs::entity createRandomShipDebris(vector3df position, vector3df rotation, vector3df scale);

flecs::entity createBeacon(vector3df pos, vector3df rot, vector3df scale);

//Creates a ship, minus all essential ship components, which can act as an obstacle. Includes obstacle component.
flecs::entity createDeadShipAsObstacle(dataId which, vector3df pos, vector3df rot, dataId wepId);
//Creates a dormant ship, without AI or any other components to make it *move*. Can be re-activated by adding on the necessary components. Does not include obstacle component.
flecs::entity createDormantShipAsObstacle(dataId which, vector3df pos, vector3df rot, dataId wepId);

//Creates a gas cloud at the given position with the given scale. Includes health and irrlicht components along with a bullet ghost component. Returns the ID.
flecs::entity createExplosiveCloud(vector3df position, vector3df scale);
//Creates a gas cloud that drains shields at the given location.
//Includes: Health, irrlicht, bullet ghost. Returns: ID
flecs::entity createShieldDrainCloud(vector3df position, vector3df scale);
//Creates a gas cloud that boosts speed at the given location.
//Includes: Health, irrlicht, bullet ghost. Returns: ID
flecs::entity createSpeedBoostCloud(vector3df position, vector3df scale);
//Creates a gas cloud that slows speed at the given location.
//Includes: Health, irrlicht, bullet ghost. Returns: ID
flecs::entity createSlowDownCloud(vector3df position, vector3df scale);
//Creates a gas cloud from the given positions.
void createCloudFormation(std::vector<vector3df> positions);
//Creates a random cloud.
flecs::entity createRandomCloud(vector3df position, vector3df scale);
//Creates a specific *type* of cloud - used for cloud formations.
flecs::entity createRolledCloud(vector3df position, vector3df scale, u32 roll);
flecs::entity createGravityAnomaly(vector3df position, vector3df scale);
flecs::entity createDustCloud(vector3df position, vector3df scale);

flecs::entity createStaticMeshCloud(vector3df position, vector3df rotation, vector3df scale, SColor innerColor=(255,255,255,255), SColor outerColor=(0,255,255,255));

//Creates a stasis pod at the given position with nothing particularly interesting about it.
flecs::entity createStasisPod(vector3df position, vector3df rotation);
//Creates some debris at the given position. Returns the ID.
flecs::entity createDebris(vector3df position, vector3df rotation, vector3df scale, f32 mass);

//Builds a weapon platform that can be captured, with the given turret ID and weapon ID.
flecs::entity createCaptureableWepPlatform(vector3df position, vector3df rotation, s32 turretId, s32 wepId);

//Creates a "dormant" rock ship at the given position and rotation.
flecs::entity createDormantRockShip(vector3df position, vector3df rotation);

//Jumpscares the player by initializing the rock in question as a *ship*.
void hugeRockJumpscare(flecs::entity rockShip);
void hugeRockJumpscareHitCb(flecs::entity attacker, flecs::entity rockShip);

//Builds a minefield out of the set positions by spawning in mines at each pos. Uses the given scale.
void createMinefield(std::vector<vector3df> positions, vector3df scale);
//Builds a minefield at the given position, getting random positions within the radius until out of mines.
void createRandomMinefield(vector3df position, vector3df scale, u32 numMines=35, f32 radius=500.f);

//Callback for when a gas cloud blows up. Stand back.
void deathExplosion(flecs::entity id);
void techDeathExplosion(flecs::entity id);
void gravDeathExplosion(flecs::entity id);
//When a fuel tank goes kablooey.
void fuelDeathExplosion(flecs::entity id);
//When a mine goes kablooey. Fixed values on damage and radius.
void mineExplosion(flecs::entity id);
//Adds supplies or ammo to the campaign.
void supplyBoxDeath(flecs::entity id);
//Splits up a rock ship into smaller ships on death.
void rockSplitEnemyDeath(flecs::entity id);
//Death callback for when a stasis pod gets collected.
void stasisPodCollect(flecs::entity id);
//Death callback for ice rocks.
void iceSplitExplosion(flecs::entity id);
//Death callback for money rocks.
void moneySplitExplosion(flecs::entity id);
//Callback for when an engine is hit by a bullet and it flies off into the void.
void onHitDebrisEngineTakeoff(flecs::entity target, flecs::entity attacker);
#endif 