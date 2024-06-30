#pragma once
#ifndef ATTRIBUTEREADERS_H
#define ATTRIBUTEREADERS_H
#include "BaseHeader.h"
#include "GvReader.h"
#include "ShipComponent.h"
#include "WeaponInfoComponent.h"
#include "IrrlichtComponent.h"
#include "TurretHardpointComponent.h"
#include "LoadoutData.h"

class GameController;
class GameStateController;
/*
* These functions are used to load various chunks of data using the GvReader class. Can load ships, weapons, and other stuff
* from .gdat files.
*/

ISceneNode* _loadObjModelData(std::string& modelPath, std::vector<_materialTexLayer>& materials, IrrlichtComponent* component, bool dummy = false);
//Loads ship data onto the given entity.
//Includes an Irrlicht component and a ShipComponent. Can also load carriers, and if the ship IS a carrier it'll tack on the carrier component.
bool loadShip(u32 id, flecs::entity entity, vector3df pos, vector3df rot, bool initializeParticles=true, NetworkId networkId=INVALID_NETWORK_ID);
//Loads weapon data onto the given entity.
//Includes a WeaponInfoComponent, an Irrlicht component, and whatever other components are necessary (e.g., a MissileInfoComponent).
bool loadWeapon(u32 id, flecs::entity weaponEntity, HARDPOINT_TYPE type=HRDP_REGULAR);

//Loads obstacle data onto the given entity.
bool loadObstacle(u32 id, flecs::entity entity);

//Loads turret data onto the given entity.
bool loadTurret(u32 id, flecs::entity entity);

//Pulls the ship data from the given .gdat file and saves it in the game state controller. Returns the ID.
s32 loadShipData(std::string path, gvReader& in);
//Pulls the weapon data from the given .gdat file and saves it in the game state controller. Returns the ID.
s32 loadWeaponData(std::string path, gvReader& in);
//Pulls out obstacle data from a given .gdat file and saves it in the game state controller. Returns the ID.
s32 loadObstacleData(std::string path, gvReader& in);
//Pulls out turret data from a given .gdat file and saves it in the game state controller. Returns the ID.
s32 loadTurretData(std::string path, gvReader& in);

//Pulls out ship upgrade data from a given .gdat file and saves it in the game state controller. Returns the ID.
s32 loadShipUpgradeData(std::string path, gvReader& in);
//Pulls out weapon upgrade data from a given .gdat file and saves it in the game state controller. Returns the ID.
s32 loadWeaponUpgradeData(std::string path, gvReader& in);

dataId loadShipArchetypeData(std::string path);
dataId loadTurretArchetypeData(std::string path);
dataId loadWeaponArchetypeData(std::string path);

//Creates a convex hull shape from an Irrlicht mesh and simplifies it down to something usable.
//This should only be used if there isn't a hitbox mesh available.
btConvexHullShape createCollisionShapeFromMesh(IMesh* mesh);
btConvexHullShape createCollisionShapeFromColliderMesh(IMesh* mesh);

//Saves a convex hull to file. Returns true if successful.
bool saveHull(std::string path, btConvexHullShape& shape);
//Loads a convex hull from file onto the "shape" hull. Returns true if successful.
bool loadHull(std::string path, btConvexHullShape& shape);

//Loads a wingman from file. Returns nullptr if unsuccessful.
WingmanInstance* loadWingman(const WingmanMarker* marker, bool setFlag=false);

//Loads a marker for the wingman based on the given path, not the actual wingman itself. Used to keep track of possible wingmen. Returns true if successful.
bool loadWingmanMarker(std::string path);

//Loads the texture and model data for the given ship, but doesn't apply it to anything. Done on loading screens.
void preloadShip(s32 id);
//Loads the texture and model data for the given weapon, but doesn't apply it to anything. Done on loading screens.
void preloadWep(s32 id);
//Loads the texture and model data for the given turret, but doesn't apply it to anything. Done on loading screens.
void preloadTurret(s32 id);

#endif 