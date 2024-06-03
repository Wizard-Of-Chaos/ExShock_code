#include "GameStateController.h"
#include "Shaders.h"
#include "AttributeReaders.h"
#include "btUtils.h"
#include "CrashLogger.h"
#include "Config.h"
#include "AudioDriver.h"
#include "GameController.h"
#include "GuiController.h"
#include "GameAssets.h"
#include <filesystem>
#include <semaphore>

//#define THREADING_YOLO

GameStateController::GameStateController()
{
	std::srand((u32)time(NULL));
	gameInitialized = false;
	cfg->keys.loadConfig("assets/cfg/keyconfig.gdat");
	cfg->keys.saveConfig("assets/cfg/keyconfig.gdat");
}

void GameStateController::init()
{
	baedsLogger::log("Initializing game state controller...\n");
#ifdef _DEBUG
	stateController->networkingInit();
#endif
	device->setEventReceiver(stateController);
	guienv->setUserEventReceiver(stateController);
	then = device->getTimer()->getTime();
	state = GAME_MENUS; //Initial state
	driver->setMinHardwareBufferVertexCount(0);

	IGUIFont* defaultFont = assets->getFont("assets/fonts/HUD/14.xml");
	if (defaultFont) {
		guienv->getSkin()->setFont(defaultFont);
	}
	IGUIFont* tooltipDefaultFont = assets->getFont("assets/fonts/tooltip.xml");
	if (tooltipDefaultFont) {
		guienv->getSkin()->setFont(tooltipDefaultFont, EGDF_TOOLTIP);
	}
	guienv->getSkin()->setColor(EGDC_BUTTON_TEXT, SColor(245, 200, 200, 200));//SColor(255, 140, 250, 255));

	audioDriver->playMusic("main_menu.ogg");

	gameController = new GameController;

	guiController = new GuiController;
	guiController->init();
	shaders->compile();

	auto bank = guienv->getSkin()->getSpriteBank();
	auto id = bank->addTextureAsSprite(assets->getTexture("assets/hud/cursorCrosshair.png", false));
	m_crosshair = SCursorSprite(bank, id, vector2di(16, 16));
	crosshairId = device->getCursorControl()->addIcon(m_crosshair);
	this->stateUpdatePacket = new Packet();
	//toggleMenuBackdrop();
	baedsLogger::log("Game state controller initialized.\n");
}

void GameStateController::networkingInit()
{
	steamNetworkingSockets = SteamNetworkingSockets();
	steamNetworkingSockets->InitAuthentication();
	steamNetworkingUtils = SteamNetworkingUtils();
	steamNetworkingUtils->InitRelayNetworkAccess();
}
void GameStateController::openListenerSocket()
{
	if (listenSocket != 0) return; //don't open one when we already have a handle
	SteamNetworkingIdentity networkingIdentity = SteamNetworkingIdentity();
	networkingIdentity.SetSteamID64(SteamUser()->GetSteamID().ConvertToUint64());

	guiController->mpMenu()->setPlayerName(-1, std::to_wstring(SteamUser()->GetSteamID().ConvertToUint64()));

	char buf[128];
	networkingIdentity.ToString(buf, 128);

	baedsLogger::log("Networking identity: " + std::string(buf) + "\n");
	SteamFriends()->SetRichPresence("connect", buf);
	listenSocket = steamNetworkingSockets->CreateListenSocketP2P(0, 0, NULL);
	baedsLogger::log("Opened listener socket " + std::to_string(listenSocket) + "\n");
}
void GameStateController::closeListenerSocket()
{
	if(steamNetworkingSockets) steamNetworkingSockets->CloseListenSocket(listenSocket);
	baedsLogger::log("Closed listener socket " + std::to_string(listenSocket) + "\n");
	listenSocket = 0;
}
void GameStateController::loadAssets()
{
	loadShipAndWeaponData();
	gameInitialized = true;
	guiController->getActiveDialog()->hide();
	guiController->setActiveDialog(GUI_MAIN_MENU);
}

void GameStateController::toggleMenuBackdrop(bool toggle)
{
	if (toggle && tux) return;
	if (!toggle && !tux) return;

	if (toggle) {
		std::string sky = "assets/skyboxes/fleet_group/";
		skybox = smgr->addSkyBoxSceneNode(
			assets->getTexture(std::string(sky + "up.jpg").c_str()),
			assets->getTexture(std::string(sky + "down.jpg").c_str()),
			assets->getTexture(std::string(sky + "left.jpg").c_str()),
			assets->getTexture(std::string(sky + "right.jpg").c_str()),
			assets->getTexture(std::string(sky + "forward.jpg").c_str()),
			assets->getTexture(std::string(sky + "back.jpg").c_str()));
		ShipData* dat = shipData.at(0);
		tux = _loadObjModelData(dat->shipMesh, dat->materials, nullptr);
		tux->setMaterialType(shaders->getShaderMaterial(SHADE_2LIGHT_NORM));
		auto glow = smgr->addVolumeLightSceneNode(tux,
			ID_IsNotSelectable, 256, 256, SColor(255, 255, 255, 255), SColor(0, 0, 0, 0), dat->ship.engineJetPos[0],
			vector3df(-90, 0, 0), vector3df(2, 10, 2));

		ISceneNodeAnimator* glowie = getTextureAnim(dat->engineTexture, 30, true);
		glow->addAnimator(glowie);
		glowie->drop(); 

		cam = smgr->addCameraSceneNode(0, vector3df(25, 10, 0), vector3df(0));
		light = smgr->addLightSceneNode(0, vector3df(0, 320, 160), SColorf(.65f, .65f, 1.f), 8000.f);
		irr::core::array<vector3df> points;
		points.push_back(vector3df(6, 0, 0));
		points.push_back(vector3df(0, 0, 3));
		points.push_back(vector3df(-2, 0, 0));
		points.push_back(vector3df(0, 0, -5));
		auto anim = smgr->createFollowSplineAnimator(device->getTimer()->getTime(), points, .25f, .5f, true, false, false);
		tux->addAnimator(anim);
		anim->drop();

		dataId dats[8] = { 34, 14, 22, 11 , 6, 15, 5, 4};
		f32 ys[8] = { 25,15,-20,-30 , 10, 5, -35, 30};
		f32 xs[8] = { -40, -20, -55, -30 , -80, -50, -25, -35};
		for (u32 i = 0; i < 8; ++i) {
			ObstacleData* dat = obstacleData.at(dats[i]);
			flybys[i] = _loadObjModelData(dat->obstacleMesh, dat->materials, nullptr);
			auto anim = smgr->createFlyStraightAnimator(vector3df(xs[i], ys[i], 60 + (i * 40)), vector3df(xs[i], ys[i], -600), 16000 + (i * 2500), true);
			flybys[i]->addAnimator(anim);
			flybys[i]->setMaterialType(shaders->getShaderMaterial(SHADE_2LIGHT_NORM));
			anim->drop();
			anim = smgr->createRotationAnimator(vector3df(random.frange(0.f, .25f), random.frange(0.f, .25f), random.frange(0.f, .25f)));
			flybys[i]->addAnimator(anim);
			anim->drop();

			dat = obstacleData.at(dats[7 - i]);
			flybys2[i] = _loadObjModelData(dat->obstacleMesh, dat->materials, nullptr);
			anim = smgr->createFlyStraightAnimator(vector3df(xs[7 - i], ys[7 - i], 60 + (i * 40)), vector3df(xs[7 - i], ys[7 - i], -600), 16000 + (i * 2500), true);
			anim->setStartTime(0);
			flybys2[i]->addAnimator(anim);
			flybys2[i]->setMaterialType(shaders->getShaderMaterial(SHADE_2LIGHT_NORM));
			anim->drop();
			anim = smgr->createRotationAnimator(vector3df(random.frange(0.f, .25f), random.frange(0.f, .25f), random.frange(0.f, .25f)));
			flybys2[i]->addAnimator(anim);
			anim->drop();
		}
		ShipData* ctheory = shipData.at(7);
		flybys[8] = _loadObjModelData(ctheory->shipMesh, ctheory->materials, nullptr);
		flybys[8]->setScale(vector3df(1.5));
		auto rockanim = smgr->createFlyStraightAnimator(vector3df(-20, 0, 120), vector3df(-20, 0, -600), 45000, true);
		flybys[8]->addAnimator(rockanim);
		rockanim->drop();
		flybys[8]->setMaterialType(shaders->getShaderMaterial(SHADE_2LIGHT_NORM));

		for (u32 i = 0; i < ctheory->ship.engineCount; ++i) {
			glow = smgr->addVolumeLightSceneNode(flybys[8],
				ID_IsNotSelectable, 256, 256, SColor(255, 255, 255, 255), SColor(0, 0, 0, 0), ctheory->ship.engineJetPos[i],
				vector3df(-90, 0, 0), vector3df(2, 6, 2));
			ISceneNodeAnimator* glowie = getTextureAnim(ctheory->engineTexture, 30, true);
			glow->addAnimator(glowie);
			glowie->drop();
		}

	}
	else {
		skybox->remove();
		tux->remove();
		cam->remove();
		light->remove();
		skybox = nullptr;
		tux = nullptr;
		cam = nullptr;
		light = nullptr;
		for (u32 i = 0; i < 9; ++i) {
			flybys[i]->remove();
			flybys[i] = nullptr;
		}
		for (u32 i = 0; i < 8; ++i) {
			flybys2[i]->remove();
			flybys2[i] = nullptr;
		}
	}
}

void GameStateController::loadShipAndWeaponData()
{
	std::string basepath = "assets/attributes/";
	std::string weaponpath = basepath + "weapons/";
	std::string physweppath = weaponpath + "phys/";
	std::string heavyweppath = weaponpath + "heavy/";
	std::string regweppath = weaponpath + "regular/";
	std::string shippath = basepath + "ships/";
	std::string hullpath = basepath + "hulls/";
	std::string obstpath = basepath + "obstacles/";
	std::string carrierpath = basepath + "carriers/";
	std::string turretpath = basepath + "turrets/";
	std::string shipuppath = basepath + "upgrades/ships/";
	std::string weaponuppath = basepath + "upgrades/weapons/";

	std::string weparchetypepath = basepath + "archetypes/weapons/";
	std::string turretarchetypepath = basepath + "archetypes/turrets/";
	std::string shiparchetypepath = basepath + "archetypes/ships/";

	gvReader in;
	baedsLogger::log("Loading all ship data...\n");
	for (const auto& file : std::filesystem::directory_iterator(shippath)) {
		u32 id = loadShipData(file.path().string(), in);
		if (id != -1) {
			btConvexHullShape hull;
			std::string fname = hullpath + shipData[id]->name + ".bullet";
			if (loadHull(fname, hull)) {
				baedsLogger::log("Hull data loaded.\n");
				shipData[id]->collisionShape = hull;
			}
			else {
				baedsLogger::log("Hull data not found. Attempting to build from mesh... ");
				if (shipData[id]->shipCollisionMesh != "") {
					baedsLogger::log("Ship has collision mesh, converting to bullet3... ");
					IMesh* mesh = smgr->getMesh(shipData[id]->shipCollisionMesh.c_str());
					shipData[id]->collisionShape = createCollisionShapeFromColliderMesh(mesh);
					saveHull(fname, shipData[id]->collisionShape);
					smgr->getMeshCache()->removeMesh(mesh);
				}
				else {
					baedsLogger::log("Ship doesn't have collision mesh, building from model... ");
					IMesh* mesh = smgr->getMesh(shipData[id]->shipMesh.c_str());
					shipData[id]->collisionShape = createCollisionShapeFromMesh(mesh);
					saveHull(fname, shipData[id]->collisionShape);
					smgr->getMeshCache()->removeMesh(mesh);
				}
				baedsLogger::log("hull built.\n");
			}
		}
		in.clear();
	}
	baedsLogger::log("Done loading ships.\nLoading all turret data...\n");
	for (const auto& file : std::filesystem::directory_iterator(turretpath)) {
		loadTurretData(file.path().string(), in);
		in.clear();
	}
	baedsLogger::log("Done loading turrets.\nLoading all weapon data...\n");
	for (const auto& file : std::filesystem::directory_iterator(weaponpath)) {
		loadWeaponData(file.path().string(), in);
		in.clear();
	}
	for (const auto& file : std::filesystem::directory_iterator(physweppath)) {
		loadWeaponData(file.path().string(), in);
		in.clear();
	}
	for (const auto& file : std::filesystem::directory_iterator(heavyweppath)) {
		loadWeaponData(file.path().string(), in);
		in.clear();
	}
	for (const auto& file : std::filesystem::directory_iterator(regweppath)) {
		loadWeaponData(file.path().string(), in);
		in.clear();
	}
	baedsLogger::log("Done loading weapons.\nLoading all ship upgrade data...\n");
	for (const auto& file : std::filesystem::directory_iterator(shipuppath)) {
		loadShipUpgradeData(file.path().string(), in);
		in.clear();
	}
	baedsLogger::log("Done loading ship upgrades.\nLoading all weapon upgrade data...\n");
	for (const auto& file : std::filesystem::directory_iterator(weaponuppath)) {
		loadWeaponUpgradeData(file.path().string(), in);
		in.clear();
	}

	baedsLogger::log("Done loading weapon upgrades.\nLoading all wingman data...\n");
	std::string wingmanPath = "assets/attributes/wingmen/";
	for (const auto& file : std::filesystem::directory_iterator(wingmanPath)) {
		WingmanInstance* data = new WingmanInstance;
		if (!loadWingmanMarker(file.path().string())) continue;
	}

	baedsLogger::log("Done loading wingmen.\nLoading all obstacle data...\n");
	for (const auto& file : std::filesystem::directory_iterator(obstpath)) {
		u32 id = loadObstacleData(file.path().string(), in);
		in.clear();
		if (id == -1) continue;
		ObstacleData* data = obstacleData[id];
		if (data->type != GAS_CLOUD && data->type != FLAT_BILLBOARD_ANIMATED && data->type != FLAT_BILLBOARD) {
			btConvexHullShape hull;
			std::string fname = hullpath + data->name + ".bullet";
			if (loadHull(fname, hull)) {
				baedsLogger::log("Hull data loaded.\n");
				data->shape = hull;
			} else {
				baedsLogger::log("Hull data not found. Attempting to build from mesh... ");
				IMesh* mesh = smgr->getMesh(data->obstacleMesh.c_str());
				data->shape = createCollisionShapeFromMesh(mesh);
				saveHull(fname, data->shape);
				smgr->getMeshCache()->removeMesh(mesh);
				baedsLogger::log("hull built.\n");
			}
		}
	}

	baedsLogger::log("Done loading obstacles.\nLoading all weapon archetype data...\n");
	for (const auto& file : std::filesystem::directory_iterator(weparchetypepath)) {
		loadWeaponArchetypeData(file.path().string());
	}
	baedsLogger::log("Done loading weapon archetypes.\nLoading all turret archetype data...\n");
	for (const auto& file : std::filesystem::directory_iterator(turretarchetypepath)) {
		loadTurretArchetypeData(file.path().string());
	}
	baedsLogger::log("Done loading turret archetypes.\nLoading all ship archetype data...\n");
	for (const auto& file : std::filesystem::directory_iterator(shiparchetypepath)) {
		loadShipArchetypeData(file.path().string());
	}
	baedsLogger::log("Done loading ship archetypes.\nDATA COUNTS:\n");

	baedsLogger::log("Weapons: " + std::to_string(weaponData.size()) + ", heavy weapons: " + 
		std::to_string(heavyWeaponData.size()) + ", physics weapons: " + std::to_string(physWeaponData.size()) + ", weapon upgrades: " + std::to_string(weaponUpgradeData.size()) + "\n");
	baedsLogger::log("Ships: " + std::to_string(shipData.size()) + ", ship upgrades: " + std::to_string(shipUpgradeData.size()) + ", turrets: " + std::to_string(turretData.size()) + "\n");
	baedsLogger::log("Obstacles: " + std::to_string(obstacleData.size()) + "\n");
	baedsLogger::log("ARCHETYPES - Ship: " + std::to_string(shipArchetypeData.size()) + ", Weapon: " + std::to_string(weaponArchetypeData.size()) + ", Turret:" + std::to_string(turretArchetypeData.size()) + "\n");
}

bool GameStateController::OnEvent(const SEvent& event)
{
	if (!gameInitialized) return true;

	if (event.EventType == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown) {
			if (state == GAME_RUNNING) {
				setState(GAME_PAUSED); //Overrides anything else and sets the game to paused when the escape key is hit - needs abstraction later
			}
			else if (state == GAME_PAUSED) {
				setState(GAME_RUNNING);
			}
		}
	}
	switch (state) { //Passes events to their respective controllers and tells them its THEIR problem now
	case GAME_MENUS:
		guiController->OnEvent(event);
		break;
	case GAME_RUNNING:
		gameController->OnEvent(event);
		break;
	case GAME_PAUSED:
		guiController->OnEvent(event);
		break;
	case GAME_FINISHED:
		guiController->OnEvent(event);
		break;
	}
	return false;
}

void GameStateController::setState(GAME_STATE newState)
{
	oldState = state;
	nextState = newState;
	stateChangeCalled = true; //Lets the stateController know it needs to update the state. Can be called anywhere
}

GAME_STATE GameStateController::getState()
{
	return state;
}

void GameStateController::backToCampaign()
{
	audioDriver->cleanupGameSounds();
	campaign->getSector()->finishScenario();
	returningToCampaign = true;
	setState(GAME_MENUS);
}

void GameStateController::stateChange() //Messy handler for the different states; since there aren't many it's just an if chain
{
	state = nextState;
	if (oldState == GAME_MENUS && state == GAME_RUNNING) {
		//guiController->close();
		gameController->init();
		gameController->initScenario();
		guiController->close();
	}
	else if (oldState == GAME_PAUSED && state == GAME_MENUS) {
		gameController->close();
		device->getTimer()->start();
		guiController->setActiveDialog(GUI_MAIN_MENU);
	}
	else if (oldState == GAME_RUNNING && state == GAME_PAUSED) {
		guiController->setActiveDialog(GUI_PAUSE_MENU);
		device->getTimer()->stop();
	}
	else if (oldState == GAME_PAUSED && state == GAME_RUNNING) {
		guiController->close();
		device->getTimer()->start();
	}
	else if (oldState == GAME_RUNNING && state == GAME_FINISHED) { 
		device->getTimer()->stop();
		guiController->setActiveDialog(GUI_DEATH_MENU);
	}
	else if (oldState == GAME_FINISHED && state == GAME_MENUS) {
		device->getTimer()->start();
		gameController->close();
		if (returningToCampaign) {
#ifdef _DEMOVERSION
			if (campaign->getSector()->getType() == SECTOR_ASTEROID && campaign->getSector()->getEncounterNum() == MAX_ENCOUNTERS) {
				guiController->setActiveDialog(GUI_CREDITS_MENU);
				campaign->exitCampaign();
				guiController->setOkPopup("End of Demo", "Congratulations on beating the demo version of Extermination Shock!\n\nThere are five more sectors after this, however... will you step up to the challenge?", "Maybe");
				guiController->setLoadoutTrigger();
				audioDriver->playMusic("main_menu.ogg");
				audioDriver->setMusicGain(0, 1.f);
				audioDriver->stopMusic(1);
			}
			else guiController->setActiveDialog(GUI_LOOT_MENU);
#else
			if (campaign->getSector()->getType() == SECTOR_FINALE && campaign->getSector()->getEncounterNum() == MAX_ENCOUNTERS) {
				guiController->setActiveDialog(GUI_CREDITS_MENU);
				campaign->exitCampaign();
				guiController->setOkPopup("Thanks for Playing!",
					"Congratulations on beating Extermination Shock! The system is behind you, and the Chaos Theory has made it home.\n\nWe hope you enjoyed the game. We've got a lot more to come, and we hope you'll stick with us for further adventures in this world.", 
					"Ok");
				guiController->setLoadoutTrigger();
				audioDriver->playMusic("main_menu.ogg");
				audioDriver->setMusicGain(0, 1.f);
				audioDriver->stopMusic(1);
			}
			else guiController->setActiveDialog(GUI_LOOT_MENU);
#endif
			returningToCampaign = false;
		} else {
			if(goToOptions) guiController->setActiveDialog(GUI_OPTIONS_MENU);
			else guiController->setActiveDialog(GUI_MAIN_MENU);
			goToOptions = false;
		}
	}
	stateChangeCalled = false;
	loadHangFrames = 0;
}

void GameStateController::logicUpdate()
{
#ifdef _DEBUG
	NetworkingUpdate();
#endif
	if (stateChangeCalled) {
		if (loadHangFrames > 2) stateChange(); //Updates state if the change has been called by one of the controllers
		else ++loadHangFrames;
	}
	switch (state) { //Calls updates from its controllers based on the current state
	case GAME_MENUS:
	case GAME_PAUSED:
	case GAME_FINISHED:
		guiController->update();
		break;
	case GAME_RUNNING:
		gameController->update();
#ifdef _DEBUG
		bWorld->debugDrawWorld();
#endif
		break;
	}
	if (inCampaign) campaign->timeSpentInCampaign += (now - then);
	if (tux && cam) cam->setTarget(tux->getPosition() - vector3df(0,0,8));
	audioDriver->menuSoundUpdate(state == GAME_RUNNING);
}

static bool GAME_OPEN = true;

void GameStateController::mainLoop()
{
	u32 lastFPS = -1;
	if (!gameInitialized) {
		loadAssets();
	}
	while (device->run()) {
		now = device->getTimer()->getTime();
#ifdef _DEBUG
		SteamAPI_RunCallbacks();
#endif
		logicUpdate();
		baedsLogger::logSystem("Irrlicht Update");
#if _DEBUG
		//gonna level with myself i dont know what this debug thing does
		driver->setTransform(ETS_WORLD, IdentityMatrix);
		SMaterial material;
		material.Lighting = false;
		material.BackfaceCulling = false;
		material.ZBuffer = ECFN_ALWAYS;
		driver->setMaterial(material);
#endif 
		driver->beginScene(true, true, SColor(255, 20, 20, 20));

		if (guiController->renderTexNeeded()) {
			driver->setRenderTarget(guiController->getRenderTex(), true, true, SColor(0, 0, 0, 0));
			smgr->drawAll();
			driver->setRenderTarget(0, true, true);
		}

		smgr->drawAll();
		guienv->drawAll();

#if _DEBUG
		for (line3df line : debugLines) {
			driver->draw3DLine(line.start, line.end);
		}
		debugLines.clear();
#endif 
		/*
		if (now - then > 100) {
			driver->runAllOcclusionQueries(false);
			driver->updateAllOcclusionQueries();
		}
		*/
		driver->endScene();
		int fps = driver->getFPS();
		stringw tmp(L"Extermination Shock [");
		tmp += driver->getName();
		tmp += L"] FPS: ";
		if (lastFPS != fps) {
			tmp += fps;
		}
		else tmp += lastFPS;

		device->setWindowCaption(tmp.c_str());
		lastFPS = fps;
		f32 delta = (f32)(now - then) / 1000.f;
		then = now;
	}
	GAME_OPEN = false;
}

/// <summary>
/// This callback is invoked when a user connects or some change has occurred in their network status, or if this is the host, a change in network status of a client
/// </summary>
/// <param name="callback"></param>
void GameStateController::OnNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* callback)
{
	baedsLogger::log("Net connection status changed\n");
	HSteamNetConnection connection = callback->m_hConn;

	SteamNetConnectionInfo_t info = callback->m_info;

	ESteamNetworkingConnectionState oldState = callback->m_eOldState;

	if (info.m_hListenSocket)
	{
		if (oldState == k_ESteamNetworkingConnectionState_None && info.m_eState == k_ESteamNetworkingConnectionState_Connecting)
		{
			baedsLogger::log("Client is beginning to connect\n");
			// find out if there's an empty slot for them to join and shit
			for (uint32_t i = 0; i < MAX_PLAYERS; ++i)
			{
				if (!clientData[i].active)
				{
					// found an empty slot!
					EResult res = steamNetworkingSockets->AcceptConnection(connection);

					if (res != k_EResultOK)
					{
						char msg[256];
						sprintf(msg, "AcceptConnection returned %d\n", res);
						baedsLogger::log(msg);
						baedsLogger::log("\n");
						steamNetworkingSockets->CloseConnection(connection, k_ESteamNetConnectionEnd_AppException_Generic, "Failed to accept connection", false);
						return;
					}
					
					std::wstring id = std::to_wstring(info.m_identityRemote.GetSteamID64());
					guiController->mpMenu()->setPlayerName(i, id);
					guiController->mpMenu()->addTextToLog(L"Player ID " + id + L" connected\n");
					// probably send some initial data here, TBD
					clientData[i].active = true;
					clientData[i].connection = connection;
					clientData[i].steamIDUser = info.m_identityRemote.GetSteamID();
					clientData[i].tickCountLastData = 0;
					for (int j = 0; i < sizeof(clientData[i].packetSlotEmpty) / sizeof(bool); i++)
					{
						clientData[i].packetSlotEmpty[j] = true;
					}
					return;
				}
			}

			baedsLogger::log("Rejection connection: Server full\n");

			steamNetworkingSockets->CloseConnection(connection, k_ESteamNetConnectionEnd_AppException_Generic, "Server full!", false);
		}
		else if ((oldState == k_ESteamNetworkingConnectionState_Connecting || oldState == k_ESteamNetworkingConnectionState_Connected) && info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
		{
			baedsLogger::log("Client is dropping connection for whatever reason\n");
			// find out if they were actually registered in the players and remove them
			for (uint32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if (!clientData[i].active)
				{
					continue;
				}

				if (clientData[i].steamIDUser == info.m_identityRemote.GetSteamID())
				{
					baedsLogger::log("Dropping user\n");
					// TODO: launch an event or do something so the dropped user's shit gets cleaned up or repossessed

					steamNetworkingSockets->CloseConnection(connection, 0, nullptr, false);

					clientData[i] = ClientConnectionData();
					break;
				}
			}
		}
	}
	else
	{
		if ((oldState == k_ESteamNetworkingConnectionState_None || oldState == k_ESteamNetworkingConnectionState_Connecting || oldState == k_ESteamNetworkingConnectionState_FindingRoute) && info.m_eState == k_ESteamNetworkingConnectionState_Connected)
		{
			// remote host has accepted our connection! Rich presence key should be set at the same time connection was initiated so don't need to handle that here
			baedsLogger::log("Succesfully connected to host\n");
			guiController->setActiveDialog(GUI_MULTIPLAYER_MENU);
			gameController->setNetworked(true);
			guiController->mpMenu()->setPlayerName(-1, std::to_wstring(info.m_identityRemote.GetSteamID64()));
		}
		else if ((oldState == k_ESteamNetworkingConnectionState_Connecting || oldState == k_ESteamNetworkingConnectionState_FindingRoute  || oldState == k_ESteamNetworkingConnectionState_Connected) && info.m_eState == k_ESteamNetworkingConnectionState_ClosedByPeer)
		{
			baedsLogger::log("We've been dropped for some reason\n");
			steamNetworkingSockets->CloseConnection(connection, info.m_eEndReason, nullptr, false);
			SteamFriends()->SetRichPresence("connect", "");
			// need to handle what happens when our connection is closed by host
		}
		else if ((oldState == k_ESteamNetworkingConnectionState_Connecting || oldState == k_ESteamNetworkingConnectionState_FindingRoute || oldState == k_ESteamNetworkingConnectionState_Connected) && info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
		{
			baedsLogger::log("We've had a local problem, possibly timeout?\n");
			steamNetworkingSockets->CloseConnection(connection, info.m_eEndReason, nullptr, false);
			SteamFriends()->SetRichPresence("connect", "");
			// TODO: handle what happens when there's a local error
		}
		else
		{
			baedsLogger::log("Unknown state combo old state: " + std::to_string(oldState) + " new state: " + std::to_string(info.m_eState) + "\n");
		}
	}
}

/// <summary>
///  Callback occurs when user tries to join a game from their friends list or accepts an invite from a friend via InviteUserToGame
/// </summary>
/// <param name="callback"></param>
void GameStateController::OnGameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* callback) {
	baedsLogger::log("Rich presence join requested\n");
	SteamNetworkingIdentity networkingIdentity = SteamNetworkingIdentity();
	auto res = networkingIdentity.ParseString(callback->m_rgchConnect);
	if (res)
	{
		baedsLogger::log("Succesfully parsed networking identity: " + std::string(callback->m_rgchConnect) + "\n");
	}
	// TODO: maybe throw a pop up saying "Are you sure you want to quit" if important shit is happening
	auto conn = steamNetworkingSockets->ConnectP2P(networkingIdentity, 0, 0, nullptr);

	baedsLogger::log("Got connection " + std::to_string(conn) + "\n");
	// set the rich presence join value here, it'll get cleared out if there's a connection issue anyway
	SteamFriends()->SetRichPresence("connect", callback->m_rgchConnect);
	hostConnection = conn;
	// TODO: handle axing like everything currently going on and shunt the user to a waiting area while this connection request is processed by host/fails because bad request
}