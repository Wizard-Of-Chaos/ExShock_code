#pragma once

#ifndef BASEFLIGHTHEADER_H
#define BASEFLIGHTHEADER_H

/*
* This header holds all of the basic includes that are needed for the project.
* When adding new files, this should be the first thing you include.
*/
#include <irrlicht.h>
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <Serialize/BulletWorldImporter/btBulletWorldImporter.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h>

#include <flecs.h>

#include <al.h>
#include <alc.h>

#include <efx.h>
#include <EFX-Util.h>
#include <efx-creative.h>

#include <ogg.h>
#include <vorbisfile.h>
#include <vorbisenc.h>

#ifdef _MSC_VER
#pragma comment(lib, "Irrlicht.lib")
#endif

#ifdef _DEBUG
#pragma comment(lib, "BulletDynamics_Debug.lib")
#pragma comment(lib, "BulletCollision_Debug.lib")
#pragma comment(lib, "LinearMath_Debug.lib")
#pragma comment(lib, "BulletWorldImporter_Debug.lib")
#pragma comment(lib, "BulletFileLoader_Debug.lib")

#else 
#pragma comment(lib, "BulletDynamics.lib")
#pragma comment(lib, "BulletCollision.lib")
#pragma comment(lib, "LinearMath.lib")
#pragma comment(lib, "BulletWorldImporter.lib")
#pragma comment(lib, "BulletFileLoader.lib")

#ifdef _IRR_WINDOWS_
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

#endif 

#pragma comment(lib, "OpenAL32.lib")
#pragma comment(lib, "EFX-Util.lib")
#pragma comment(lib, "libogg.lib")
#pragma comment(lib, "libvorbis.lib")
#pragma comment(lib, "libvorbisfile.lib")
#pragma comment(lib, "steam_api64.lib")

#include <unordered_map>
#include <map>
#include <iostream>
#include <functional>
#include <algorithm>
#include <memory>
#include <random>

//init some random number generation
#include "RandomGeneration.h"

typedef int instId; //seems silly but helps keep my head straight
typedef int dataId;

typedef uint32_t NetworkId;
const NetworkId INVALID_NETWORK_ID = 0;


#define INVALID_DATA_ID -1

const flecs::id_t INVALID_ENTITY_ID = 0;
#define INVALID_ENTITY flecs::entity(INVALID_ENTITY_ID)

extern _BaedsRandGen random;

class GameStateController;
class GameController;
class GuiController;
class SceneManager;
class BulletPhysicsWorld;
class Assets;
class Campaign;
class AudioDriver;
class BaedsLights;
class ShaderManager;

struct Config;
/*
* These are global variables used from throughout the program to track what exactly is going on.
*/

extern GameStateController* stateController;
extern GameController* gameController;
extern GuiController* guiController;
extern Config* cfg;

extern Assets* assets;
extern Campaign* campaign;

extern IrrlichtDevice* device;
extern IVideoDriver* driver;
extern ISceneManager* smgr;
extern IGUIEnvironment* guienv;
extern BulletPhysicsWorld* bWorld;
extern flecs::world* game_world;

extern AudioDriver* audioDriver;
extern BaedsLights* lmgr;
extern ShaderManager* shaders;

struct ShipData;
struct TurretData;
struct WeaponData;
struct ObstacleData;
struct ShipUpgradeData;
struct WeaponUpgradeData;
struct WingmanMarker;

extern std::unordered_map<s32, ShipData*> shipData;
extern std::unordered_map<s32, TurretData*> turretData;
extern std::unordered_map<s32, WeaponData*> weaponData;
extern std::unordered_map<s32, WeaponData*> physWeaponData;
extern std::unordered_map<s32, WeaponData*> heavyWeaponData;
extern std::unordered_map<s32, ObstacleData*> obstacleData;
extern std::unordered_map<s32, ShipUpgradeData*> shipUpgradeData;
extern std::unordered_map<s32, WeaponUpgradeData*> weaponUpgradeData;
extern std::unordered_map<s32, WingmanMarker*> wingMarkers;

struct WeaponArchetype;
struct TurretArchetype;
struct ShipArchetype;

extern std::unordered_map<dataId, WeaponArchetype*>  weaponArchetypeData;
extern std::unordered_map<dataId, TurretArchetype*>  turretArchetypeData;
extern std::unordered_map<dataId, ShipArchetype*>  shipArchetypeData;

const std::string entDebugStr(flecs::entity ent);

//max amount of wingmen allowed
const u32 WINGMEN_RECRUITED_PER_SECTOR = 2;
const u32 WINGMEN_PER_CAMPAIGN = 8;
const u32 MAX_WINGMEN_ON_WING = 3;

//Max hardpoints.
const u32 MAX_HARDPOINTS = 6;
#define PHYS_HARDPOINT MAX_HARDPOINTS +1
#define HEAVY_HARDPOINT MAX_HARDPOINTS +2

#endif