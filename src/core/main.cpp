#include "BaseHeader.h"
#include "GameStateController.h"
#include "GvReader.h"
#include "Config.h"
#include "AudioDriver.h"
#include "BaedsLightManager.h"
#include "Shaders.h"
#include "CrashLogger.h"
#include "GameAssets.h"
#include "Campaign.h"
#include "steam_api.h"

GameStateController* stateController = 0;
GameController* gameController = 0;
GuiController* guiController = 0;

IrrlichtDevice* device = 0;
IVideoDriver* driver = 0;
ISceneManager* smgr = 0;
IGUIEnvironment* guienv = 0;
BulletPhysicsWorld* bWorld = 0;
flecs::world* game_world = 0;

std::unordered_map<s32, ShipData*> shipData;
std::unordered_map<s32, TurretData*> turretData;
std::unordered_map<s32, WeaponData*> weaponData;
std::unordered_map<s32, WeaponData*> physWeaponData;
std::unordered_map<s32, ObstacleData*> obstacleData;
std::unordered_map<s32, WeaponData*> heavyWeaponData;
std::unordered_map<s32, ShipUpgradeData*> shipUpgradeData;
std::unordered_map<s32, WeaponUpgradeData*> weaponUpgradeData;
std::unordered_map<s32, WingmanMarker*> wingMarkers;

std::unordered_map<dataId, WeaponArchetype*>  weaponArchetypeData;
std::unordered_map<dataId, TurretArchetype*>  turretArchetypeData;
std::unordered_map<dataId, ShipArchetype*>  shipArchetypeData;

Assets* assets = new Assets;
Campaign* campaign = new Campaign;
AudioDriver* audioDriver = new AudioDriver(2);
BaedsLights* lmgr = new BaedsLights();
ShaderManager* shaders = new ShaderManager();

Config* cfg = new Config;

_BaedsRandGen random;

const std::string entDebugStr(flecs::entity ent)
{
	std::string ret = "";
	if (ent.is_alive()) {
		if (ent.doc_name()) ret = ent.doc_name();
		else ret = "[UNNAMED] ";
		ret += "(" + std::to_string(ent.id()) + ")";
	} else {
		ret = "DEAD ENTITY " + std::to_string(ent.id());
	}
	return ret;
}

int main()
{
	cfg->vid.loadConfig("assets/cfg/videoconfig.gdat");
	cfg->vid.saveConfig("assets/cfg/videoconfig.gdat");
	dimension2du res(cfg->vid.resX, cfg->vid.resY);
	if (cfg->vid.useScreenRes) {
		IrrlichtDevice* nullDev = createDevice(EDT_NULL); //Used to get the current screen res if needed
		res = nullDev->getVideoModeList()->getDesktopResolution();
		nullDev->drop();
	}
	device = createDevice(cfg->vid.driver, res, 32, cfg->vid.toggles[TOG_FULLSCREEN], cfg->vid.toggles[TOG_STENCILBUF], cfg->vid.toggles[TOG_VSYNC], 0);
	driver = device->getVideoDriver();
	smgr = device->getSceneManager();
	smgr->setLightManager(lmgr);
	guienv = device->getGUIEnvironment();
	stateController = new GameStateController();
#ifndef _DEBUG //only monitor when not, like, actively debugging...
	baedsLogger::monitor();
#endif
	//This got RUDE with me, maybe go back and error handle later
	if (SteamAPI_Init()) {
		baedsLogger::log("Steam API successfully initialized\n");
	}
	stateController->init();
	stateController->mainLoop();
	device->drop();
	delete stateController;
	return 0;

}