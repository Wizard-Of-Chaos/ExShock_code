#pragma once
#include "BaseHeader.h"
#include "ShipComponent.h"
#include "WeaponInfoComponent.h"
#include "HealthComponent.h"
#include "MissileComponent.h"
#include "ObstacleComponent.h"
#include "HangarComponent.h"
#include "BolasComponent.h"
#include "AIComponent.h"
#include "ThrustComponent.h"
#include "HardpointComponent.h"
#include "TurretHardpointComponent.h"
#include "StationModuleComponent.h"
#include "PowerComponent.h"
#include "FactionComponent.h"

struct _materialTexLayer
{
	u32 count = 0;
	std::string tex[MATERIAL_MAX_TEXTURES] = {"", "", "", "", "", "", "", ""};
};

/*
* Contains all data necessary to be able to properly load a ship in the game.
* The u32 id is NOT an entity id.
*/
struct ShipData
{
	s32 id;

	f32 buildCost = 0;
	bool canBuild = false;
	bool canLoot = false;

	bool hasTurrets = false;
	bool hasHangar = false;
	bool artilleryShip = false;

	f32 mass;
	vector3df scale;

	std::string name;
	std::string description;
	HangarComponent hangar;
	TurretHardpointComponent turr;
	ShipComponent ship;
	ThrustComponent thrust;
	HardpointComponent hards;
	HealthComponent hp;
	PowerComponent power;
	btConvexHullShape collisionShape;

	std::string shipMesh;
	std::string shipCollisionMesh = "";

	std::vector<_materialTexLayer> materials;
	std::string engineTexture;
	std::string jetTexture;

	SColor engineTipCenter;
	SColor engineTipEdge;
	SColor engineEndCenter;
	SColor engineEndEdge;
	SColor shieldColor;

};

/*
* Extends the ship data class and adds in a carrier component that can also be loaded.
*/

struct ChaosTheoryStats
{
	HealthComponent hp;
	ThrustComponent thrst;
	u32 turretId=0;
	u32 turretWepId=3;
};

/*
* Contains necessary data for building a turret in the game.
*/
struct TurretData
{
	s32 id;
	HardpointComponent hards;
	ThrustComponent thrust;
	HealthComponent hp;

	std::string name;
	std::string description;

	std::string mesh;
	std::vector<_materialTexLayer> materials;
};
/*
* Contains all baseline data for a given weapon. The u32 id is NOT an entity id.
* Take note of additional subclasses when loading and constructing new weapon types.
*/
struct WeaponData
{
	s32 id;
	f32 buildCost = 0;
	bool canBuild = false;
	bool canLoot = false;

	std::string name;
	std::string description;
	WeaponInfoComponent wepComp;

	std::string weaponMesh;
	std::vector<_materialTexLayer> materials;
	std::string weaponEffect;
	SColor muzzleFlashColor;
	//SColor projectileLightColor;
};

/*
* An extension to the base WeaponData class that also contains a mesh for the missile model
* and the missile component associated with it.
*/
struct MissileData : public WeaponData
{
	MissileInfoComponent miss;
	ThrustComponent missThrust;
	std::string missileMesh;
	std::string missileTexture;
};

struct BolasData : public WeaponData
{
	BolasInfoComponent bolas;
};

struct WingmanInstance
{
	instId id; //0 is the player
	std::string name;
	std::string description;
	std::string personality;

	std::string attackLine;
	std::string formUpLine;
	std::string deathLine;
	std::string killLine;
	std::string negLine;
	std::string dockLine;
	std::string helpLine;
	std::string disengageLine;
	std::string haltLine;

	std::string bustTotalLine;
	std::string bustSalvageLine;
	std::string bustAmbushLine;
	std::string getBackInsideLine;
	std::string missionAccomplishedLine;

	AIComponent ai;
	instId assignedShip = -1;
	u32 totalKills = 0;
	u32 totalInjuries = 0;
	u32 turnsInjured = 0;
	bool injured = false;
	bool assigned = false;
};

//used to store the id of the wingman
struct WingmanMarker
{
	instId id=-1;
	u32 minSector = 0;
	std::wstring flag; //recruit flag
	std::string path;
};

/*
* All the data necessary to load an obstacle, including type, id, mesh, texture, and shape.
*/
struct ObstacleData
{
	s32 id;
	std::string obstacleMesh;
	std::vector<_materialTexLayer> materials;

	std::string name;
	btConvexHullShape shape;
	OBSTACLE type;
	HealthComponent hp;
	std::string flatTexture = "";
};

struct StationData : public ObstacleData
{
	TurretHardpointComponent turretComponent;
	f32 scale;
};

const u32 MAX_STATION_MODULE_CONNECTIONS = 4;

struct StationModuleData : public ObstacleData
{
	bool hasTurrets = false;
	bool hasHangar = false;
	TurretHardpointComponent turretComp;
	HangarComponent hangarComp;
	StationModuleComponent moduleComp;
	PowerComponent power;
};

struct MapGenObstacle
{
	//there are two weird exceptions, where it should generate a *ship* as a dead obstacle instead of a regular obstacle, or a weapon as an obstacle
	//if so, mark it
	bool isShip = false;
	bool isWeapon = false;

	dataId id = 0;
	flecs::entity_t entity = INVALID_ENTITY_ID;

	vector3df position = vector3df(0, 0, 0);
	vector3df rotation = vector3df(0, 0, 0);
	vector3df scale = vector3df(1, 1, 1);
	f32 startLinVel = 0;
	f32 startRotVel = 0;
	f32 mass = 1;
	FACTION_TYPE faction = FACTION_NEUTRAL;
	//some of the things that get punted across need extra data, store it here
	f32 extraFloats[8] = { 0,0,0,0,0,0,0,0 };
};

struct MapGenShip
{
	MapGenShip() { for (u32 i = 0; i < MAX_HARDPOINTS; ++i) { hardpoints[i] = INVALID_DATA_ID; wepArchetype[i] = false; hardpointEntities[i] = INVALID_ENTITY_ID;} }
	MapGenShip(dataId id, bool isArchetype = false, vector3df position = vector3df(0.f), vector3df rotation = vector3df(0.f), FACTION_TYPE faction = FACTION_NEUTRAL) :
		id(id), isArchetype(isArchetype), position(position), rotation(rotation), faction(faction) {
		for (u32 i = 0; i < MAX_HARDPOINTS; ++i) { hardpoints[i] = INVALID_DATA_ID; wepArchetype[i] = false; hardpointEntities[i] = INVALID_ENTITY_ID; }
	}
	//tears out data from an entity
	MapGenShip(flecs::entity ent, vector3df position = vector3df(0.f), vector3df rotation = vector3df(0.f));
	bool isArchetype = false;

	dataId id = 0;
	flecs::entity_t entity = INVALID_ENTITY_ID;

	bool wepArchetype[MAX_HARDPOINTS];
	flecs::entity_t hardpointEntities[MAX_HARDPOINTS];
	dataId hardpoints[MAX_HARDPOINTS];
	bool physArchetype = false;
	dataId phys = INVALID_DATA_ID;
	flecs::entity_t physEntity = INVALID_ENTITY_ID;
	bool heavyArchetype = false;
	dataId heavy = INVALID_DATA_ID;
	flecs::entity_t heavyEntity = INVALID_ENTITY_ID;

	vector3df position = vector3df(0, 0, 0);
	vector3df rotation = vector3df(0, 0, 0);
	FACTION_TYPE faction = FACTION_HOSTILE;
};