#include "MapRunner.h"
#include "AttributeReaders.h"
#include "IrrlichtUtils.h"
#include "Config.h"
#include "GameFunctions.h"
#include "Campaign.h"
#include "GameController.h"
#include "ShipUtils.h"
#include "LargeShipUtils.h"
#include "StationModuleUtils.h"
#include "NodeAnimators.h"
#include "MissionUtils.h"
#include "TurretUtils.h"
#include "StatusEffects.h"
#include "BaedsLightManager.h"
#include "GameAssets.h"
#include "ObstacleUtils.h"
#include "GameStateController.h"
#include "AudioDriver.h"
#include <CrashLogger.h>
#include "SensorComponent.h"
#include "NetworkingComponent.h"
#include "ObstacleAI.h"

const std::unordered_map<SCENARIO_TYPE, std::string> objectiveTypeStrings = {
	{SCENARIO_KILL_HOSTILES, "Dogfight"},
	{SCENARIO_SALVAGE, "Salvage"},
	{SCENARIO_DESTROY_OBJECT, "Destroy"},
	{SCENARIO_SCRAMBLE, "Scramble"},
	{SCENARIO_BOSSFIGHT, "Carrier_battle"},
	{SCENARIO_RETRIEVE_POD, "Retrieve_pod" },
	{SCENARIO_BRAWL, "Battle"},
	{SCENARIO_CAPTURE, "Capture_station"},
	{SCENARIO_FRIGATE_RESCUE, "Frigate_rescue"},
	{SCENARIO_SCUTTLE, "Scuttle"},
	{SCENARIO_NOT_LOADED, "error"},
	{SCENARIO_ROCK_IMPACT, "Rock_impact"},
	{SCENARIO_EXTRACT_FUEL, "Extract_fuel"},
	{SCENARIO_DISTRESS_BEACON, "Distress_beacon"},
	{SCENARIO_HARVEST, "Harvest"},
	{SCENARIO_ESCORT, "Escort"},
	{SCENARIO_SEARCH_AND_DESTROY, "Search_and_destroy"},
	{SCENARIO_DUMMY, "Dummy"},

	{SCENARIO_ARNOLD, "Arnold"},
	{SCENARIO_ARTHUR, "Arthur"},
	{SCENARIO_SEAN, "Sean"},
	{SCENARIO_CAT, "Cat"},
	{SCENARIO_MI_CHA, "Lee"},
	{SCENARIO_TAURAN, "Tauran"},
	{SCENARIO_THEOD, "Theod"},
	{SCENARIO_JAMES, "James"},

	{SCENARIO_CUSTOM, "Custom"},
	{SCENARIO_TUTORIAL, "Tutorial"}
};

const std::unordered_map<SECTOR_TYPE, std::string> environmentTypeStrings = {
	{SECTOR_DEBRIS, "debris"},
	{SECTOR_ASTEROID, "asteroid"},
	{SECTOR_GAS, "gas"},
	{SECTOR_SUPPLY_DEPOT, "supply_depot"},
	{SECTOR_RINGS, "rings"},
	{SECTOR_FLEET_GROUP, "fleet_group"},
	{SECTOR_FINALE, "comet_shield"}
};

void MapRunner::build(SCENARIO_TYPE objective, SECTOR_TYPE sector, bool isCampaignMatch, std::string mapFile)
{
	baedsLogger::log("Building a scenario to run...\n");
	m_type = objective;
	m_sector = sector;
	m_boss = (m_type == SCENARIO_BOSSFIGHT);
	m_campaignMatch = isCampaignMatch;
	baedsLogger::log("Loading basic data for scenarios...\n");

	//TODO: stuff this into the end-map func
	m_mapData.clear();
	m_objectiveData.clear();
	m_objectivesStartPos.clear();
	m_objectives.clear();
	m_corridors.clear();
	m_mapShips.clear();
	m_mapObstacles.clear();
	m_obstaclePositions.clear();

	m_mapShips.clear();
	m_mapObstacles.clear();

	m_customMap = false;

	if (mapFile != "") {
		m_buildScenarioFromXml(mapFile);
	}

	m_mapData.read("assets/attributes/scenarios/environments/" + environmentTypeStrings.at(m_sector) + ".gdat");
	m_mapData.readLinesToValues();
	m_objectiveData.read("assets/attributes/scenarios/objectives/" + objectiveTypeStrings.at(m_type) + ".gdat");
	m_objectiveData.readLinesToValues();
	baedsLogger::log("Basic data loaded from file.\n");

	m_regAce = m_mapData.getInt("regAce");
	m_regShip = m_mapData.getInt("regShip");
	m_regAceGun = m_mapData.getInt("regAceGun");
	m_regGun = m_mapData.getInt("regGun");

	m_scoutAce = m_mapData.getInt("scoutAce");
	m_scoutAceGun = m_mapData.getInt("scoutAceGun");
	m_scoutShip = m_mapData.getInt("scoutShip");
	m_scoutGun = m_mapData.getInt("scoutGun");

	m_regTurret = m_mapData.getInt("regTurret");
	m_scoutTurret = m_mapData.getInt("scoutTurret");
	m_scoutTurretGun = m_mapData.getInt("scoutTurretGun");
	m_regTurretGun = m_mapData.getInt("regTurretGun");

	m_cleanup = (m_objectiveData.getString("hasCleanup") == "yes") ? true : false;
	m_bustSignals = (m_objectiveData.getString("hasBustSignals") == "yes") ? true : false;

	m_obstaclesAroundObjective = m_mapData.getFloat("obstaclesAroundObj");
	m_looseObstacles = m_mapData.getFloat("looseObstacles");

	m_obstDensityPerKm = m_mapData.getFloat("obstCorridorDensityPerKm");

	m_objectiveDeadZone = m_mapData.getFloat("objObstacleDeadZone");
	m_corridorRadius = m_mapData.getFloat("corridorRadius");
	m_objectiveFieldRadius = m_mapData.getFloat("objectiveObstFieldRadius");
	m_maxObjGenRadius = m_mapData.getFloat("maxObjGenRadius");
	m_maxObjTorusRadius = m_mapData.getFloat("maxObjTorusRadius");
	m_maxTorusDistance = m_mapData.getFloat("maxTorusDistance");

#ifdef _DEBUG
	//cut down the number of obstacles so that debugging runs at ok FPS
	baedsLogger::log("Chopping scenario obstacle counts for FPS during debugging...\n");
	u32 debugObstacleCut = 4;
	m_obstDensityPerKm /= debugObstacleCut;
	m_looseObstacles /= debugObstacleCut;
	m_obstaclesAroundObjective /= debugObstacleCut;

	f32 debugRadiusCut = 1.75f;
	m_corridorRadius /= debugRadiusCut;
	m_objectiveFieldRadius /= debugRadiusCut;
#endif 

	m_preload();
	m_setGlobals();
	m_setPlayerObjPositions();

	m_setPlayer();
	//gameController->initializeHUD();

	if (!gameController->isNetworked() || stateController->isHost()) {
		m_setObjectives();
		m_setObstacles();
	}

	m_timeBetweenBanters = 180.f;
	m_minTimeSinceBeingShot = 25.f;
	m_banterTimer = m_timeBetweenBanters - m_minTimeSinceBeingShot;

	if (isCampaignMatch) {
		campaign->loadRandomBanter();
		m_bantz = campaign->getAvailableBanter();
	}

	if (gameController->isNetworked()) {
		if (stateController->isHost()) {
			baedsLogger::log("Host sending off scenario obstacle data...\n");
			auto filter = game_world->filter<ObstacleComponent, NetworkingComponent>();
			filter.each([this](flecs::entity e, ObstacleComponent& obst, NetworkingComponent& net) {
				auto gen = createMapGenObstacleFromEntity(e);
				if (gen.networkId != INVALID_NETWORK_ID)
					this->addObstacleToMapObstacles(gen);
				});
		}
		auto filter2 = game_world->filter<ShipComponent, NetworkingComponent, BulletRigidBodyComponent>();
		filter2.each([this](flecs::entity e, ShipComponent& ship, NetworkingComponent& net, BulletRigidBodyComponent& rbc) {
			btTransform motionStateTransform;
			rbc.rigidBody->getMotionState()->getWorldTransform(motionStateTransform);
			vector3df pos = btVecToIrr(motionStateTransform.getOrigin());
			btVector3 eulerOrientation;
			QuaternionToEuler(motionStateTransform.getRotation(), eulerOrientation);
			vector3df rot = btVecToIrr(eulerOrientation);
			MapGenShip gen(e, pos, rot);
			if (gen.networkId != INVALID_NETWORK_ID)
				this->addShipToMapShips(gen);
			});
	}

	std::cout << "Map and scenario built!\n";
	if (xml) {
		delete xml;
		xml = nullptr;
	}
	startMap();
}

void MapRunner::startMap()
{
	//use this for initial cutscene
	if (!gameController->getPlayer().is_alive()) {
		//huh
		return;
	}
	auto node = gameController->getPlayer().get<IrrlichtComponent>()->node;
	vector3df endPos = node->getPosition();
	vector3df startPos = endPos + (getNodeBackward(node) * 100.f);
	audioDriver->playMenuSound("scenario_load.ogg");
	irr::core::array<vector3df> points;
	points.push_back(startPos); points.push_back(endPos);
	auto anim = smgr->createFollowSplineAnimator(device->getTimer()->getTime(), points, .2f, 1.f, false, false, false);
	node->addAnimator(anim);
	anim->drop();
}

bool MapRunner::run(f32 dt)
{
	m_banterTimer += dt;
	m_timeForBanter();

	Objective* obj = m_curObjective.get();
	if (!gameController->getPlayer().is_alive()) return true;

	if (obj->isObjOver(dt)) {
		//if we failed we're done here, return true
		if (!obj->success()) return true;
		audioDriver->playGameSound(gameController->getPlayer(), "objective_complete.ogg", 1.f, 100.f, 1200.f, false, true);
		//trigger the completion function for the objective
		obj->onComplete(); 

		//if the list is empty and we're not doing cleanup we're done here
		if (m_objectives.empty() && !m_cleanup) {
			return true;
		}
		//otherwise, make a cleanup objective and set that as the current objective
		else if (m_objectives.empty() && m_cleanup) {
			m_curObjective = m_getCleanup();
			return false;
		}

		//in all other cases we need to switch to the next objective, whatever's in the list
		m_curObjective = m_objectives.front();
		obj = m_curObjective.get();
		baedsLogger::log("Starting new objective. Type: " + objNameStrings.at(obj->type()) + "\n");
		//give it a radio spawn!
		m_setRadioSignals(m_curObjective);
		m_objectives.pop_front();
		return false;

	}
	return false;
}

void MapRunner::m_setRadioSignals(ObjElem& obj)
{
	gameController->clearRadioContacts();

	if (obj->hasRadioSpawn()) {
		if (radioSignal.is_alive()) destroyObject(radioSignal);
		radioSignal = createRadioMarker(obj->position() + vector3df(0.f, -75.f, 0.f), "Radio Signal");
	}

	for (const auto& ent : obj->getTargets()) {
		gameController->addContact(ent, true, true);
	}
}

void MapRunner::endMap()
{
	audioDriver->fadeTrack(0, .65f);
	audioDriver->fadeTrack(1, .65f);
	if (!gameController->getPlayer().is_alive()) {
		audioDriver->playMenuSound("mission_failed.ogg");
		return;
	}
	auto node = gameController->getPlayer().get<IrrlichtComponent>()->node;
	vector3df start = node->getAbsolutePosition();
	vector3df out = start + (getNodeForward(node) * 200.f);
	vector3df up = out + (getNodeUp(node) * 155.f);
	irr::core::array<vector3df> points;
	audioDriver->playMenuSound("mission_success.ogg");
	points.push_back(start); points.push_back(out); points.push_back(up);
	auto anim = smgr->createFollowSplineAnimator(device->getTimer()->getTime(), points, .35f, .5f, false, false, true);
	node->addAnimator(anim);
	anim->drop();
	//use this for exiting cutscene
}

bool MapRunner::m_timeForBanter()
{
	if (m_bantz.empty() || !gameController->getPlayer().is_alive() || m_type == SCENARIO_BOSSFIGHT || !cfg->game.toggles[GTOG_BANTER] || !m_campaignMatch || m_type == SCENARIO_TUTORIAL) {
		return false;
	}
	f32 lastTime = gameController->getPlayer().get<HealthComponent>()->timeSinceLastHit;
	if (lastTime >= m_minTimeSinceBeingShot && m_banterTimer >= m_timeBetweenBanters) {
		Banter b = m_bantz.back();
		for (auto& line : b.lines()) {
			gameController->addLargePopup(line.line, line.spkr, true);
		}
		m_banterTimer = 0;
		campaign->setBanterUsed(b.id());
		m_bantz.pop_back();
		return true;
	}
	return false;
}

void MapRunner::m_preload()
{
	baedsLogger::log("Preloading ships / weapons / animations for the scenario...\n");
	preloadShip(m_regAce);
	preloadShip(m_regShip);
	preloadWep(m_regAceGun);
	preloadWep(m_regGun);
	preloadShip(m_scoutAce);
	preloadShip(m_scoutShip);
	preloadWep(m_scoutAceGun);
	preloadWep(m_scoutGun);
	preloadTurret(m_regTurret);
	preloadTurret(m_scoutTurret);
	preloadWep(m_regTurretGun);
	preloadWep(m_scoutTurretGun);

	auto anim = getExplosionTextureAnim();
	anim->drop();
	anim = getExplosionSphereTextureAnim();
	anim->drop();
	anim = getExplosionTextureAnim(EXTYPE_TECH);
	anim->drop();
	anim = getExplosionTextureAnim(EXTYPE_ALIEN);
	anim->drop();
	anim = getTextureAnim("assets/effects/decloak/");
	anim->drop();
	anim = getFireTextureAnim();
	anim->drop();
	anim = getSmokeTextureAnim();
	anim->drop();
	anim = getTextureAnim("assets/effects/shieldhit/");
	anim->drop();
	anim = getTextureAnim("assets/effects/buff_defense/");
	anim->drop();
	anim = getTextureAnim("assets/effects/buff_offense/");
	anim->drop();
	anim = getTextureAnim("assets/effects/buff_shield/");
	anim->drop();

	baedsLogger::log("Assets pre-loaded.\n");
}

void MapRunner::m_setGlobals()
{
	baedsLogger::log("Setting up global lights and skybox data... ");
	//todo: set this as directional light
	ILightSceneNode* n = smgr->addLightSceneNode(0, vector3df(0, 80000, 0),
		m_mapData.getColorf("lightColor"), 800000.f);
	n->setID(ID_IsNotSelectable);
	lmgr->setGlobal(n);

	driver->setFog(m_mapData.getColor("fogColor"), EFT_FOG_LINEAR,
		100.f + ((f32)cfg->vid.renderDist * 200.f), 1000.f + ((f32)cfg->vid.renderDist * 800.f),
		.01f, true, true);

	std::string sky = "assets/skyboxes/" + environmentTypeStrings.at(m_sector) + "/";

	auto node = smgr->addSkyBoxSceneNode(
		assets->getTexture(std::string(sky + "up.jpg").c_str()),
		assets->getTexture(std::string(sky + "down.jpg").c_str()),
		assets->getTexture(std::string(sky + "left.jpg").c_str()),
		assets->getTexture(std::string(sky + "right.jpg").c_str()),
		assets->getTexture(std::string(sky + "forward.jpg").c_str()),
		assets->getTexture(std::string(sky + "back.jpg").c_str()));
	node->setID(ID_IsNotSelectable);
	node->setMaterialFlag(EMF_LIGHTING, false);
	node->setMaterialFlag(EMF_FOG_ENABLE, false);
	baedsLogger::log("Done.\n");
}

void MapRunner::m_setPlayerObjPositions()
{
	f32 out = std::min(m_objectiveData.getFloat("playerStartRadius"), m_maxObjGenRadius); //outer torus radius
	f32 torusRad = std::min(m_objectiveData.getFloat("objTorusRadius"), m_maxObjTorusRadius); //radius of the objective torus for generation
	f32 distBetweenTorus = std::min(m_objectiveData.getFloat("torusDistance"), m_maxTorusDistance);
	f32 in = out - torusRad;

	m_playerStartPos = getPointInTorus(vector3df(0, 0, 0), out, out + torusRad, randomDirectionalVector());
	std::cout << "Player start: " << vecStr(m_playerStartPos) << std::endl;

	u32 maxObjectives = m_objectiveData.getUint("maxObjectives");
	u32 minObjectives = m_objectiveData.getUint("minObjectives");
	u32 numObjectives = random.urange(minObjectives, maxObjectives);

	vector3df origin(0.f);
	vector3df directionToOrigin = (origin - m_playerStartPos).normalize();
	vector3df torusCenter = m_playerStartPos + (directionToOrigin * distBetweenTorus);

	for (u32 i = 0; i < numObjectives; ++i) {
		vector3df pos = getPointInTorus(torusCenter, in, out, directionToOrigin);

		m_objectivesStartPos.push_back(pos);
		torusCenter += (directionToOrigin * distBetweenTorus);
		out -= torusRad;
		in -= torusRad;
	}
}

void MapRunner::m_setPlayer()
{
	baedsLogger::log("Setting up the player and wingmen at position " + vecStr(m_playerStartPos) + "... ");
	auto rotation = (m_objectivesStartPos.front() - m_playerStartPos).getHorizontalAngle();

	flecs::entity player = createPlayerShip(m_playerStartPos, rotation);

	if (m_type == SCENARIO_DUMMY) {
		auto obj = new RemoveEntityObjective({ player });
		m_curObjective = std::shared_ptr<Objective>(obj);
	}
	auto n = player.get_mut<IrrlichtComponent>()->node;

	if (m_sector == SECTOR_RINGS) {
		auto spotlight = smgr->addLightSceneNode(n, vector3df(0, 0, 12.f), SColorf(.8f, .8f, .8f), 250.f, ID_IsNotSelectable);
		spotlight->setLightType(ELT_SPOT);
	}
	if (m_campaignMatch) { //if its a campaign load the wingmen
		if (m_type != SCENARIO_TUTORIAL) {
			for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
				if (!campaign->getAssignedWingman(i) || !campaign->getAssignedShip(i)) continue;
				vector3df wingpos = m_playerStartPos;
				if (i % 2 == 0) {
					wingpos.X += 20.f + (i * 20.f);
				}
				else {
					wingpos.X -= 20.f + (i * 20.f);
				}
				flecs::entity wingman = createWingman(i, player, wingpos, rotation);
				gameController->setWingman(wingman, i);
			}
		}
		if (m_type != SCENARIO_BOSSFIGHT && m_sector != SECTOR_FINALE && m_sector != SECTOR_RINGS && m_type != SCENARIO_ARNOLD) {
			StickySpawnerScenarioEffect* eff = new StickySpawnerScenarioEffect();
			setSpawns(eff->aceShipId, eff->shipId, eff->aceWepId, eff->wepId, eff->turretId, eff->turretWepId);
			eff->spawnInterval = 60.f;
			eff->curInterval = 0.f;
			eff->percentageChance = campaign->getSector()->getCurrentScenario()->detectionChance();
			addStatusEffectToEntity(eff, gameController->getPlayer());
		}

	}
	else {
		//load other players here
	}

	if (m_type == SCENARIO_SCRAMBLE || m_type == SCENARIO_BOSSFIGHT) {
		baedsLogger::log("Adding carrier... ");
		flecs::entity chaos = createChaosTheory(m_playerStartPos + (getNodeForward(n)*500.f) + (getNodeDown(n)*300.f), rotation);
		gameController->setChaosTheory(chaos);
		auto ai = chaos.get_mut<AIComponent>();
		ai->wingCommander = player;
		ai->onWing = true;
	}
	baedsLogger::log("Done.\n");
}

void MapRunner::m_setObstacles()
{
	baedsLogger::log("Setting up scenario obstacles for type " + environmentTypeStrings.at(m_sector) + "... \n");
	//if (m_type == SCENARIO_DUMMY) {
		//createAsteroid(vector3df(0), vector3df(0), vector3df(20.f), 20.f, 0.f, 0.f);
		//return;
	//}
	switch (m_sector) {
	case SECTOR_DEBRIS: {
		m_debris();
		break;
	}
	case SECTOR_ASTEROID: {
		m_asteroid();
		break;
	}
	case SECTOR_GAS: {
		m_gas();
		break;
	}
	case SECTOR_SUPPLY_DEPOT: {
		m_supplyDepot();
		break;
	}
	case SECTOR_RINGS: {
		m_rings();
		break;
	}
	case SECTOR_FLEET_GROUP: {
		m_fleet();
		break;
	}
	case SECTOR_FINALE: {
		m_finale();
		break;
	}
	default: {
		baedsLogger::log("No valid sector type.\n");
		break;
	}
	}//end switch
	baedsLogger::log("Done building obstacles.\n");
}

void MapRunner::m_setObjectives()
{
	baedsLogger::log("Setting up objectives.\n");
	if (m_type == SCENARIO_DUMMY) return;

	for (auto& pos : m_objectivesStartPos) {
		if (!m_bustSignals) break; // don't set this up for scenarios that dont have fake objectives
		if (pos == m_objectivesStartPos.back()) break; //don't set this up for the last one
		auto obj = new GoToPointObjective({});
		obj->setPoint(pos);
		obj->setRadioSpawn(pos);
		obj->setDistance(800.f);

		//set up bust objectives
		u32 roll = random.d100();
		baedsLogger::log("Bust objective: ");
		if (roll <= gameController->currentScenario->detectionChance()) {
			baedsLogger::log("Ambush");
			obj->setCompletionCb(onAmbushBustCb);
		}
		else if (roll <= 80) {
			baedsLogger::log("Complete bust");
			obj->setCompletionCb(onTotalBustCb);
		}
		else {
			baedsLogger::log("Salvage");
			if (m_type != SCENARIO_SALVAGE && m_type != SCENARIO_HARVEST) {
				u32 numSalvage = 3;
				for (u32 i = 0; i < numSalvage; ++i) {
					createSupplyBox(getPointInSphere(pos, 250.f), randomRotationVector(), vector3df(5.f, 5.f, 5.f));
				}
				obj->setCompletionCb(onSalvageBustCb);
			}
			else {
				baedsLogger::log("...but this mission has salvage -- total bust");
				obj->setCompletionCb(onTotalBustCb);
			}
		}
		baedsLogger::log("\n");

		m_objectives.push_back(std::shared_ptr<Objective>(obj));
	}
	baedsLogger::log("Setting up type " + objectiveTypeStrings.at(m_type) + "...\n");
	switch (m_type) {
	case SCENARIO_KILL_HOSTILES:
		m_dogfight();
		break;
	case SCENARIO_SCRAMBLE:
		m_scramble();
		break;
	case SCENARIO_DESTROY_OBJECT:
		m_destroy_object();
		break;
	case SCENARIO_RETRIEVE_POD:
		m_pod();
		break;
	case SCENARIO_SALVAGE:
		m_salvage();
		break;
	case SCENARIO_BRAWL:
		m_brawl();
		break;
	case SCENARIO_FRIGATE_RESCUE:
		m_frigate_rescue();
		break;
	case SCENARIO_CAPTURE:
		m_capture();
		break;
	case SCENARIO_SCUTTLE:
		m_scuttle();
		break;
	case SCENARIO_ROCK_IMPACT:
		m_rock_impact();
		break;
	case SCENARIO_EXTRACT_FUEL:
		m_extract();
		break;
	case SCENARIO_DISTRESS_BEACON:
		m_distress();
		break;
	case SCENARIO_ESCORT:
		m_escort();
		break;
	case SCENARIO_HARVEST:
		m_harvest();
		break;
	case SCENARIO_SEARCH_AND_DESTROY:
		m_search_and_destroy();
		break;
	case SCENARIO_ARNOLD:
		m_arnold();
		break;
	case SCENARIO_THEOD:
		m_theod();
		break;
	case SCENARIO_MI_CHA:
		m_lee();
		break;
	case SCENARIO_ARTHUR:
		m_arthur();
		break;
	case SCENARIO_SEAN:
		m_sean();
		break;
	case SCENARIO_TAURAN:
		m_tauran();
		break;
	case SCENARIO_JAMES:
		m_james();
		break;
	case SCENARIO_CAT:
		m_cat();
		break;

	case SCENARIO_BOSSFIGHT: {
		switch (m_sector) {
		case SECTOR_DEBRIS:
			m_debrisBoss();
			break;
		case SECTOR_ASTEROID:
			m_asteroidBoss();
			break;
		case SECTOR_GAS:
			m_gasBoss();
			break;
		case SECTOR_SUPPLY_DEPOT:
			m_supplyDepotBoss();
			break;
		case SECTOR_RINGS:
			m_ringsBoss();
			break;
		case SECTOR_FLEET_GROUP:
			m_fleetBoss();
			break;
		case SECTOR_FINALE:
			m_finaleBoss();
			break;
		default:
			m_debrisBoss();
			break;
		}
		
		break;
	}
	case SCENARIO_TUTORIAL:
		m_tutorial();
		break;
	default:
		m_dogfight();
		break;
	}

	if (m_sector == SECTOR_FINALE && m_type != SCENARIO_BOSSFIGHT) {
		baedsLogger::log("...and adding finale extraction...\n");
		vector3df origin(0.f);

		vector3df directionToOrigin = (origin - m_playerStartPos).normalize();
		f32 distBetweenTorus = std::min(m_objectiveData.getFloat("torusDistance"), m_maxTorusDistance);
		vector3df torusCenter = m_objectivesStartPos.back() + (directionToOrigin * distBetweenTorus);
		f32 out = std::min(m_objectiveData.getFloat("playerStartRadius"), m_maxObjGenRadius); //outer torus radius
		f32 torusRad = std::min(m_objectiveData.getFloat("objTorusRadius"), m_maxObjTorusRadius); //radius of the objective torus for generation
		f32 in = out - torusRad;

		vector3df pos = getPointInTorus(torusCenter, in, out, directionToOrigin);
		m_objectivesStartPos.push_back(pos);
		auto obj = new GoToPointObjective({});
		obj->setPoint(pos);
		obj->setRadioSpawn(pos);
		obj->setDistance(400.f);
		obj->setCompletionCb(onFinaleHoldoutCb);
		m_objectives.push_back(ObjElem(obj));

		auto obj2 = new ProtectObjective({ gameController->getPlayer() }, 45.f);
		obj2->setRadioSpawn(pos);
		m_objectives.push_back(ObjElem(obj2));
		m_cleanup = false;
	}

	m_curObjective = m_objectives.front();
	if(m_sector != SECTOR_RINGS && m_type != SCENARIO_TUTORIAL) 
		m_spawnRegWing(m_objectivesStartPos.back() + vector3df(0, -250.f, 0), vector3df(0, 0, 0));
	baedsLogger::log("Done with objective setup.\n");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//environments

//use InnerRad to define the radius of the capsule
static std::vector<vector3df> _pointsGet(vector3df start, std::vector<vector3df>& tooclose, f32 toocloserad, 
	f32 innerRad, f32 outerRad, u32 num, bool cylinder=false, vector3df cylinderEnd=vector3df(0,0,0))
{
	std::vector<vector3df> points;
	for (u32 i = 0; i < num; ++i) {
		vector3df pos;
		if (cylinder) pos = getPointInCapsule(start, cylinderEnd, innerRad);
		else pos = getPointInShell(start, innerRad, outerRad);
		while (isPointCloseToList(pos, tooclose, toocloserad)) {
			if (cylinder) pos = getPointInCapsule(start, cylinderEnd, innerRad);
			else pos = getPointInShell(start, innerRad, outerRad);
		}
		tooclose.push_back(pos);
		points.push_back(pos);
	}

	return points;
}

static bool _safeToAdd(dataId obstacleId, vector3df pos, vector3df rot, vector3df scale)
{
	bool safe = true;
	auto shape = btConvexHullShape(assets->getHull(obstacleData[obstacleId]->name));
	shape.setLocalScaling(irrVecToBt(scale));

	btPairCachingGhostObject ghost; 
	btTransform transform;
	transform.setIdentity();
	btQuaternion q;
	vector3df eulerRot = rot * DEGTORAD;
	q.setEulerZYX(eulerRot.Z, eulerRot.Y, eulerRot.X);
	transform.setRotation(q);
	
	transform.setOrigin(irrVecToBt(pos));
	ghost.setCollisionShape(&shape);
	ghost.setWorldTransform(transform);

	bWorld->addCollisionObject(&ghost);
	if (ghost.getNumOverlappingObjects() > 0) safe = false;
	bWorld->removeCollisionObject(&ghost);

	return safe;
}

void MapRunner::m_debris()
{
	//near-objective obstacles
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {
		auto debrisPos = _pointsGet(m_objectivesStartPos[i], m_obstaclePositions, 250.f, m_objectiveDeadZone, m_objectiveFieldRadius, m_obstaclesAroundObjective);
		for (auto& vec : debrisPos) {
			createRandomShipDebris(vec, randomRotationVector(), vector3df(random.frange(10.f, 70.f)));
		}
	}
	baedsLogger::log("Debris near objective generated. Number: " + std::to_string(m_obstaclesAroundObjective) + ", radius: " + std::to_string(m_objectiveFieldRadius) + "\n");

	vector3df corridorStart = m_playerStartPos;

	//corridor obstacles
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {
		baedsLogger::log("Building corridor from " + vecStr(corridorStart) + " to " + vecStr(m_objectivesStartPos[i]) + "\n");
		m_corridors.push_back({ corridorStart, m_objectivesStartPos[i] });
		f32 corridorLength = (m_objectivesStartPos[i] - corridorStart).getLength();

		u32 numObstacles = (u32)(corridorLength / 1000.f) * m_obstDensityPerKm;
		baedsLogger::log("Number of obstacles: " + std::to_string(numObstacles) + "\n");
		auto debrisPos = _pointsGet(corridorStart, m_obstaclePositions, 300.f, m_corridorRadius, 0.f, numObstacles, true, m_objectivesStartPos[i]);
		for (auto& vec : debrisPos) {
			createRandomShipDebris(vec, randomRotationVector(), vector3df(random.frange(10.f, 70.f)));
		}
		corridorStart = m_objectivesStartPos[i];
	}

	//loose obstacles
	baedsLogger::log("Generating loose objects...\n");
	for (u32 i = 0; i < m_looseObstacles; ++i) {
		auto pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		f32 scale = random.frange(10.f, 70.f);

		while (isPointCloseToList(pos, m_obstaclePositions, 400.f)) pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		createRandomShipDebris(pos, randomRotationVector(), vector3df(scale, scale, scale));
		m_obstaclePositions.push_back(pos);
	}
	if(m_type != SCENARIO_BOSSFIGHT && m_type != SCENARIO_TUTORIAL) m_debrisRoadblocks();
}

void MapRunner::m_debrisRoadblocks()
{
	//add encounters to the fields
	u32 numEncounters = 4;
	baedsLogger::log("Setting up roadblocks...\n");
	baedsLogger::log("Number: " + std::to_string(numEncounters) + ", corridors: " + std::to_string(m_corridors.size()) + "\n");

	for (u32 i = 0; i < m_corridors.size(); ++i) {
		for (u32 j = 0; j < numEncounters; ++j) {
			vector3df dir = m_corridors[i].end - m_corridors[i].start;
			f32 len = dir.getLength();
			dir.normalize();
			vector3df linepoint = (random.frange(0.f, 1.f) * len * dir) + m_corridors[i].start;

			u32 roll = random.d100();
			baedsLogger::log("Roll: " + std::to_string(roll) + ", ");
			//decide what encounter it is - turrets, mines, supplies, enemy ships
			if (roll >= 65) {
				//enemy ships
				baedsLogger::log("Fighter wing\n");
				auto ships = m_spawnScoutWing(getPointInSphere(linepoint, m_corridorRadius), randomRotationVector());
				auto cdrai = ships.front().get_mut<AIComponent>();
				cdrai->route.push_back(irrVecToBt(linepoint));
				cdrai->route.push_back(irrVecToBt(m_corridors[i].start));
				cdrai->route.push_back(irrVecToBt(m_corridors[i].end + vector3df(0.f, 450.f, 0.f)));
				cdrai->route.push_back(irrVecToBt(linepoint));
				cdrai->fixedPatrolRoute = true;
			}
			else if (roll >= 45) {
				//mines
				u32 numMines = 45;
#ifdef _DEBUG
				numMines /= 4;
#endif
				baedsLogger::log("Mines\n");
				for (u32 k = 0; k < numMines; ++k) {
					vector3df pos = getPointInDisc(linepoint, m_corridorRadius, dir);
					while (isPointCloseToList(pos, m_obstaclePositions, 125.f)) pos = getPointInDisc(linepoint, m_corridorRadius, dir);
					m_obstaclePositions.push_back(pos);
					createMine(pos, randomRotationVector(), vector3df(8.f, 8.f, 8.f));
				}
			}
			else if (roll > 25) {
				//missiles
				u32 numMissiles = 35;
#ifdef _DEBUG
				numMissiles /= 3;
#endif
				baedsLogger::log("Missiles\n");
				for (u32 k = 0; k < numMissiles; ++k) {
					vector3df pos = getPointInDisc(linepoint, m_corridorRadius, dir);
					while (isPointCloseToList(pos, m_obstaclePositions, 125.f)) pos = getPointInDisc(linepoint, m_corridorRadius, dir);
					m_obstaclePositions.push_back(pos);
					createDebrisMissile(pos, randomRotationVector(), vector3df(8.f, 8.f, 8.f));
				}
			}
			else {
				//malfunctioning turrets
				baedsLogger::log("Malfunctioning turrets\n");
				u32 numTurrets = 25;
#ifdef _DEBUG
				numTurrets /= 3;
#endif
				for (u32 k = 0; k < numTurrets; ++k) {
					vector3df pos = getPointInDisc(linepoint, m_corridorRadius, dir);
					while (isPointCloseToList(pos, m_obstaclePositions, 125.f)) pos = getPointInDisc(linepoint, m_corridorRadius, dir);
					m_obstaclePositions.push_back(pos);
					createDebrisTurret(pos, randomRotationVector(), vector3df(6.f, 6.f, 6.f));
				}
			}
		}
	}
}

void MapRunner::m_asteroid()
{
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {
		auto roidPos = _pointsGet(m_objectivesStartPos[i], m_obstaclePositions, 250.f, m_objectiveDeadZone, m_objectiveFieldRadius, m_obstaclesAroundObjective);
		for (auto& vec : roidPos) {
			f32 scale = random.frange(10.f, 70.f);
			createAsteroid(vec, randomRotationVector(), vector3df(scale, scale, scale), scale, 0, .4f);
		}
	}
	baedsLogger::log("Obstacles near objective generated. Number: " + std::to_string(m_obstaclesAroundObjective) + ", radius: " + std::to_string(m_objectiveFieldRadius) + "\n");

	vector3df corridorStart = m_playerStartPos;

	//createStaticMeshCloud(m_playerStartPos, randomRotationVector(), vector3df(5, 5, 5));

	//corridor obstacles
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {
		baedsLogger::log("Building corridor from " + vecStr(corridorStart) + " to " + vecStr(m_objectivesStartPos[i]) + "\n");
		m_corridors.push_back({ corridorStart, m_objectivesStartPos[i] });
		f32 corridorLength = (m_objectivesStartPos[i] - corridorStart).getLength();

		u32 numObstacles = (u32)(corridorLength / 1000.f) * m_obstDensityPerKm;
		baedsLogger::log("Number of obstacles: " + std::to_string(numObstacles) + "\n");
		auto debrisPos = _pointsGet(corridorStart, m_obstaclePositions, 300.f, m_corridorRadius, 0.f, numObstacles, true, m_objectivesStartPos[i]);

		//generate the rocks in the corridor
		for (auto& vec : debrisPos) {
			f32 scale = random.frange(40.f, 100.f);
			vector3df rotation = (m_objectivesStartPos[i] - corridorStart).getHorizontalAngle();
			if (random.coinflip()) rotation = -rotation;
			//fuzz up the vector a bit
			makeVectorInaccurate(rotation, .2f);

			//figure out some rolls for unique asteroids
			u32 roll = random.d10();
			if (roll == 10) createIceAsteroid(vec, rotation, vector3df(scale / 4.f, scale / 4.f, scale / 4.f), 25.f, .4f);
			if (roll == 9) createMoneyAsteroid(vec, rotation, vector3df(scale / 3.5f), 40.f, .8f);
			else createAsteroid(vec, rotation, vector3df(scale, scale, scale), scale, 55.f, .4f);
		}

		//add some fuel stations throughout the corridor
		u32 fuelstations = 2;
		debrisPos = _pointsGet(corridorStart, m_obstaclePositions, 400.f, m_corridorRadius, 0, fuelstations, true, m_objectivesStartPos[i]);
		for (auto& vec : debrisPos) {
			vector3df rotation = randomRotationVector();
			createModularStationFromFile(vec, rotation, "fuel_station.xml");
		}
		corridorStart = m_objectivesStartPos[i];
	}

	//loose obstacles
	baedsLogger::log("Generating loose objects...\n");
	for (u32 i = 0; i < m_looseObstacles; ++i) {
		auto pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		f32 scale = random.frange(10.f, 70.f);

		while (isPointCloseToList(pos, m_obstaclePositions, 400.f)) pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		createAsteroid(pos, randomRotationVector(), vector3df(scale, scale, scale), scale, 180.f, 1.f);
		m_obstaclePositions.push_back(pos);
	}
	if (m_type != SCENARIO_BOSSFIGHT) {
		m_asteroidRoadblocks();
	}
}

void MapRunner::m_asteroidRoadblocks()
{
	for (const auto& corr : m_corridors) {
		//dust clouds
		u32 dustclouds = 4;
		vector3df dir = corr.end - corr.start;
		f32 len = dir.getLength();
		dir.normalize();
		u32 numCloudsPerDustBlocker = 18;
#ifdef _DEBUG
		dustclouds = 2;
		numCloudsPerDustBlocker = 12;
#endif
		for (u32 i = 0; i < dustclouds; ++i) {
			vector3df linepoint = (random.frange(0.f, 1.f) * len * dir) + corr.start;
			SColor inner = SColor(20, 150, 150, 150);
			SColor outer = SColor(1, 100, 94, 64);

			for (u32 j = 0; j < numCloudsPerDustBlocker; ++j) {
				f32 scale = random.frange(25.f, 60.f);
				vector3df pos = getPointInDisc(linepoint, m_corridorRadius, dir);
				createDustCloud(pos, vector3df(scale, scale, scale));
			}
		}
		//patrolling alien ships
		vector3df linepoint = (random.frange(0.f, 1.f) * len * dir) + corr.start;

		auto ships = m_spawnScoutWing(getPointInSphere(linepoint, m_corridorRadius), randomRotationVector());
		auto cdrai = ships.front().get_mut<AIComponent>();
		cdrai->route.push_back(irrVecToBt(linepoint));
		cdrai->route.push_back(irrVecToBt(corr.start));
		cdrai->route.push_back(irrVecToBt(corr.end + vector3df(0.f, 450.f, 0.f)));
		cdrai->route.push_back(irrVecToBt(linepoint));
		cdrai->fixedPatrolRoute = true;
	}
}

void MapRunner::m_gas()
{
	//obstacles around the objective
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		auto objectiveObstaclesPositions = _pointsGet(m_objectivesStartPos[i], m_obstaclePositions, 250.f, m_objectiveDeadZone, m_objectiveFieldRadius, m_obstaclesAroundObjective);

		for (auto& vec : objectiveObstaclesPositions) {
			//add crap around the objectives
			u32 cloudsPerClump = 11U;
#ifdef _DEBUG
			cloudsPerClump = 7U;
#endif
			std::vector<vector3df> clouds;
			for (u32 j = 0; j < cloudsPerClump; ++j) {
				clouds.push_back(getPointInSphere(vec, 800.f));
			}
			createCloudFormation(clouds);
		}
	}

	vector3df corridorStart = m_playerStartPos;

	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		baedsLogger::log("Building corridor from " + vecStr(corridorStart) + " to " + vecStr(m_objectivesStartPos[i]) + "\n");

		//get the points in the corridor and add the corridor to the start
		m_corridors.push_back({ corridorStart, m_objectivesStartPos[i] });
		f32 corridorLength = (m_objectivesStartPos[i] - corridorStart).getLength();
		u32 numObstacles = (u32)(corridorLength / 1000.f) * m_obstDensityPerKm;
		baedsLogger::log("Number of obstacles: " + std::to_string(numObstacles) + "\n");

		auto corridorObstaclesPositions = _pointsGet(corridorStart, m_obstaclePositions, 300.f, m_corridorRadius, 0.f, numObstacles, true, m_objectivesStartPos[i]);

		//generate the things in the corridor
		vector3df station1(0, 0, 0), station2(0, 0, 0), cloud1(0, 0, 0), cloud2(0, 0, 0);
		for (auto& vec : corridorObstaclesPositions) {
			if (station1 == vector3df(0, 0, 0)) station1 = vec;
			else if (station2 == vector3df(0, 0, 0)) station2 = vec;
			else if (cloud1 == vector3df(0, 0, 0)) cloud1 = vec;
			else if (cloud2 == vector3df(0, 0, 0)) cloud2 = vec;

			u32 cloudsPerClump = 6U;
			std::vector<vector3df> clouds;
			for (u32 j = 0; j < cloudsPerClump; ++j) {
				clouds.push_back(getPointInSphere(vec, 800.f));
			}
			createCloudFormation(clouds);
		}
		if (m_type != SCENARIO_BOSSFIGHT) {
			baedsLogger::log("Station 1 location: " + vecStr(station1) + ", route cloud: " + vecStr(cloud1) + "\n");
			baedsLogger::log("Station 2 location: " + vecStr(station2) + ", route cloud: " + vecStr(cloud2) + "\n");

			createModularStationFromFile(station1, randomRotationVector(), "alien_gunplatform.xml");
			createModularStationFromFile(station2, randomRotationVector(), "alien_gunplatform.xml");
			auto gasdrone1 = spawnShipWing(cloud1, randomRotationVector(), 2, 22, 0, 22, 0, 1, 28);
			auto gasdrone2 = spawnShipWing(cloud2, randomRotationVector(), 2, 22, 0, 22, 0, 1, 28);
			for (auto& ent : gasdrone1) {
				auto ai = ent.get_mut<AIComponent>();
				ai->fixedPatrolRoute = true;
				ai->route.push_back(irrVecToBt(station1) + btVector3(0,500.f,0));
				ai->route.push_back(irrVecToBt(cloud1));
				ai->state = AI_STATE_PATROL;
			}
			for (auto& ent : gasdrone2) {
				auto ai = ent.get_mut<AIComponent>();
				ai->fixedPatrolRoute = true;
				ai->route.push_back(irrVecToBt(station2) + btVector3(0, 500.f, 0));
				ai->route.push_back(irrVecToBt(cloud2));
				ai->state = AI_STATE_PATROL;

			}
		}
		//do any other stuff you want to add to the corridor
		corridorStart = m_objectivesStartPos[i];
	}

	//finish up with loose obstacles
	baedsLogger::log("Generating loose objects...\n");
	for (u32 i = 0; i < m_looseObstacles; ++i) {
		auto pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		while (isPointCloseToList(pos, m_obstaclePositions, 400.f)) pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);

		m_obstaclePositions.push_back(pos);
	}
	if (m_type != SCENARIO_BOSSFIGHT) {
		m_gasRoadblocks();
	}
}

void MapRunner::m_gasRoadblocks()
{
	StickySpawnerScenarioEffect* eff = new StickySpawnerScenarioEffect();
	setSpawns(eff->aceShipId, eff->shipId, eff->aceWepId, eff->wepId, eff->turretId, eff->turretWepId);
	eff->spawnInterval = 120.f;
	eff->curInterval = 105.f;
	addStatusEffectToEntity(eff, gameController->getPlayer());

	for (const auto& corr : m_corridors) {
		//figure out what to put in as roadblocks
		u32 numAnomalies = 6U;
		auto anomalies = _pointsGet(corr.start, m_obstaclePositions, 400.f, m_corridorRadius, 0, numAnomalies, true, corr.end);

		for (auto& pos : anomalies) {
			f32 scalenum = random.frange(60, 80.f);
			while (isPointCloseToList(pos, anomalies, scalenum * 11.2f)) {
				pos *= 1.2f; //shimmy it further and further away until it's not hitting anything
			}

			createGravityAnomaly(pos, vector3df(scalenum));
		}
		vector3df dir = corr.end - corr.start;
		f32 len = dir.getLength();
		dir.normalize();

		u32 numWings = 2;
		for (u32 i = 0; i < numWings; ++i) {

			vector3df linepoint = (random.frange(0.f, 1.f) * len * dir) + corr.start;
			if(m_type != SCENARIO_BOSSFIGHT) {
				auto ships = m_spawnScoutWing(getPointInSphere(linepoint, m_corridorRadius), randomRotationVector());
				auto cdrai = ships.front().get_mut<AIComponent>();
				vector3df top(0, m_corridorRadius, 0);
				cdrai->route.push_back(irrVecToBt(corr.start));
				cdrai->route.push_back(irrVecToBt(corr.end));
				cdrai->fixedPatrolRoute = true;
			}
		}

		u32 numDisabledShips = 3;
		for (u32 i = 0; i < numDisabledShips; ++i) {
			vector3df linepoint = (random.frange(0.f, 1.f) * len * dir) + corr.start;
			auto vec = getPointInSphere(linepoint, m_corridorRadius);
			createDeadShipAsObstacle(12, vec, randomRotationVector(), 3);
			u32 cratesNearby = 3;
			for (u32 j = 0; j < cratesNearby; ++j) {
				createSupplyBox(getPointInShell(vec, 200.f, 400.f), randomRotationVector(), vector3df(random.frange(5.f, 10.f)));
			}
			u32 turrets = 2;
			for (u32 j = 0; j < turrets; ++j) {
				createDebrisTurret(getPointInShell(vec, 400.f, 500.f), randomRotationVector(), vector3df(7.f));
			}
		}
	}
}

void MapRunner::m_supplyDepot()
{
	//obstacles around the objective
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		auto objectiveObstaclesPositions = _pointsGet(m_objectivesStartPos[i], m_obstaclePositions, 250.f, m_objectiveDeadZone, m_objectiveFieldRadius, m_obstaclesAroundObjective);

		u32 numDerelicts = m_obstaclesAroundObjective / 2.f;
		u32 numDebris = m_obstaclesAroundObjective - numDerelicts;

		for (u32 j = 0; j < numDerelicts; ++j) {
			u32 roll = random.d100();
			if (roll > 80) createDeadShipAsObstacle(12, objectiveObstaclesPositions[j], randomRotationVector(), 3);
			else if (roll > 50) createDeadShipAsObstacle(19, objectiveObstaclesPositions[j], randomRotationVector(), 3);
			else if (roll > 40) createDeadShipAsObstacle(0, objectiveObstaclesPositions[j], randomRotationVector(), 3);
			else if (roll > 30) createDeadShipAsObstacle(2, objectiveObstaclesPositions[j], randomRotationVector(), 3);
			else if (roll > 20) createDeadShipAsObstacle(11, objectiveObstaclesPositions[j], randomRotationVector(), 3);
			else if (roll > 10) createDeadShipAsObstacle(6, objectiveObstaclesPositions[j], randomRotationVector(), 1);

		}
		for (u32 j = numDerelicts; j < numDerelicts + numDebris; ++j) {
			f32 scale = random.frange(15.f, 25.f);
			createLooseHumanModule(objectiveObstaclesPositions[j], randomRotationVector(), vector3df(scale));
		}
	}

	vector3df corridorStart = m_playerStartPos;

	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		baedsLogger::log("Building corridor from " + vecStr(corridorStart) + " to " + vecStr(m_objectivesStartPos[i]) + "\n");

		//get the points in the corridor and add the corridor to the start
		m_corridors.push_back({ corridorStart, m_objectivesStartPos[i] });
		f32 corridorLength = (m_objectivesStartPos[i] - corridorStart).getLength();

		u32 numObstacles = (u32)(corridorLength / 1000.f) * m_obstDensityPerKm;
		baedsLogger::log("Number of obstacles: " + std::to_string(numObstacles) + "\n");

		/*
		vector3df corridorDirection = (m_objectivesStartPos[i] - corridorStart).normalize();
		vector3df top = getPointInDisc(corridorStart, m_corridorRadius, corridorDirection);
		vector3df upDir = (top - corridorStart).normalize();

		vector3df bot = corridorStart + (upDir * (m_corridorRadius / 2.f));
		top = corridorStart + (-upDir * (m_corridorRadius / 2.f));
		f32 beaconInterval = 1800.f;
		u32 numBeacons = (u32)(corridorLength / beaconInterval);

		for (u32 j = 0; j < numBeacons; ++j) {
			createBeacon(top, randomRotationVector(), vector3df(10.f));
			createBeacon(bot, randomRotationVector(), vector3df(10.f));
			top += (corridorDirection * beaconInterval);
			bot += (corridorDirection * beaconInterval);
		}
		*/

		auto corridorObstaclesPositions = _pointsGet(corridorStart, m_obstaclePositions, 300.f, m_corridorRadius, 0.f, numObstacles, true, m_objectivesStartPos[i]);

		//generate the things in the corridor
		for (auto& vec : corridorObstaclesPositions) {

			u32 roll = random.d100();
			if (roll > 95) {
				createDeadShipAsObstacle(12, vec, randomRotationVector(), 3);
				u32 cratesNearby = 2;
				for (u32 j = 0; j < cratesNearby; ++j) {
					 createSupplyBox(getPointInShell(vec, 200.f, 500.f), randomRotationVector(), vector3df(random.frange(5.f,10.f)));
				}
			}
			else if (roll > 80) {
				//createDeadShipAsObstacle(21, vec, randomRotationVector(), 3);
				createRandomShipDebris(vec, randomRotationVector(), vector3df(random.frange(10.f, 30.f)));
			}
			else if (roll > 65) {
				f32 scale = random.frange(15.f, 25.f);
				createLooseHumanModule(vec, randomRotationVector(), vector3df(scale));
				//createCaptureableWepPlatform(vec, randomRotationVector(), 0, 16);
			}
			else {
				f32 scale = random.frange(15.f, 25.f);
				createLooseHumanModule(vec, randomRotationVector(), vector3df(scale));
			}

		}
		//do any other stuff you want to add to the corridor
		corridorStart = m_objectivesStartPos[i];
	}

	m_supplyDepotRoadblocks();
}

void MapRunner::m_supplyDepotRoadblocks()
{
	for (u32 i = 0; i < m_corridors.size(); ++i) {
		//figure out what to put in as roadblocks
	}
}

void MapRunner::m_rings()
{
	//obstacles around the objective
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		auto objectiveObstaclesPositions = _pointsGet(m_objectivesStartPos[i], m_obstaclePositions, 250.f, m_objectiveDeadZone, m_objectiveFieldRadius, m_obstaclesAroundObjective);

		for (auto& vec : objectiveObstaclesPositions) {
			f32 scale = random.frange(10.f, 70.f);
			if (random.percentChance(10.f))	createDormantRockShip(vec, randomRotationVector());
			else createAsteroid(vec, randomRotationVector(), vector3df(scale), scale, 0, .4f);
		}
	}

	vector3df corridorStart = m_playerStartPos;

	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		baedsLogger::log("Building corridor from " + vecStr(corridorStart) + " to " + vecStr(m_objectivesStartPos[i]) + "\n");

		//get the points in the corridor and add the corridor to the start
		m_corridors.push_back({ corridorStart, m_objectivesStartPos[i] });
		f32 corridorLength = (m_objectivesStartPos[i] - corridorStart).getLength();

		u32 numObstacles = (u32)(corridorLength / 1000.f) * m_obstDensityPerKm;
		baedsLogger::log("Number of obstacles: " + std::to_string(numObstacles) + "\n");

		auto corridorObstaclesPositions = _pointsGet(corridorStart, m_obstaclePositions, 300.f, m_corridorRadius, 0.f, numObstacles, true, m_objectivesStartPos[i]);

		//generate the things in the corridor
		for (auto& vec : corridorObstaclesPositions) {
			f32 scale = random.frange(45.f, 80.f);
			u32 roll = random.d10();
			vector3df rotation = randomRotationVector();
			if (roll == 10) createIceAsteroid(vec, rotation, vector3df(scale / 4.f), 25.f, .4f);
			if (roll == 9) createMoneyAsteroid(vec, rotation, vector3df(scale / 3.5f), 40.f, .8f);
			else createAsteroid(vec, randomRotationVector(), vector3df(scale), scale, 0, .4f);

		}
		//do any other stuff you want to add to the corridor
		corridorStart = m_objectivesStartPos[i];
	}

	//finish up with loose obstacles
	baedsLogger::log("Generating loose objects...\n");
	for (u32 i = 0; i < m_looseObstacles; ++i) {
		auto vec = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		while (isPointCloseToList(vec, m_obstaclePositions, 400.f)) vec = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		f32 scale = random.frange(10.f, 70.f);
		createAsteroid(vec, randomRotationVector(), vector3df(scale), scale, 0, .4f);

		m_obstaclePositions.push_back(vec);
	}
	m_ringsRoadblocks();
}

void MapRunner::m_ringsRoadblocks()
{
	for (u32 i = 0; i < m_corridors.size(); ++i) {

		u32 rockEncounters = 2;
		for (u32 j = 0; j < rockEncounters; ++j) {
			vector3df dir = m_corridors[i].end - m_corridors[i].start;
			f32 len = dir.getLength();
			dir.normalize();
			vector3df linepoint = (random.frange(0.f, 1.f) * len * dir) + m_corridors[i].start;

			auto rock = createDormantRockShip(getPointInDisc(linepoint, m_corridorRadius, dir), randomRotationVector());
			rock = createDormantRockShip(getPointInDisc(linepoint, m_corridorRadius, dir), randomRotationVector());
			rock = createDormantRockShip(getPointInDisc(linepoint, m_corridorRadius/4, dir), randomRotationVector());
		}
	}
}

const u32 TOTAL_HUMAN_FIGHTERS = 5;
const u32 TOTAL_HUMAN_FRIGATES = 2;
const dataId HUMAN_FIGHTER_SHIPS[TOTAL_HUMAN_FIGHTERS] = {0, 11, 20, 18, 2};
const dataId HUMAN_FRIGATE_SHIPS[TOTAL_HUMAN_FRIGATES] = {12, 19};
dataId _randHumanFighter() {
	return HUMAN_FIGHTER_SHIPS[random.unumEx(TOTAL_HUMAN_FIGHTERS)];
}
dataId _randHumanFrigate() {
	return HUMAN_FRIGATE_SHIPS[random.unumEx(TOTAL_HUMAN_FRIGATES)];
}

const u32 TOTAL_ALIEN_FIGHTERS = 5;
const u32 TOTAL_ALIEN_FRIGATES = 2;
const dataId ALIEN_FIGHTER_SHIPS[TOTAL_ALIEN_FIGHTERS] = {3, 1, 10, 13, 4};
const dataId ALIEN_FRIGATE_SHIPS[TOTAL_ALIEN_FRIGATES] = {8, 6};
dataId _randAlienFighter() {
	return ALIEN_FIGHTER_SHIPS[random.unumEx(TOTAL_ALIEN_FIGHTERS)];
}
dataId _randAlienFrigate() {
	return ALIEN_FRIGATE_SHIPS[random.unumEx(TOTAL_ALIEN_FRIGATES)];
}

const dataId FRIGATE_RAILGUN = 16;
const dataId CAUSALITY_RAILGUN = 23;
const dataId UPGRADED_TSUNAMI = 26;
const dataId UPGRADED_PLASMA = 25;
const dataId UPGRADED_PLASMA_SNIPER = 24;

void MapRunner::m_fleet()
{
	vector3df rotation = (m_objectivesStartPos.front() - m_playerStartPos).getHorizontalAngle();
	vector3df up, right, forward;
	forward = rotation.rotationToDirection(vector3df(0, 0, 1));
	up = rotation.rotationToDirection(vector3df(0, 1, 0));
	right = rotation.rotationToDirection(vector3df(1, 0, 0));

	vector3df corridorStart = m_playerStartPos;
	vector3df causalityPos = m_playerStartPos + (up * 1000.f);
	vector3df chaosTheoryPos = m_playerStartPos + (-up * 300.f);
	vector3df baseFighterPos = (m_playerStartPos + (forward * 500.f));

	if (m_type == SCENARIO_BOSSFIGHT) {
		causalityPos += (forward * 600.f);
		baseFighterPos += forward * 600.f;
	}
	flecs::entity causality = INVALID_ENTITY;
	if (m_type != SCENARIO_BOSSFIGHT) {
		causality = createHumanCarrier(21, causalityPos, rotation, CAUSALITY_RAILGUN, 0, UPGRADED_TSUNAMI);
		gameController->setChaosTheory(createChaosTheory(chaosTheoryPos, rotation));
	}
	else {
		createHumanCarrier(12, causalityPos, rotation, FRIGATE_RAILGUN, 0, UPGRADED_TSUNAMI);
		createHumanCarrier(12, chaosTheoryPos, rotation, FRIGATE_RAILGUN, 0, UPGRADED_TSUNAMI);
	}

	vector3df humanPos[3] = { causalityPos + (right * 1000.f), causalityPos + (-right * 1000.f), causalityPos + (up * 800.f)};
	for (u32 i = 0; i < 3; ++i) {
		auto ent = createHumanCarrier(12, humanPos[i], rotation, FRIGATE_RAILGUN, 0, UPGRADED_TSUNAMI);
	}

	vector3df fighterPos[3] = { baseFighterPos + (right * 300.f), baseFighterPos + (-right * 300.f)};
	for (u32 i = 0; i < 3; ++i) {
		spawnShipWing(fighterPos[i], rotation, getCurAiNum(), _randHumanFighter(), UPGRADED_TSUNAMI, _randHumanFighter(), UPGRADED_TSUNAMI, 0, 3, true);
	}

	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		baedsLogger::log("Building corridor from " + vecStr(corridorStart) + " to " + vecStr(m_objectivesStartPos[i]) + "\n");

		//get the points in the corridor and add the corridor to the start
		m_corridors.push_back({ corridorStart, m_objectivesStartPos[i] });
		f32 corridorLength = (m_objectivesStartPos[i] - corridorStart).getLength();
		u32 numObstacles = (u32)(corridorLength / 1000.f) * m_obstDensityPerKm;
		auto corridorObstaclesPositions = _pointsGet(corridorStart, m_obstaclePositions, 300.f, m_corridorRadius, 0.f, numObstacles, true, m_objectivesStartPos[i]);

		u32 totalShips = 0;
		//generate the things in the corridor
		for (auto& vec : corridorObstaclesPositions) {
			createRandomShipDebris(vec, randomRotationVector(), vector3df(random.frange(40.f, 90.f)));
		}

		corridorStart = m_objectivesStartPos[i];
	}

	//finish up with loose obstacles
	baedsLogger::log("Generating loose objects...\n");
	for (u32 i = 0; i < m_looseObstacles; ++i) {
		auto pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		while (isPointCloseToList(pos, m_obstaclePositions, 400.f)) pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);

		m_obstaclePositions.push_back(pos);
	}
	m_fleetRoadblocks();
}

void MapRunner::m_fleetRoadblocks()
{
	for (u32 i = 0; i < m_corridors.size(); ++i) {
		//figure out what to put in as roadblocks
	}
}

void MapRunner::m_finale()
{
	//obstacles around the objective
	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		auto objectiveObstaclesPositions = _pointsGet(m_objectivesStartPos[i], m_obstaclePositions, 350.f, m_objectiveDeadZone, m_objectiveFieldRadius, m_obstaclesAroundObjective);

		for (auto& vec : objectiveObstaclesPositions) {
			if (random.d4() == 4) createIceAsteroid(vec, randomRotationVector(), vector3df(random.frange(15.f, 55.f)), 35.f, .6f);
			else createIceSpikeAsteroid(vec, randomRotationVector(), vector3df(random.frange(35.f, 75.f)), 3.f, .05f);
		}
	}

	vector3df corridorStart = m_playerStartPos;

	for (u32 i = 0; i < m_objectivesStartPos.size(); ++i) {

		baedsLogger::log("Building corridor from " + vecStr(corridorStart) + " to " + vecStr(m_objectivesStartPos[i]) + "\n");

		//get the points in the corridor and add the corridor to the start
		m_corridors.push_back({ corridorStart, m_objectivesStartPos[i] });
		f32 corridorLength = (m_objectivesStartPos[i] - corridorStart).getLength();

		u32 numObstacles = (u32)(corridorLength / 1000.f) * m_obstDensityPerKm;
		baedsLogger::log("Number of obstacles: " + std::to_string(numObstacles) + "\n");

		auto corridorObstaclesPositions = _pointsGet(corridorStart, m_obstaclePositions, 425.f, m_corridorRadius, 0.f, numObstacles, true, m_objectivesStartPos[i]);

		//generate the things in the corridor

		std::vector<vector3df> gravAnomalyPositions;

		for (auto& vec : corridorObstaclesPositions) {
			u32 roll = random.d6();
			if (roll == 6) {
				f32 scale = random.frange(45.f, 65.f);
				while (isPointCloseToList(vec, gravAnomalyPositions, scale * 11.2f)) {
					vec *= 1.2f; //shimmy it further and further away until it's not hitting anything
				}
				createGravityAnomaly(vec, vector3df(scale));
				gravAnomalyPositions.push_back(vec);
			}
			else if (roll == 5) {
				u32 clumpSize = 5U;
				std::vector<vector3df> positions;
				for (u32 j = 0; j < clumpSize; ++j) {
					positions.push_back(getPointInSphere(vec, 300.f));
				}
				createCloudFormation(positions);
			}
			else if (roll == 4) {
				createRadioactiveAsteroid(vec, randomRotationVector(), vector3df(random.frange(55.f, 65.f)), 18.f, .2f);
			}
			else {
				if (random.d4()==4) createIceAsteroid(vec, randomRotationVector(), vector3df(random.frange(45.f, 95.f)), 35.f, .6f);
				else createIceSpikeAsteroid(vec, randomRotationVector(), vector3df(random.frange(45.f, 105.f)), 3.f, .05f);
			}
		}
		//do any other stuff you want to add to the corridor
		corridorStart = m_objectivesStartPos[i];
	}

	//finish up with loose obstacles
	baedsLogger::log("Generating loose objects...\n");
	for (u32 i = 0; i < m_looseObstacles; ++i) {
		auto pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);
		while (isPointCloseToList(pos, m_obstaclePositions, 400.f)) pos = getPointInSphere(vector3df(0, 0, 0), 15000.f);

		createIceAsteroid(pos, randomRotationVector(), vector3df(random.frange(15.f, 80.f)), 20.f, .8f);

		m_obstaclePositions.push_back(pos);
	}
	m_finaleRoadblocks();
}

void MapRunner::m_finaleRoadblocks()
{

	StickySpawnerScenarioEffect* eff = new StickySpawnerScenarioEffect();
	setSpawns(eff->aceShipId, eff->shipId, eff->aceWepId, eff->wepId, eff->turretId, eff->turretWepId);
	eff->spawnInterval = 20.f;
	eff->curInterval = 0.f;
	eff->percentageChance = 45;

	if (m_type == SCENARIO_BOSSFIGHT) eff->percentageChance = 90;

	addStatusEffectToEntity(eff, gameController->getPlayer());

	/*
	for (const auto& corr : m_corridors) {
		u32 krakenCount = 1;

		vector3df dir = corr.end - corr.start;
		f32 len = dir.getLength();
		dir.normalize();;
		for (u32 i = 0; i < krakenCount; ++i) {
			auto point = getPointInDisc(corr.end, m_corridorRadius, dir);
			auto upDir = point - corr.end;
			upDir.normalize();
			auto ent = createAlienCarrier(14, corr.end + (upDir * .5f * m_corridorRadius), -dir.getHorizontalAngle(), UPGRADED_PLASMA_SNIPER, 0, UPGRADED_PLASMA);
			setArtillery(ent);
		}
	}
	*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//objectives
void MapRunner::m_dogfight()
{
	auto obj = new RemoveEntityObjective(m_spawnRegWing(m_objectivesStartPos.back(), vector3df(0, 0, 0)));
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}

void MapRunner::m_salvage()
{
	u32 numSalvage = random.urange(3U, 14U);
	std::list<flecs::entity> ls;

	for (u32 i = 0; i < numSalvage; ++i) {
		vector3df supplyPos = getPointInSphere(m_objectivesStartPos.back(), 200.f);
		auto box = createSupplyBox(supplyPos, randomRotationVector(), vector3df(5.f, 5.f, 5.f));
		ls.push_back(box);
	}
	auto obj = new RemoveEntityObjective(ls);
	obj->setType(OBJ_COLLECT);
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}

void MapRunner::m_pod()
{
	auto obj = new RemoveEntityObjective({ createStasisPod(m_objectivesStartPos.back(), randomRotationVector()) });
	obj->setType(OBJ_COLLECT);
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}
void MapRunner::m_destroy_object()
{
	if (m_sector == SECTOR_FLEET_GROUP) {
		auto carrier = createAlienCarrier(14, m_objectivesStartPos.back(), randomRotationVector(), UPGRADED_PLASMA_SNIPER, 0, UPGRADED_PLASMA);
		m_spawnRegWing(m_objectivesStartPos.back() + vector3df(0, -900.f, 0), randomRotationVector());
		m_spawnRegWing(m_objectivesStartPos.back() + vector3df(0, 900.f, 0), randomRotationVector());

		auto obj = new RemoveEntityObjective({carrier});
		obj->setRadioSpawn(m_objectivesStartPos.back());
		m_objectives.push_back(std::shared_ptr<Objective>(obj));
		return;
	}
	auto station = createModularStationFromFile(m_objectivesStartPos.back(), randomRotationVector(), "alien_scoutstation.xml");
	auto obj = new RemoveEntityObjective({station[0]});
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));

	auto patrolStart = m_objectivesStartPos.front();
	patrolStart.Y += 550;

	m_spawnRegWing(patrolStart, vector3df(0, 0, 0));
}
void MapRunner::m_capture()
{
	auto station = createModularHumanStation(m_objectivesStartPos.back(), vector3df(0, 0, 0), 32, FACTION_NEUTRAL, vector3df(15, 15, 15));
	//get the dock module
	std::list<flecs::entity> targs;
	flecs::entity dockTarget = station[0];
	auto capShip = createTroopTransport(m_playerStartPos + vector3df(0, -100.f, 0));
	targs.push_back(dockTarget);
	targs.push_back(capShip);

	auto obj = new DockObjective(targs);
	obj->enemySpawnRate = 21.f;
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));

}
void MapRunner::m_brawl()
{
	vector3df pos = m_objectivesStartPos.front();
	vector3df rotation = (m_playerStartPos - m_objectivesStartPos.front()).getHorizontalAngle();

	if (m_sector == SECTOR_FLEET_GROUP) {
		auto targs = m_fleetBattleSpawn(pos, rotation);
		auto obj = new RemoveEntityObjective(targs);
		obj->setRadioSpawn(pos);
		m_objectives.push_back(ObjElem(obj));
		return;
	}

	vector3df hostileStart = pos + vector3df(650, 0, 0);
	vector3df friendlyStart = pos + vector3df(-550, 0, 0);

	auto targs = spawnShipWing(hostileStart, vector3df(0, -90, 0), getCurAiNum(1), 1, 1, 4, 9, 1, 1);

	spawnShipWing(friendlyStart, vector3df(0, 90, 0), getCurAiNum(-1), 18, 17, 18, 17, 0, 3, true);

	auto obj = new RemoveEntityObjective(targs);
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));

}
void MapRunner::m_frigate_rescue()
{
	vector3df battlePos = m_objectivesStartPos.front();

	createHumanCarrier(12, battlePos, randomRotationVector(), FRIGATE_RAILGUN, 0, UPGRADED_TSUNAMI);

	vector3df rotation = (m_playerStartPos - m_objectivesStartPos.front()).getHorizontalAngle();

	auto targs = m_fleetBattleSpawn(battlePos, rotation);
	m_objectives.push_back(ObjElem(new RemoveEntityObjective(targs)));
}

void MapRunner::m_scuttle()
{
	auto pnode = gameController->getPlayer().get<IrrlichtComponent>()->node;
	auto chaostheory = createChaosTheory(m_playerStartPos + (getNodeDown(pnode)*800.f), pnode->getRotation(), true);
	gameController->setChaosTheory(chaostheory);
	auto station = createModularHumanStation(m_objectivesStartPos.back(), randomRotationVector(), 16, FACTION_NEUTRAL, vector3df(25, 25, 25));
	//get the dock module
	std::list<flecs::entity> targs;
	flecs::entity dockTarget = station[0];
	auto capShip = createTroopTransport(m_playerStartPos + vector3df(0, -100.f, 0));
	targs.push_back(dockTarget);
	targs.push_back(capShip);

	auto obj = new DockObjective(targs);
	obj->setRadioSpawn(m_objectivesStartPos.back());

	obj->completeStr = "We've got the goods here, commander! Bring us back home.";
	obj->dockStr = "We're in pursuit of the supplies -- give us some time.";
	obj->timeToCapture = 30.f;
	obj->enemySpawnRate = 15.f;

	auto escort = new GoToPointObjective({});
	escort->setPoint(m_playerStartPos);
	escort->setDistance(400.f);

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
	m_objectives.push_back(std::shared_ptr<Objective>(escort));
	//finish up with an escort back to the Chaos Theory
}

void MapRunner::m_rock_impact()
{
	f32 scale = 125.f;
	auto rock = createAsteroid(m_objectivesStartPos.back(), randomRotationVector(), vector3df(scale, scale, scale), scale, 20.f, .8f);

	auto obj = new RemoveEntityObjective({ rock });
	obj->setRadioSpawn(m_objectivesStartPos.back());
	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}

void MapRunner::m_extract()
{
	auto station = createModularStationFromFile(m_objectivesStartPos.front(), randomRotationVector(), "fuel_station.xml");
	std::list<flecs::entity> targs;
	flecs::entity dockTarget = station[0];
	auto capShip = createTroopTransport(m_playerStartPos + vector3df(0, -100.f, 0));
	targs.push_back(dockTarget);
	targs.push_back(capShip);

	auto obj = new DockObjective(targs);
	obj->setRadioSpawn(m_objectivesStartPos.back());
	obj->appliesStickySpawner = false;

	obj->completeStr = "We're all clear. Let's head on back to the rendezvous, commander.";
	obj->dockStr = "We're reactivating the fuel siphons now, commander. Give us a minute.";
	obj->timeToCapture = 30.f;
	//obj->enemySpawnRate = 900.f;

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
	auto escort = new GoToPointObjective({});
	escort->setPoint(m_playerStartPos);
	escort->setDistance(400.f);
	m_objectives.push_back(std::shared_ptr<Objective>(escort));
}

void MapRunner::m_distress()
{
	auto beacon = createStaticObstacle(32, m_objectivesStartPos.front(), randomRotationVector(), vector3df(5.f, 5.f, 5.f));
	auto light = createLightWithVolumeMesh(64, 8, 100.f, SColor(10, 255, 255, 255), SColor(0, 255, 0, 0), beacon.get<IrrlichtComponent>()->node);
	CSceneNodeScalePulseAnimator* anim = new CSceneNodeScalePulseAnimator(device->getTimer()->getTime(), vector3df(1, 1, 1), vector3df(10, 10, 10), 6000U);
	light->addAnimator(anim);
	anim->drop();

	GoToPointObjective* obj = new GoToPointObjective({ beacon });
	Objective* secondary = nullptr;
	bool stillAlive = random.coinflip();
	flecs::entity targ = INVALID_ENTITY;
	obj->setRadioSpawn(m_objectivesStartPos.back());
	if (stillAlive) {
		targ = createDormantShipAsObstacle(12, m_objectivesStartPos.front() + vector3df(0.f, 200.f, 0.f), randomRotationVector(), FRIGATE_RAILGUN);
		obj->setCompletionCb(onDistressBeaconSuccessCb);
		auto sec = new WaveDefenseObjective();
		sec->addWave(m_scoutAce, m_scoutAceGun, m_scoutShip, m_scoutGun, m_scoutTurret, m_scoutTurretGun);
		sec->addWave(m_scoutAce, m_scoutAceGun, m_scoutShip, m_scoutGun, m_scoutTurret, m_scoutTurretGun);
		secondary = sec;
	}
	else {
		targ = createDeadShipAsObstacle(12, m_objectivesStartPos.front() + vector3df(0.f, 200.f, 0.f), randomRotationVector(), FRIGATE_RAILGUN);
		obj->setCompletionCb(onDistressBeaconFailCb);
		auto sec = new RemoveEntityObjective({ targ });
		sec->setCompletionCb(onAmbushBustCb);
	}
	m_objectives.push_back(ObjElem(obj));
	if (secondary) m_objectives.push_back(ObjElem(secondary));
}

void MapRunner::m_escort()
{

}

void MapRunner::m_search_and_destroy()
{
	for (const auto& pos : m_objectivesStartPos) {
		auto targets = m_spawnRegWing(pos, randomRotationVector());
		targets.front().get_mut<AIComponent>()->hasPatrolRoute = true;

		RemoveEntityObjective* obj = new RemoveEntityObjective(targets);
		obj->setRadioSpawn(pos);
		m_objectives.push_back(std::shared_ptr<Objective>(obj));
	}
}

void MapRunner::m_harvest()
{
	auto& point = m_objectivesStartPos.back();

	std::list<flecs::entity> targs;
	u32 num = random.urange(2, 3);
	for (u32 i = 0; i < num; ++i) {
		auto pos = getPointInSphere(point, 105.f);
		auto ent = createMoneyAsteroid(pos, randomRotationVector(), vector3df(25.f));
		initializeFaction(ent, FACTION_NEUTRAL);
		targs.push_back(ent);
	}
	auto obj = new RemoveEntityObjective(targs);
	obj->setRadioSpawn(point);

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}

void MapRunner::m_scramble()
{
	auto carr = createAlienCarrier(8, m_objectivesStartPos.front() + vector3df(0, 150, 0), vector3df(0, 180, 0));
	auto obj = new RemoveEntityObjective({carr});
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}

void MapRunner::m_arnold()
{
	gameController->addLargePopup(L"Sir, Arnold's wing ran into far more resistance than we expected. I'm sending you navigational coordinates to pick up the pods. We can't afford losses.", L"Steven Mitchell");
	gameController->addLargePopup(L"Damn it, just leave me! You've got more important places to be! Go!", L"Arnold Kenmar");
	gameController->addLargePopup(L"Reading massive amounts of cloaked signals. Avoid extended engagements if possible.", L"Steven Mitchell");

	std::list<flecs::entity> targs;

	for (u32 i = 0; i < 3; ++i) {
		auto pod = createStasisPod(getPointInSphere(m_objectivesStartPos.front(), 250.f), randomRotationVector());
		targs.push_back(pod);
	}

	for (u32 i = 0; i < 4; ++i) {
		createDeadShipAsObstacle(0, getPointInShell(m_objectivesStartPos.front(), 200.f, 350.f), randomRotationVector(), 3);
	}

	createDeadShipAsObstacle(8, m_objectivesStartPos.front() + vector3df(0, -400.f, 0), randomRotationVector(), 1);

	StickySpawnerScenarioEffect* eff = new StickySpawnerScenarioEffect();
	setSpawns(eff->aceShipId, eff->shipId, eff->aceWepId, eff->wepId, eff->turretId, eff->turretWepId);
	eff->spawnInterval = 24.f;
	eff->curInterval = 8.f;
	eff->percentageChance = 75;
	addStatusEffectToEntity(eff, gameController->getPlayer());

	auto obj = new RemoveEntityObjective(targs);
	obj->setType(OBJ_COLLECT);
	obj->setRadioSpawn(m_objectivesStartPos.front());
	obj->setCompletionCb(arnoldCollectedCb);
	m_objectives.push_back(ObjElem(obj));

	auto finobj = new GoToPointObjective({});
	finobj->setType(OBJ_GO_TO);
	finobj->setPoint(m_objectivesStartPos.back());
	finobj->setRadioSpawn(m_objectivesStartPos.back());
	m_objectives.push_back(ObjElem(finobj));
}

void MapRunner::m_sean()
{
	gameController->addLargePopup(L"This seems to be the correct sector... head to the first point, sir.", L"Sean Cooper");

	auto obj1 = new GoToPointObjective({});
	obj1->setType(OBJ_GO_TO);
	obj1->setPoint(m_objectivesStartPos.front());
	obj1->setRadioSpawn(m_objectivesStartPos.front());
	obj1->setCompletionCb(sean1Cb);
	m_objectives.push_back(ObjElem(obj1));

	auto obj2 = new GoToPointObjective({});
	obj2->setType(OBJ_GO_TO);
	obj2->setPoint(m_objectivesStartPos[1]);
	obj2->setRadioSpawn(m_objectivesStartPos[1]);
	obj2->setCompletionCb(sean2Cb);
	m_objectives.push_back(ObjElem(obj2));

	auto pendulum = createDeadShipAsObstacle(12, m_objectivesStartPos[2] - vector3df(60.f), randomRotationVector(), 3);
	pendulum.set_doc_name("RFS Pendulum");

	createSupplyBox(m_objectivesStartPos[2] + vector3df(40.f), randomRotationVector(), vector3df(5.f));
	createSupplyBox(m_objectivesStartPos[2] + vector3df(40.f,0.f,0.f), randomRotationVector(), vector3df(5.f));
	createSupplyBox(m_objectivesStartPos[2] + vector3df(70.f), randomRotationVector(), vector3df(5.f));

	auto obj3 = new GoToPointObjective({});
	obj3->setType(OBJ_GO_TO);
	obj3->setPoint(m_objectivesStartPos[2]);
	obj3->setRadioSpawn(m_objectivesStartPos[2]);
	obj3->setCompletionCb(sean3Cb);
	m_objectives.push_back(ObjElem(obj3));

	auto tux = createDeadShipAsObstacle(0, m_objectivesStartPos.back(), randomRotationVector(), 3);
	tux.set_doc_name("Empty Tuxedo?");

	auto obj4 = new GoToPointObjective({});
	obj4->setType(OBJ_GO_TO);
	obj4->setPoint(m_objectivesStartPos.back());
	obj4->setRadioSpawn(m_objectivesStartPos.back());
	obj4->setCompletionCb(sean4Cb);
	m_objectives.push_back(ObjElem(obj4));
}

void MapRunner::m_cat()
{
	auto obj = new GoToPointObjective({});
	obj->setType(OBJ_GO_TO);
	obj->setPoint(m_objectivesStartPos.back());
	obj->setRadioSpawn(m_objectivesStartPos.back());
	obj->setCompletionCb(catCb);
	m_objectives.push_back(ObjElem(obj));
}

void MapRunner::m_tauran()
{
	auto station = createModularHumanStation(m_objectivesStartPos.back(), randomRotationVector(), 16, FACTION_NEUTRAL, vector3df(25, 25, 25));
	//get the dock module
	std::list<flecs::entity> targs;
	flecs::entity dockTarget = station[0];
	auto capShip = createTroopTransport(m_playerStartPos + vector3df(0, -100.f, 0));
	targs.push_back(dockTarget);
	targs.push_back(capShip);

	auto obj = new DockObjective(targs);
	obj->setRadioSpawn(m_objectivesStartPos.back());

	obj->spkr = "Tauran Druugas";
	obj->dockStr = "Won't take but a moment, commander. I'm uploading the data as quickly as I can.";
	obj->completeStr = "I do believe we're set. Making my way toward the exit now, commander. See you back on the Chaos Theory.";
	obj->timeToCapture = 30.f;
	obj->enemySpawnRate = 20.f;
	obj->setCompletionCb(tauranCb);

	m_objectives.push_back(std::shared_ptr<Objective>(obj));

	auto obj2 = new GoToPointObjective({});
	obj2->setPoint(m_objectivesStartPos.back());
	obj2->setRadioSpawn(m_objectivesStartPos.back());
	m_objectives.push_back(std::shared_ptr<Objective>(obj2));
}

void MapRunner::m_lee()
{
	auto station = createModularStationFromFile(m_objectivesStartPos.back(), randomRotationVector(), "alien_scoutstation.xml", true, FACTION_NEUTRAL);
	//get the dock module
	std::list<flecs::entity> targs;
	flecs::entity dockTarget = station[0];
	auto capShip = createTroopTransport(m_playerStartPos + vector3df(0, -100.f, 0));
	targs.push_back(dockTarget);
	targs.push_back(capShip);

	auto obj = new DockObjective(targs);
	obj->setRadioSpawn(m_objectivesStartPos.back());
	
	obj->completeStr = "Sir, Lee's back, but she passed out almost as soon as we got to her. She's got something with her -- we're bringing that back too. Get us out of here.";
	obj->dockStr = "This place is eerily quiet, commander. Lee just sprinted right in. We're chasing after her now.";
	obj->timeToCapture = 30.f;
	obj->enemySpawnRate = 20.f;
	obj->setCompletionCb(leeFinishCb);

	m_objectives.push_back(std::shared_ptr<Objective>(obj));

	auto obj2 = new GoToPointObjective({});
	obj2->setPoint(m_objectivesStartPos.back());
	obj2->setRadioSpawn(m_objectivesStartPos.back());
	m_objectives.push_back(std::shared_ptr<Objective>(obj2));
}

void MapRunner::m_theod()
{
	gameController->addLargePopup("That's it. I've detected the signal up ahead. Bring me my tools, human, and I will craft you a masterpiece.", "Theod Tantrus");
	flecs::entity ship = createDeadShipAsObstacle(24, m_objectivesStartPos.front(), randomRotationVector(), 3);
	gameController->registerDeathCallback(ship, supplyBoxDeath);
	AIComponent ai;
	auto mineAI = new MineAI();
	mineAI->self = ship;
	ai.aiControls = std::shared_ptr<AIType>(mineAI);
	ship.set<AIComponent>(ai);	
	ThrustComponent thrust;
	HardpointComponent hards;
	ship.set<ThrustComponent>(thrust);
	ship.set<HardpointComponent>(hards);
	initializeNeutralFaction(ship);
	initializeSensors(ship, 175.f, DEFAULT_SENSOR_UPDATE_INTERVAL, true);

	auto obj = new RemoveEntityObjective({ship});
	obj->setType(OBJ_COLLECT);
	obj->setRadioSpawn(m_objectivesStartPos.front());
	obj->setCompletionCb(theodCollectCb);
	m_objectives.push_back(ObjElem(obj));


	auto finobj = new GoToPointObjective({});
	finobj->setType(OBJ_GO_TO);
	finobj->setPoint(m_objectivesStartPos.back());
	finobj->setRadioSpawn(m_objectivesStartPos.back());
	m_objectives.push_back(ObjElem(finobj));
}
void MapRunner::m_arthur()
{
	gameController->addLargePopup("Analysis of this sector indicates several possible locations for the weapons. We will need to search.", "ARTHUR");
	auto wep1 = createWeaponAsObstacle(30, HRDP_REGULAR, m_objectivesStartPos.front(), randomRotationVector(), vector3df(8.f), INVALID_NETWORK_ID, true);
	auto obj = new RemoveEntityObjective({ wep1 });
	obj->setType(OBJ_COLLECT);
	obj->setRadioSpawn(m_objectivesStartPos.front());
	obj->setCompletionCb(arthur1Cb);
	m_objectives.push_back(ObjElem(obj));

	auto wep2 = createWeaponAsObstacle(30, HRDP_REGULAR, m_objectivesStartPos[1] + vector3df(40.f), randomRotationVector(), vector3df(8.f), INVALID_NETWORK_ID, true);
	auto wep3 = createWeaponAsObstacle(30, HRDP_REGULAR, m_objectivesStartPos[1] + vector3df(-40.f), randomRotationVector(), vector3df(8.f), INVALID_NETWORK_ID, true);
	obj = new RemoveEntityObjective({ wep2, wep3 });
	obj->setType(OBJ_COLLECT);
	obj->setRadioSpawn(m_objectivesStartPos[1]);
	obj->setCompletionCb(arthur2Cb);
	m_objectives.push_back(ObjElem(obj));

	auto wep4 = createWeaponAsObstacle(30, HRDP_REGULAR, m_objectivesStartPos.back(), randomRotationVector(), vector3df(8.f), INVALID_NETWORK_ID, true);
	obj = new RemoveEntityObjective({ wep4 });
	obj->setType(OBJ_COLLECT);
	obj->setRadioSpawn(m_objectivesStartPos.back());
	obj->setCompletionCb(arthur3Cb);
	m_objectives.push_back(ObjElem(obj));
}

void MapRunner::m_james()
{
	gameController->addLargePopup(L"Hahahaha! Commander, do you see it?! Do you see our quarry? It is *time*, commander!", L"James Lavovar");
	auto ent = createAlienCarrier(14, m_objectivesStartPos.front(), randomRotationVector(), UPGRADED_PLASMA_SNIPER, 0, UPGRADED_PLASMA);
	auto obj = new RemoveEntityObjective({ ent });
	obj->setType(OBJ_DESTROY);
	obj->setRadioSpawn(m_objectivesStartPos.front());
	obj->setCompletionCb(jamesCb);
	m_objectives.push_back(ObjElem(obj));
}

void _tutorial1()
{
	gameController->addLargePopup(L"Up and running. Commander, do a few strafes and test the jets.", L"Martin Hally");
	gameController->addLargePopup(L"Use " + getKeyDesc(IN_STRAFE_UP) + L" and " + getKeyDesc(IN_STRAFE_DOWN) + L" to move up and down. Use " + 
	getKeyDesc(IN_STRAFE_LEFT) + L" and " + getKeyDesc(IN_STRAFE_RIGHT) + L" to strafe left and right. Try those now.", L"Tutorial");
}

void _tutorial2()
{
	gameController->addLargePopup(L"Right. Alright, do a few rolls, and then test the boost capacity.", L"Martin Hally");
	gameController->addLargePopup(L"Use " + getKeyDesc(IN_ROLL_LEFT) + L" and " + getKeyDesc(IN_ROLL_RIGHT) + L" to roll left and right. Use " +
		getKeyDesc(IN_AFTERBURNER) + L" to accelerate forward with a massive boost. Try those now.", L"Tutorial");
}

void _tutorial3()
{
	gameController->addLargePopup(L"Careful, dammit! If you go too fast, you're gonna take more damage. Test out your guns now. Target some of the nearby debris and let fly.", L"Martin Hally");
	gameController->addLargePopup(L"Use " + getKeyDesc(IN_SELECT_TARGET) + L" to select a target, then use " + getKeyDesc(IN_FIRE_REGULAR) + L" to fire your basic weaponry.", L"Tutorial");
}

void _tutorial4()
{
	gameController->addLargePopup(L"On the right-hand side of your screen ammunition counts are shown for your weaponry. Press " + getKeyDesc(IN_RELOAD)
		+ L" to manually reload. Some guns will also cost energy to fire, shown above your health.", L"Tutorial");
	gameController->addLargePopup(L"Your ship is also equipped with heavy weaponry -- high-damage but slow. Press " + getKeyDesc(IN_FIRE_HEAVY) + L" to fire your rail gun.", L"Tutorial");
}

void _tutorial5()
{
	gameController->addLargePopup(L"Your ship is also equipped with a physics-based hardpoint, which can manipulate objects and other ships. Press " + getKeyDesc(IN_FIRE_PHYS) + L" to fire your impulse cannon.", L"Tutorial");
}

void _tutorial6()
{
	gameController->addLargePopup(L"Good work, sir. We've got a small problem, though. We've got a few malfunctioning turrets between you and a chase team. Take them out so they can get there.", L"Steven Mitchell");
	gameController->addLargePopup(L"Your HUD will show contacts for objectives, hostiles, and friendlies. Select and move toward the blue diamond, then destroy all hostiles nearby.", L"Tutorial");
	gameController->addLargePopup(L"Enemy ships won't be standing still, and neither will you. With them selected as a target, aim for the crosshairs in front of them to hit them.", L"Tutorial");
}

void _tutorial7()
{
	gameController->triggerShipSpawn([](){
		for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
			if (!campaign->getAssignedWingman(i) || !campaign->getAssignedShip(i)) continue;
			auto playerIrr = gameController->getPlayer().get<IrrlichtComponent>();
			vector3df wingpos = playerIrr->node->getAbsolutePosition();
			wingpos += getNodeBackward(playerIrr->node) * 50.f;
			if (i % 2 == 0) {
				wingpos.X += 20.f + (i * 20.f);
			}
			else {
				wingpos.X -= 20.f + (i * 20.f);
			}
			flecs::entity wingman = createWingman(i, gameController->getPlayer(), wingpos, playerIrr->node->getRotation());
			gameController->setWingman(wingman, i);
		}
		});
	gameController->addLargePopup(L"Sending out the chase squad now. Lieutenants Cooper and Cheadle, bring the commander in.", L"Steven Mitchell");
	gameController->addLargePopup(L"Yes, sir! Good that you're in one piece, commander! Let's move toward the rendezvous, yeah?", L"Cat Cheadle");
	gameController->addLargePopup(L"Your wingmen will engage enemies on their own, but can be ordered toward specific targets. Hit " + getKeyDesc(IN_OPEN_COMMS) + L" to open your comms menu and see the options.", L"Tutorial");
	gameController->addLargePopup(L"Make sure to keep your wingmen focused -- if they're shot down, they'll be injured for several engagements.");
	gameController->addLargePopup(L"When you're ready, move toward the next objective to complete the tutorial.", L"Tutorial");
}

void MapRunner::m_tutorial()
{
	gameController->addLargePopup(L"Got it. The Tuxedo should start up now.", L"Martin Hally");
	gameController->addLargePopup(L"Excellent. Sir, can you hear us? Try moving your ship around, we think we've remotely reactivated it.", L"Steven Mitchell");
	gameController->addLargePopup(L"Use your mouse to move the camera around, and use " + getKeyDesc(IN_THRUST_FORWARDS) + L" and " + getKeyDesc(IN_STRAFE_BACKWARDS) + L" to move forward and backward. Try those now.", L"Tutorial");
	KeyPressObjective* obj = new KeyPressObjective({ IN_THRUST_FORWARDS, IN_STRAFE_BACKWARDS });
	obj->setCompletionCb(_tutorial1);
	m_objectives.push_back(ObjElem{ obj });
	obj = new KeyPressObjective({ IN_STRAFE_UP, IN_STRAFE_DOWN, IN_STRAFE_LEFT, IN_STRAFE_RIGHT });
	obj->setCompletionCb(_tutorial2);
	m_objectives.push_back(ObjElem{ obj });
	obj = new KeyPressObjective({ IN_ROLL_LEFT, IN_ROLL_RIGHT, IN_AFTERBURNER });
	obj->setCompletionCb(_tutorial3);
	m_objectives.push_back(ObjElem{ obj });
	obj = new KeyPressObjective({ IN_FIRE_REGULAR, IN_SELECT_TARGET });
	obj->setCompletionCb(_tutorial4);
	m_objectives.push_back(ObjElem{ obj });
	obj = new KeyPressObjective({ IN_FIRE_HEAVY });
	obj->setCompletionCb(_tutorial5);
	m_objectives.push_back(ObjElem{ obj });
	obj = new KeyPressObjective({ IN_FIRE_PHYS });
	obj->setCompletionCb(_tutorial6);
	m_objectives.push_back(ObjElem{ obj });
	
	RemoveEntityObjective* obj2 = new RemoveEntityObjective({
		createDebrisTurret(m_objectivesStartPos[0] + vector3df(75.f), randomRotationVector(), vector3df(5.f)),
		createDebrisTurret(m_objectivesStartPos[0] + vector3df(-75.f), randomRotationVector(), vector3df(5.f)),
		});
	//obj2->setRadioSpawn(m_objectivesStartPos[0]);
	obj2->setCompletionCb(_tutorial7);
	m_objectives.push_back(ObjElem{ obj2 });
	GoToPointObjective* obj3 = new GoToPointObjective({});
	obj3->setPoint(m_objectivesStartPos.back());
	obj3->setDistance(400.f);
	obj3->setRadioSpawn(m_objectivesStartPos.back());
	m_objectives.push_back(ObjElem{ obj3 });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bossfights

void MapRunner::m_debrisBoss()
{
	vector3df pos = m_objectivesStartPos[0];

	auto carr = createAlienCarrier(8, pos + vector3df(0, 225, 0), randomRotationVector());
	auto carr2 = createAlienCarrier(8, pos + vector3df(0, -225, 0), randomRotationVector());

	auto obj = new RemoveEntityObjective({carr, carr2});
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}
void MapRunner::m_asteroidBoss()
{
	auto station = createModularStationFromFile(m_objectivesStartPos[0], randomRotationVector(), "alien_heavyoutpost.xml");
	auto obj = new RemoveEntityObjective({ station[0] }); // central point
	obj->setRadioSpawn(m_objectivesStartPos.back());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}
void MapRunner::m_gasBoss()
{
	auto station = createModularHumanStation(m_objectivesStartPos[0], randomRotationVector(), 20);
	auto obj = new GoToPointObjective({ station[0] });
	m_spawnScoutWing(m_objectivesStartPos[0] + vector3df(0, 500, 0), randomRotationVector());

	m_objectives.push_back(std::shared_ptr<Objective>(obj));
	
	auto waveObj = new WaveDefenseObjective();
	u32 waveCount = 4;
	for (u32 i = 0; i < waveCount; ++i) {
		waveObj->addWave(m_regAce, m_regAceGun, m_regShip, m_regGun, m_regTurret, m_regTurretGun);
	}
	m_objectives.push_back(std::shared_ptr<Objective>(waveObj));
}

void MapRunner::m_supplyDepotBoss()
{
	u32 turretWepId = 3;
	if (m_campaignMatch) {
		if (campaign->getFlag(L"SUPPLYBOSS_WEAPONS")) turretWepId = 16;
	}
	auto station = createModularHumanStation(m_objectivesStartPos[0], vector3df(0, 0, 0), 36, FACTION_PLAYER, vector3df(13, 13, 13), 0, turretWepId);
	//get the dock module
	std::list<flecs::entity> targs;
	flecs::entity dockTarget = station[0];
	auto capShip = createTroopTransport(m_playerStartPos + vector3df(0, -100.f, 0));
	targs.push_back(dockTarget);
	targs.push_back(capShip);

	//create the extra parameters
	if (m_campaignMatch) {
		if (campaign->getFlag(L"SUPPLYBOSS_MINES")) {
			for (u32 i = 0; i < 120; ++i) {
				auto pos = getPointInShell(m_objectivesStartPos[0], 2000.f, 2400.f);
				createMine(pos, randomRotationVector(), vector3df(5, 5, 5));
			}
		}
		if (campaign->getFlag(L"SUPPLYBOSS_DEFENSE")) {
			u32 numCapStations = random.d4();
			for (u32 i = 0; i < numCapStations; ++i) {
				auto point = getPointInTorus(m_objectivesStartPos[0], 1800.f, 2000.f);
				createCaptureableWepPlatform(point, vector3df(0, random.frange(-180.f, 180.f), 0), 0, 3);
			}
		}
		if (campaign->getFlag(L"SUPPLYBOSS_DRONES")) {
			//build out some drones
			auto ships = spawnShipWing(m_playerStartPos + vector3df(200, 0, 0), vector3df(0, 0, 0), 4, 15, 17, 15, 17, 0, 3, true);
			for (auto& ship : ships) {
				ship.get_mut<AIComponent>()->wingCommander = gameController->getPlayer();
			}
		}
	}
	auto obj = new DockObjective(targs);
	obj->completeStr = "We're in the clear -- the signal is operating at max range. We're out of sight! Pick us up.";
	obj->dockStr = "On my way to the communications tower. Cover us, commander.";
	obj->breakStr = "Dammit, commander! Get us back to a docking position!";
	obj->spkr = "Martin Hally";
	obj->timeToCapture = 4.f;
	obj->enemySpawnRate = 21.f;
	m_objectives.push_back(std::shared_ptr<Objective>(obj));
}

void MapRunner::m_ringsBoss()
{
	auto causality = createFriendlyShip(21, 23, m_objectivesStartPos[0], vector3df());
	initializeDefaultAI(causality);
	initializeTurretsOnOwner(causality, 0, 13);

	auto rocks = m_spawnWing(m_objectivesStartPos[0] + vector3df(150, -600, 9), randomRotationVector(), 16, 17, 19, 19, 1, 1);

	auto goTo = new GoToPointObjective({ causality });
	goTo->setDistance(50.f);
	m_objectives.push_back(std::shared_ptr<Objective>(goTo));
	
	auto defense = new WaveDefenseObjective();
	
	auto frigateFormation = spawnShipWing(m_objectivesStartPos[0] + vector3df(-300, -900, 5), randomRotationVector(),4,  12, 16, 12, 16, 1, 3, true);

	auto destroyerFormation = spawnShipWing(m_objectivesStartPos[0] + vector3df(0, 1500, 5), randomRotationVector(), 4, 19, 11, 19, 11, 1, 3, true);

	const u32 numWaves = 4;
	for (u32 i = 0; i < numWaves; ++i) {
		defense->addWave(17, 19, 17, 19, 1, 1);
	}
	defense->addWave(16, 19, 17, 19, 1, 1);
	m_objectives.push_back(std::shared_ptr<Objective>(defense));
}

void MapRunner::m_fleetBoss()
{
	vector3df rotation = (m_playerStartPos - m_objectivesStartPos.front()).getHorizontalAngle();
	vector3df up = rotation.rotationToDirection(vector3df(0, 1, 0));
	vector3df playerForward = rotation.rotationToDirection(vector3df(0, 0, -1));

	vector3df runPoint = m_objectivesStartPos.front() + (playerForward * 900.f);
	m_fleetBattleSpawn(m_objectivesStartPos.front() + (up * 500.f), rotation);
	m_fleetBattleSpawn(m_objectivesStartPos.front() + (-up * 400.f), rotation);

	if (m_campaignMatch) {
		if (campaign->getFlag(L"DAVIS_SAVED")) {
			auto pod = createStasisPod(m_objectivesStartPos.front(), randomRotationVector());
			auto obj = new RemoveEntityObjective({ pod });
			obj->setType(OBJ_COLLECT);
			obj->setRadioSpawn(m_objectivesStartPos.back());
			m_objectives.push_back(ObjElem(obj));
			runPoint = m_playerStartPos + (-playerForward * 1000.f);
		}
	}
	auto skedaddle = new GoToPointObjective({});
	skedaddle->setPoint(runPoint);
	skedaddle->setRadioSpawn(runPoint);
	m_objectives.push_back(ObjElem(skedaddle));
	m_cleanup = false;
}

void MapRunner::m_finaleBoss()
{
	//reset the objectives
	m_objectivesStartPos.clear();

	vector3df origin(0.f);

	vector3df directionToOrigin = (origin - m_playerStartPos).normalize();
	f32 distBetweenTorus = 25000.f;
	vector3df torusCenter = m_playerStartPos + (directionToOrigin * distBetweenTorus);
	f32 out = std::min(m_objectiveData.getFloat("playerStartRadius"), m_maxObjGenRadius);
	f32 torusRad = std::min(m_objectiveData.getFloat("objTorusRadius"), m_maxObjTorusRadius);
	f32 in = out - torusRad;

	vector3df pos = getPointInTorus(torusCenter, in, out, directionToOrigin);
	m_objectivesStartPos.push_back(pos);

	//TODO: set this up to be a ship instead
	auto finalBoss = createModularStationFromFile(pos, randomRotationVector(), "boss.xml");
	std::list<flecs::entity> targs;
	for (const auto& ent : finalBoss) {
		if (ent.get<StationModuleComponent>()->type == SPIECE_BOSS_ARM) targs.push_back(ent);
	}
	auto obj = new RemoveEntityObjective(targs);
	obj->setRadioSpawn(pos);

	m_objectives.push_back(ObjElem(obj));

	auto skedaddle = new GoToPointObjective({});
	skedaddle->setPoint(m_playerStartPos);
	skedaddle->setRadioSpawn(m_playerStartPos);
	m_objectives.push_back(ObjElem(skedaddle));
	m_cleanup = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//spawners
std::list<flecs::entity> MapRunner::m_spawnRegWing(vector3df pos, vector3df rot)
{
	return m_spawnWing(pos, rot, m_regAce, m_regShip, m_regAceGun, m_regGun, m_regTurret, m_regTurretGun);
}

std::list<flecs::entity> MapRunner::m_spawnScoutWing(vector3df pos, vector3df rot)
{
	return m_spawnWing(pos, rot, m_scoutAce, m_scoutShip, m_scoutAceGun, m_scoutGun, m_scoutTurret, m_scoutTurretGun, -1);
}

std::list<flecs::entity> MapRunner::m_spawnWing(vector3df pos, vector3df rot, s32 aceShip, s32 regShip, s32 aceGun, s32 regGun, s32 turret, s32 turretGun, s32 numAdjust)
{
	auto targs = spawnShipWing(pos, rot, getCurAiNum(numAdjust), regShip, regGun, aceShip, aceGun, turret, turretGun);
	auto& cdr = targs.front();
	auto cdrai = cdr.get_mut<AIComponent>();
	cdrai->startPos = irrVecToBt(pos);
	cdrai->hasPatrolRoute = true;

	return targs;
}

std::list<flecs::entity> MapRunner::m_fleetBattleSpawn(vector3df pos, vector3df rot)
{
	vector3df up, right, forward;
	forward = rot.rotationToDirection(vector3df(0, 0, 1));
	up = rot.rotationToDirection(vector3df(0, 1, 0));
	right = rot.rotationToDirection(vector3df(1, 0, 0));

	spawnShipWing(pos + (-forward * 800.f) + (right * 800.f), rot, getCurAiNum(), _randAlienFighter(), 1, _randAlienFighter(), 1);
	spawnShipWing(pos + (-forward * 500.f) + (-right * 800.f), rot, getCurAiNum(), _randAlienFighter(), 1, _randAlienFighter(), 1);

	vector3df alienStart = pos + (-forward * 1700.f);
	vector3df alienPos[2] = { alienStart, alienStart + (up * 1200.f)};
	std::list<flecs::entity> targs;
	for (u32 i = 0; i < 2; ++i) {
		auto ent = createAlienCarrier(8, alienPos[i], rot, UPGRADED_PLASMA_SNIPER, 0, UPGRADED_PLASMA);
		setArtillery(ent);
		targs.push_back(ent);
	}
	return targs;
}

std::shared_ptr<Objective> MapRunner::m_getCleanup()
{
	std::list<flecs::entity> cleanuptargs;
	game_world->each([&cleanuptargs](flecs::entity e, FactionComponent& fac) { //grab all hostiles
		if (!e.has<IrrlichtComponent>()) return;
		if (!e.get<IrrlichtComponent>()->node) return;
		vector3df playerpos = gameController->getPlayer().get<IrrlichtComponent>()->node->getPosition();
		vector3df entpos = e.get<IrrlichtComponent>()->node->getAbsolutePosition();
		if (fac.type == FACTION_HOSTILE && (entpos-playerpos).getLength() <= 1500.f && !e.has<ObstacleComponent>() && !e.has<TurretTag>()) {
			cleanuptargs.push_back(e);
		}
		});
	m_cleanup = false;
	auto lastObj = new RemoveEntityObjective(cleanuptargs);
	return std::shared_ptr<Objective>(lastObj);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//wholesale scenario building
bool MapRunner::m_buildScenarioFromXml(std::string xmlstr)
{
	baedsLogger::log("Building a scenario from file " + xmlstr + "...\n");

	std::string file = "assets/attributes/scenarios/custom_objectives/" + xmlstr;
	auto xml = createIrrXMLReader(file.c_str());
	if (!xml) return false;

	const stringw objTag = L"objective";
	const stringw globalTag = L"global";
	const stringw teamTag = L"team";

	while (xml->read()) {
		switch (xml->getNodeType()) {
		case EXN_ELEMENT: {
			const stringw type = xml->getNodeName();
			if (type.equals_ignore_case(globalTag)) {
				std::string objective = xml->getAttributeValueSafe("objective");
				std::string env = xml->getAttributeValueSafe("map");
				std::string skybox = xml->getAttributeValueSafe("sky");
				m_customMap = (env == "custom" || env == "Custom");
				//use these later.
				bool mp = (xml->getAttributeValueSafe("teamMultiplayer") == "true");
				int teamCount = xml->getAttributeValueAsInt("teamCount");

				baedsLogger::log("Scenario has type " + objective + " in environment " + env 
					+ "Customized map: " + (m_customMap ? "yes" : "no") + ", multiplayer: " + (mp ? "yes" : "no") + ".\n");

				//if it's not a custom objective (and we're in campaign) we can just outright load the objective
				if (objective != "Custom" && objective != "custom" && stateController->inCampaign) {
					SCENARIO_TYPE type = SCENARIO_NOT_LOADED;
					for (auto& [key, val] : objectiveTypeStrings) {
						if (val == objective) type = key;
					}
					if (type == SCENARIO_NOT_LOADED)
						goto error;

					//m_builtScenario.setType(campaign->getDifficulty(), type);
				}

			}
			break;
		}
		default: {
			break;
		}
		}//end switch
	}
	delete xml;
	return true;
error:
	baedsLogger::errLog("Something went wrong loading the custom scenario.\n");
	delete xml;
	return false;
}
