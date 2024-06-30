#include "GameController.h"
#include "GameStateController.h"
#include "ObstacleUtils.h"
#include "BaedsLightManager.h"
#include "Networking.h"
#include "NetworkingComponent.h"
#include "SensorCallbackComponent.h"
#include "PlayerComponent.h"
#include "BulletRigidBodyComponent.h"
#include "BulletGhostComponent.h"
#include "ShipParticleComponent.h"
#include "WeaponInfoComponent.h"
#include "IrrlichtComponent.h"
#include "Config.h"
#include "AudioDriver.h"
#include "Campaign.h"
#include "GameAssets.h"
#include "btUtils.h"
#include "HUDHeader.h"
#include "SensorComponent.h"
#include "CrashLogger.h"
#include "SystemsHeader.h"
#include "GameFunctions.h"
#include "ShipUtils.h"
#include "PhysShieldComponent.h"
#include <iostream>
#include <random>

//macro to comment out if I want to include debug lines of bt objects
//#define INCLUDE_DEBUGGING_LINES

const btVector3 GameController::playerStart() const { return irrVecToBt(mapRunner.playerStart()); }

GameController::GameController()
{
	open = false;
	cfg->game.loadConfig("assets/cfg/gameconfig.gdat");
	cfg->game.saveConfig("assets/cfg/gameconfig.gdat");
	cfg->game.setAudioGains();
	m_dummyScenario = new Scenario(SECTOR_ASTEROID);
	m_dummyScenario->setType(1, SCENARIO_DUMMY);
}

void GameController::markForDeath(flecs::entity& ent, bool fromNetwork)
{
	if (isMarkedForDeath(ent)) return;
	m_markedForDeath.push_back(ent);
	if (gameController->isNetworked() && !fromNetwork) {
		if (ent.has<NetworkingComponent>())
			m_networkMarkedForDeath.push_back(ent.get<NetworkingComponent>()->networkedId);
	}
}

void GameController::update()
{
	u32 now = device->getTimer()->getTime();
	f32 delta = (f32)(now - then) / 1000.f;
	if (delta > .25) { //If the delta is too big, it's .25.
		delta = .25;
	}
	then = now;
	accumulator += delta;
	m_timeSinceScenarioStart += delta;

	musicChangeCurCooldown += delta;

	if (m_inCombat && (musicChangeCurCooldown > musicChangeCooldown)) {
		triggerAmbientMusic();
	}
	audioDriver->ingameMusicUpdate(delta);
	if (!m_finishingAnim && !m_startingAnim) {
		try {
			baedsLogger::logSystem("Bullet Update");
			bWorld->stepSimulation(delta, 15, btScalar(1.f) / btScalar(100.f));
			collisionCheckingSystem();
		}
		catch (...) {
			baedsLogger::errLog("Bullet simulation failed to progress correctly\n");
		}
		auto rbf = game_world->filter<BulletRigidBodyComponent, IrrlichtComponent>();
		rbf.each([](flecs::entity e, BulletRigidBodyComponent& rbc, IrrlichtComponent& irr) {
			if (!e.is_alive())
				return;
			if (!e.has<IrrlichtComponent>() || !e.has<BulletRigidBodyComponent>())
				return;
			irrlichtRigidBodyPositionSystem(e, rbc, irr);
			});
	}

	if (getPlayer().is_alive()) {
		auto pf = game_world->filter<IrrlichtComponent, PlayerComponent, BulletRigidBodyComponent>();
		pf.each([](flecs::entity e, IrrlichtComponent& irr, PlayerComponent& player, BulletRigidBodyComponent& rbc) {
			playerUpdateSystem(e, irr, player, rbc);
			});
	}

	while (accumulator >= dt && !m_finishingAnim && !m_startingAnim) {
		
		try {
			game_world->progress(dt);
		}
		catch (...) {
			baedsLogger::errLog("ECS world step failed.\n");
		}
		soundSystem();

		f32 deltaTime = dt;

		auto snscb = game_world->filter<SensorComponent, SensorCallbackComponent>();
		//game_world->defer_suspend();
		snscb.each([deltaTime](flecs::entity e, SensorComponent& sns, SensorCallbackComponent& cb) {
			if (!e.is_alive()) return;
			if (cb.callback) cb.callback.get()->affect(&sns, deltaTime);
		});
		//game_world->defer_resume();

		for (auto& gun : m_gunsToFire) {
			//todo: thread this
			if (!gun.is_alive()) continue;
			if (!gun.has<WeaponInfoComponent>() || !gun.has<PowerComponent>() || !gun.has<WeaponFiringComponent>()) {
				baedsLogger::errLog("Gun set to fire lacking components! " + entDebugStr(gun) + "\n");
				continue;
			}
			auto info = gun.get_mut<WeaponInfoComponent>();
			auto power = gun.get_mut<PowerComponent>();
			auto firing = gun.get_mut<WeaponFiringComponent>();
			info->fire(info, firing, power, gun, dt, nullptr);
		}
		m_gunsToFire.clear();

		for (auto& shot : m_networkGunsToFire) {
			if (stateController->networkToEntityDict.find(shot.firingWeapon) == stateController->networkToEntityDict.end()) {
				baedsLogger::errLog("No weapon exists for network id " + std::to_string(shot.firingWeapon) + "\n");
				continue;
			}
			flecs::entity gun = flecs::entity(stateController->networkToEntityDict.at(shot.firingWeapon));
			if (!gun.is_alive()) {
				baedsLogger::errLog("Weapon for network id " + std::to_string(shot.firingWeapon) + " is dead.\n");
				stateController->networkToEntityDict.erase(shot.firingWeapon);
				destroyNetworkId(shot.firingWeapon); //free it up
				continue;
			}
			if (!gun.has<WeaponInfoComponent>() || !gun.has<PowerComponent>() || !gun.has<WeaponFiringComponent>()) {
				baedsLogger::errLog("Networked gun set to fire lacking components! " + entDebugStr(gun) + "\n");
				continue;
			}
			auto info = gun.get_mut<WeaponInfoComponent>();
			auto power = gun.get_mut<PowerComponent>();
			auto firing = gun.get_mut<WeaponFiringComponent>();
			if(info->fire(info, firing, power, gun, dt, &shot))
				audioDriver->playGameSound(gun, info->fireSound);
		}
		m_networkGunsToFire.clear();

		for (auto& ent : m_markedForDeath) {
			destroyObject_real(ent);
		}
		m_markedForDeath.clear();

		for (auto& func : m_shipsToSpawn) {
			func();
		}
		m_shipsToSpawn.clear();

		if (isNetworked()) {
			for (auto& inst : m_netDamageToApply) {
				flecs::entity ent = getEntityFromNetId(inst.to);
				if (ent == INVALID_ENTITY)
					continue;
				if (!ent.has<HealthComponent>())
					continue;

				//its completely acceptable to have an instance from a dead entity here because bullets can travel after a guy dies
				ent.get_mut<HealthComponent>()->registerDamageInstance(
					DamageInstance(getEntityFromNetId(inst.from), ent, inst.type, inst.amount, inst.time, inst.hitPos));
			}
			m_netDamageToApply.clear();
		}

		bWorld->checkConstraints(dt);
		applyPopups();
		m_hudUpdate();

		t += dt;
		accumulator -= dt;

		if (mapRunner.run(dt)) {
			clearPlayerHUD();
			mapRunner.endMap();
			m_finishingAnim = true;
		}
	}
	if (m_finishingAnim) {
		m_finishTimer += delta;
		if (m_finishTimer >= 5.f)
			stateController->setState(GAME_FINISHED);
	}
	if (m_startingAnim) {
		m_startTimer += delta;
		if (m_startTimer >= 3.5f) {
			gameController->getPlayer().get<IrrlichtComponent>()->node->removeAnimators();
			initializeHUD();
			device->getCursorControl()->setPosition(vector2di(driver->getScreenSize().Width / 2, driver->getScreenSize().Height / 2));
			if(mapRunner.objective()->hasRadioSpawn())
				mapRunner.radioSignal = createRadioMarker(mapRunner.objectiveStartPos()[0] + vector3df(0.f, -75.f, 0.f), "Radio Signal");
			m_startingAnim = false;
			for (auto& msg : startMsgs) {
				addLargePopup(msg.msg, msg.spkr, msg.banter);
			}
			startMsgs.clear();
		}
	}
}

void GameController::init()
{
	if (open) return;
	stateController->toggleMenuBackdrop(false);
	smgr->clear();
	m_markedForDeath.clear();
	m_gunsToFire.clear();
	m_shipsToSpawn.clear();
	stateController->networkToEntityDict.clear();
	if (m_isNetworked) {
		if (stateController->isHost()) {
			setNetworkIdRange(1, HOST_RANGE);
		}
		else {
			setNetworkIdRange(HOST_RANGE+1, HOST_RANGE + CONNECTED_RANGE);
		}
	}
	else {
		setNetworkIdRange(1, HOST_RANGE);
	}

	FIRED_BY = INVALID_ENTITY;
	DO_NOT_COLLIDE_WITH = INVALID_ENTITY;

	m_timeSinceScenarioStart = 0.f;

	then = device->getTimer()->getTime();

	priorityAccumulator = std::vector<std::shared_ptr<AccumulatorEntry>>();
	packet = new Packet();

	if (stateController->inCampaign) currentScenario = campaign->getSector()->getCurrentScenario();
	else currentScenario = m_dummyScenario;
	//bullet init
	broadPhase = new bt32BitAxisSweep3(btVector3(-200000, -200000, -200000), btVector3(200000, 200000, 200000));
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfig);
	solver = new btSequentialImpulseConstraintSolverMt();
	gPairCb = new btGhostPairCallback();
	solverPool = new btConstraintSolverPoolMt(32);
	//smgr->setLightManager(lightmanager);
	bWorld = new BulletPhysicsWorld(dispatcher, broadPhase, solverPool, solver, collisionConfig);
	bWorld->setGravity(btVector3(0, 0, 0));
	bWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(gPairCb);

	auto oldTasker = taskScheduler;

	taskScheduler = btCreateDefaultTaskScheduler();
	btSetTaskScheduler(taskScheduler);

	if (oldTasker) delete oldTasker;

#if defined(_DEBUG) && defined(INCLUDE_DEBUGGING_LINES)
	rend.setController(stateController);
	rend.setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	bWorld->setDebugDrawer(&rend);
#endif 

	collCb = new broadCallback();
	bWorld->getPairCache()->setOverlapFilterCallback(collCb);

	//ecs init
	game_world = new flecs::world();
	registerComponents(); 
	registerSystems();
	registerRelationships();
	m_finishingAnim = false;
	m_finishTimer = 0.f;
	m_startingAnim = true;
	m_startTimer = 0.f;

	musicChangeCurCooldown = musicChangeCooldown;
	audioDriver->setMusicGain(0, 0.f);
	audioDriver->setMusicGain(1, 1.f);
	open = true;
}

bool clientOwnsThisEntity(const flecs::entity& ent)
{
	if (!ent.is_alive()) {
		baedsLogger::errLog("Trying to check ownership of a dead entity.\n");
		return false;
	}
	if (!ent.has<NetworkingComponent>()) {
		baedsLogger::errLog("Ownership check does not have networking component on entity " + entDebugStr(ent) + "\n");
		return false;
	}
	auto net = ent.get<NetworkingComponent>();
	return networkIdInMyRange(net->networkedId);
}

void GameController::registerComponents()
{
	game_world->component<AIComponent>();
	game_world->component<BulletGhostComponent>();
	game_world->component<BulletRigidBodyComponent>();
	game_world->component<HealthComponent>();
	game_world->component<IrrlichtComponent>();
	game_world->component<ObstacleComponent>();
	game_world->component<InputComponent>();
	game_world->component<PlayerComponent>();
	game_world->component<HangarComponent>();
	game_world->component<FactionComponent>();
	game_world->component<SensorComponent>();
	game_world->component<ShipComponent>();
	game_world->component<ShipParticleComponent>();
	game_world->component<BolasInfoComponent>();
	game_world->component<MissileInfoComponent>();
	game_world->component<WeaponInfoComponent>();
	game_world->component<WeaponFiringComponent>();
	game_world->component<ProjectileInfoComponent>();
	game_world->component<ThrustComponent>();
	game_world->component<HardpointComponent>();
	game_world->component<SensorCallbackComponent>();
	game_world->component<StationModuleComponent>();
	game_world->component<StationModuleOwnerComponent>();
	game_world->component<TurretHardpointComponent>();
	game_world->component<PowerComponent>();
	game_world->component<StatusEffectComponent>();
	game_world->component<PhysShieldComponent>();
	if (m_isNetworked) {
		game_world->component<NetworkingComponent>();
	}
}
void GameController::registerSystems()
{

/*
*	flecs::OnLoad
*	flecs::PostLoad
*	flecs::PreUpdate
*	flecs::OnUpdate
*	flecs::OnValidate
*	flecs::PostUpdate
*	flecs::PreStore
*	flecs::OnStore
*/

	baedsLogger::log("Registering systems for flecs\n");
	auto sys = game_world->system<InputComponent, HardpointComponent, ShipComponent, ThrustComponent, PlayerComponent, BulletRigidBodyComponent, IrrlichtComponent, SensorComponent>()
		.no_readonly().kind(flecs::PreUpdate).iter(shipControlSystem);
	baedsLogger::log("SHIP CONTROL: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<AIComponent, IrrlichtComponent, BulletRigidBodyComponent, ThrustComponent, HardpointComponent, SensorComponent, HealthComponent>()
		.no_readonly().kind(flecs::PreUpdate).iter(AIUpdateSystem);
	baedsLogger::log("AI UPDATES: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<WeaponInfoComponent, WeaponFiringComponent, IrrlichtComponent, PowerComponent>().kind(flecs::OnUpdate).iter(weaponFiringSystem);
	baedsLogger::log("WEAPON FIRING: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<HangarComponent, IrrlichtComponent, FactionComponent>().kind(flecs::OnUpdate).iter(hangarSystem);
	baedsLogger::log("HANGAR / SHIP LAUNCHES: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<SensorComponent>().kind(flecs::OnUpdate).iter(sensorSystem);
	baedsLogger::log("SENSOR: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<HealthComponent>().no_readonly().kind(flecs::OnUpdate).iter(damageSystem);
	baedsLogger::log("DAMAGE: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<BulletRigidBodyComponent, ProjectileInfoComponent, IrrlichtComponent>().kind(flecs::OnUpdate).iter(projectileSystem);
	baedsLogger::log("PROJECTILES / RANGE: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<ThrustComponent, ShipComponent, BulletRigidBodyComponent, IrrlichtComponent, ShipParticleComponent, PowerComponent>().kind(flecs::OnUpdate).iter(shipUpdateSystem);
	baedsLogger::log("SHIP UPDATE: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<ThrustComponent, HealthComponent, BulletRigidBodyComponent, IrrlichtComponent>().kind(flecs::OnUpdate).iter(thrustSystem);
	baedsLogger::log("THRUST: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<PowerComponent>().kind(flecs::OnUpdate).iter(powerSystem);
	baedsLogger::log("POWER: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<StatusEffectComponent>().kind(flecs::PostUpdate).no_readonly().iter(statusEffectSystem);
	baedsLogger::log("STATUS EFFECTS: " + std::to_string(sys.id()) + "\n");

	sys = game_world->system<HealthComponent>().kind(flecs::PostUpdate).iter(healthSystem);
	baedsLogger::log("HEALTH: " + std::to_string(sys.id()) + "\n");

	if (m_isNetworked) {
		sys = game_world->system<NetworkingComponent>().kind(flecs::PostUpdate).no_readonly().iter(networkingSystem);
		baedsLogger::log("NETWORKING: " + std::to_string(sys.id()) + "\n");
	}

	sys = game_world->system<BulletRigidBodyComponent>().kind(flecs::OnUpdate).iter(timeKeepingSystem);
	baedsLogger::log("SPAWN INVULNERABLE: " + std::to_string(sys.id()) + "\n");
}

void GameController::registerRelationships()
{
	FIRED_BY = game_world->entity();
	DO_NOT_COLLIDE_WITH = game_world->entity();
}

void GameController::close()
{
	if (!open) return;

	clearPlayerHUD();

	m_networkMarkedForDeath.clear();
	m_markedForDeath.clear();
	m_gunsToFire.clear();
	m_shipsToSpawn.clear();
	m_netDamageToApply.clear();
	networkDamageToSend.clear();

	FIRED_BY = INVALID_ENTITY;
	DO_NOT_COLLIDE_WITH = INVALID_ENTITY;


	auto f = game_world->filter<>();
	f.each([](flecs::entity e) {
		destroyObject_real(e, true);
	});
	try {
		game_world->quit();
		delete game_world;
	}
	catch (...) {
		baedsLogger::errLog("Flecs screwed up deleting itself\n");
	}
	smgr->clear();

	for (auto& reg : bWorld->ECS_activeConstraints) {
		delete reg.constraint;
	}
	bWorld->clearObjects();
	delete broadPhase;
	delete collisionConfig;
	delete dispatcher;
	delete solver;
	delete bWorld; //this likely leaks some memory
	bWorld = nullptr;
	delete collCb;
	delete gPairCb;
	delete solverPool;
	//delete taskScheduler;

	delete packet; 

	//todo: need to clean out the ECS
	assets->clearGameAssets();
	m_popups.clear();
	ctcts.clear();
	popups.clear();
	deathCallbacks.clear();
	hitCallbacks.clear();
	stateController->networkToEntityDict.clear();
	clearAllNetworkIds();

	lmgr->setGlobal(nullptr);

	playerEntity = INVALID_ENTITY;
	chaosTheory = INVALID_ENTITY;
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		wingmen[i] = INVALID_ENTITY;
		wingmenDisengaged[i] = false;
	}
	device->getCursorControl()->setActiveIcon(ECI_NORMAL);

	audioDriver->clearFades();
	audioDriver->gainTrack(0, .65f, 1.f);
	musicChangeCurCooldown = musicChangeCooldown;
	open = false;
	m_inCombat = false;
}

void GameController::triggerCombatMusic()
{
	if (m_inCombat) {
		musicChangeCurCooldown = 0.f;
		return;
	}
	if (musicChangeCurCooldown < musicChangeCooldown) return;
	audioDriver->fadeTracks(1, 0, .65f);
	m_inCombat = true;
	musicChangeCurCooldown = 0.f;
}

void GameController::triggerAmbientMusic()
{
	if (!m_inCombat) {
		return;
	}
	if (musicChangeCurCooldown < musicChangeCooldown) return;
	audioDriver->fadeTracks(0, 1, .65f);
	m_inCombat = false;
	musicChangeCurCooldown = 0.f;
}

void GameController::m_hudUpdate()
{
	auto playerId = getPlayer();
	if (!playerId.is_alive()) return; //what?
	for (ContactInfo info : getPlayer().get<SensorComponent>()->contacts) { //checks contacts and updates hud if they dont exist
		flecs::entity id = info.ent;
		if (trackedContacts[id] == nullptr && id.is_alive()) {
			if (id.has<IrrlichtComponent>()) {
				if (id.get<IrrlichtComponent>()->node->getID() == ID_IsNotSelectable) continue;
			}
			HUDContact* ct = new HUDContact(rootHUD, id, playerId);
			//player->contacts.push_back(ct);
			trackedContacts[id] = ct;
		}
	}
	for (auto it = trackedContacts.begin(); it != trackedContacts.end(); /*no inc*/) {

		//we want the copy here in case we have to delete the sucker
		auto [id, hud] = *it;
		if (!flecs::entity(game_world->get_world(), id).is_alive()) {
			if (hud) {
				m_ctct_mtx.lock();
				delete hud;
				it = trackedContacts.erase(it);
				m_ctct_mtx.unlock();
				continue;
			}
		}

		if (hud) {
			for (auto& ent : mapRunner.objective().get()->getTargets()) {
				if (hud->isObjective) break;
				if (id == ent.id()) {
					hud->setObjective(true, objNameStrings.at(mapRunner.objective().get()->type()));
					break;
				}
			}
			if (!hud->isObjective) hud->setObjective(false);
			if (!hud->isValidContact()) {
				m_ctct_mtx.lock();
				delete hud;
				it = trackedContacts.erase(it);
				m_ctct_mtx.unlock();
			}
		}

		if(it != trackedContacts.end()) ++it;
	}

	resources->updateElement(getPlayer());
	activeSelect->selectTimer += dt;
	activeSelect->updateElement(getPlayer());

	//player->rootHUD->setRelativePosition(rect<s32>(position2di(0, 0), driver->getScreenSize()));

	auto it = popups.begin();
	while (it != popups.end()) {
		auto pop = (*it);
		if (!pop) continue;
		pop->currentDuration += dt;
		pop->updateElement(getPlayer());
		if (pop->currentDuration >= pop->duration) {
			delete pop;
			it = popups.erase(it);
			continue;
		}
		if(it != popups.end())++it;
	}
	auto hazit = hazards.begin();
	while (hazit != hazards.end()) {
		auto haz = (*hazit);
		haz->updateElement(playerId);
		haz->flashTimer += dt;
		haz->timeoutTimer += dt;
		if (!haz->valid()) {
			delete haz;
			hazit = hazards.erase(hazit);
			continue;
		}
		if(hazit != hazards.end())++hazit;
	}
	largePop->curDuration += dt;
	if (largePop->curDuration >= largePop->duration) largePop->curDuration = largePop->duration;
	largePop->updateElement(getPlayer());
	for (auto& [id, elem] : trackedContacts) {
		if (elem) elem->updateElement(getPlayer());
	}
}
void GameController::addContact(flecs::entity ent, bool objective, bool radio)
{
	if (trackedContacts.find(ent) != trackedContacts.end()) {
		return;
	}
	try {
		if (!ent.is_alive()) game_throw("Cannot add dead contact: " + entDebugStr(ent) + "\n");
		m_ctct_mtx.lock();
		HUDContact* ctct = new HUDContact(rootHUD, ent, gameController->getPlayer(), false, true);
		trackedContacts[ent] = ctct;
		m_ctct_mtx.unlock();
	}
	catch (gameException e) {
		baedsLogger::errLog(e.what());
	}

}
void GameController::removeContact(flecs::entity contact)
{
	m_ctct_mtx.lock();
	if (trackedContacts.find(contact) == trackedContacts.end()) {
		m_ctct_mtx.unlock();
		return;
	}
	delete trackedContacts.at(contact);
	trackedContacts.erase(contact);
	m_ctct_mtx.unlock();
}

void GameController::clearRadioContacts()
{
	m_ctct_mtx.lock();
	for (auto& [id, elem] : trackedContacts) {
		if(elem) elem->isRadio = false;
	}
	m_ctct_mtx.unlock();
}

const bool GameController::contactTracked(flecs::entity ent)
{
	if (trackedContacts.find(ent) != trackedContacts.end()) return true;
	return false;
}

HUDContact* GameController::getContact(flecs::entity ent)
{
	if (!contactTracked(ent)) return nullptr;
	return trackedContacts.at(ent);
}

HUDHazard* GameController::addHazard(flecs::entity ent, HAZARD which)
{
	HUDHazard* newHaz = new HUDHazard(rootHUD, ent, which, hazards.size());
	hazards.push_back(newHaz);
	audioDriver->playGameSound(getPlayer(), "hazard_notif.ogg", .8f, 200.f);
	return newHaz;
}

HUDHazard* GameController::getHazard(flecs::entity ent, HAZARD which)
{
	for (auto haz : hazards) {
		if (haz->cause == ent) return haz;
		if (which != HAZARD::NONE && haz->which == which) return haz;
	}
	return nullptr;
}

const HUDActiveSelection* GameController::selectHUD()
{ 
	return activeSelect; 
}

static u32 _whichHp(vector3df pos)
{
	bool behind = false;
	position2di coords = screenCoords(pos, smgr->getActiveCamera(), behind);
	dimension2du screen = driver->getScreenSize();
	if (coords.X < screen.Width / 3) return 2;
	if (coords.X > (screen.Width / 3) * 2) return 3;
	if (coords.Y < screen.Height / 2) return 0;
	return 1;
}

void GameController::smackShields(vector3df pos)
{
	m_rsrc_mtx.lock();
	resources->hpHit[_whichHp(pos)]->setColor(SColor(255, 60, 150, 255));
	m_rsrc_mtx.unlock();
}
void GameController::smackHealth(vector3df pos)
{
	m_rsrc_mtx.lock();
	resources->hpHit[_whichHp(pos)]->setColor(SColor(255, 255, 50, 50));
	m_rsrc_mtx.unlock();
}

void GameController::initializeHUD()
{

	dimension2du baseSize = dimension2du(960, 540);
	rootHUD = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), driver->getScreenSize()));
	scaleAlign(rootHUD);

	activeSelect = new HUDActiveSelection(rootHUD);
	resources = new HUDResources(rootHUD, getPlayer());
	largePop = new HUDLargePopup(rootHUD);
}

void GameController::addPopup(std::string spkr, std::string msg)
{
	//std::cout << "adding popup at slot " << popups.size() << std::endl;
	auto pop = new HUDPopup(rootHUD, (s32)popups.size(), msg, spkr);
	popups.push_back(pop);
}
void GameController::addPopup(std::wstring spkr, std::wstring msg)
{
	auto pop = new HUDPopup(rootHUD, (s32)popups.size(), wstrToStr(msg), wstrToStr(spkr));
	popups.push_back(pop);
}

void GameController::addLargePopup(std::string msg, std::string spkr, bool banter)
{
	if (startAnim()) {
		startMsgs.push_back({ spkr, msg, banter });
		return;
	}
	largePop->showMsg(msg, spkr, banter);
}
void GameController::addLargePopup(std::wstring msg, std::wstring spkr, bool banter)
{
	if (startAnim()) {
		startMsgs.push_back({ wstrToStr(spkr), wstrToStr(msg), banter });
		return;
	}
	largePop->showMsg(wstrToStr(msg), wstrToStr(spkr), banter);
}

void GameController::setPlayer(flecs::entity_t id)
{
	playerEntity = id;
}
flecs::entity GameController::getPlayer()
{
	return flecs::entity(game_world->get_world(), playerEntity);
}
void GameController::setChaosTheory(flecs::entity_t id)
{
	chaosTheory = id;
}
flecs::entity GameController::getChaosTheory()
{
	return flecs::entity(game_world->get_world(), chaosTheory);
}

void GameController::setWingman(flecs::entity_t id, u32 slot)
{
	wingmen[slot] = id;
	wingmenDisengaged[slot] = false;
}
flecs::entity GameController::getWingman(u32 slot)
{
	return flecs::entity(game_world->get_world(), wingmen[slot]);
}

void GameController::disengageWingman(flecs::entity id)
{
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		if (wingmen[i] == id) {
			wingmen[i] = INVALID_ENTITY;
			wingmenDisengaged[i] = true;
		}
	}
}

void GameController::clearPlayerHUD()
{	
	for (auto& [id, hud] : trackedContacts) {
		if (hud) delete hud;
	}
	trackedContacts.clear();
	if(resources) delete resources;
	if(activeSelect) delete activeSelect;
	if(largePop) delete largePop;
	if (rootHUD) rootHUD->remove();
	resources = nullptr;
	activeSelect = nullptr;
	largePop = nullptr;
	rootHUD = nullptr;
}

void GameController::applyPopups()
{
	if (!getPlayer().is_alive()) return;
	auto plyr = getPlayer().get_mut<PlayerComponent>();
	for (auto& pair : m_popups) {
		addPopup(pair.first, pair.second);
	}
	m_popups.clear();
}

bool GameController::OnEvent(const SEvent& event)
{
	if (!open) return true;
	if (!getPlayer().is_alive()) return true;
	if(!getPlayer().has<InputComponent>()) return true;
	auto input = getPlayer().get_mut<InputComponent>();
	auto player = getPlayer().get_mut<PlayerComponent>();
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		input->keysDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
		if(event.KeyInput.Key == cfg->keys.key[IN_TOGGLE_MOUSE] && !input->keysDown[cfg->keys.key[IN_TOGGLE_MOUSE]]) {
			input->mouseControlEnabled = !input->mouseControlEnabled;
		}
		if (event.KeyInput.Key == cfg->keys.key[IN_TOGGLE_SAFETY] && !input->keysDown[cfg->keys.key[IN_TOGGLE_SAFETY]]) {
			input->safetyOverride = !input->safetyOverride;
		}
		if (event.KeyInput.Key == cfg->keys.key[IN_REVERSE_CAMERA] && !input->keysDown[cfg->keys.key[IN_REVERSE_CAMERA]]) {
			input->usingReverseCamera = !input->usingReverseCamera;
			smgr->setActiveCamera((input->usingReverseCamera) ? player->reverseCamera : player->camera);
		}
		if (event.KeyInput.Key == cfg->keys.key[IN_TOGGLE_ANG_FRICTION] && !input->keysDown[cfg->keys.key[IN_TOGGLE_ANG_FRICTION]]) {
			cfg->game.toggles[GTOG_ANG_SPACE_FRICTION] = !cfg->game.toggles[GTOG_ANG_SPACE_FRICTION];
		}
		if (event.KeyInput.Key == cfg->keys.key[IN_TOGGLE_LIN_FRICTION] && !input->keysDown[cfg->keys.key[IN_TOGGLE_LIN_FRICTION]]) {
			cfg->game.toggles[GTOG_LIN_SPACE_FRICTION] = !cfg->game.toggles[GTOG_LIN_SPACE_FRICTION];
		}
	}
	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		switch(event.MouseInput.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN:
			input->keysDown[KEY_LBUTTON] = true;
			break;
		case EMIE_LMOUSE_LEFT_UP:
			input->keysDown[KEY_LBUTTON] = false;
			break;
		case EMIE_RMOUSE_PRESSED_DOWN:
			input->keysDown[KEY_RBUTTON] = true;
			break;
		case EMIE_RMOUSE_LEFT_UP:
			input->keysDown[KEY_RBUTTON] = false;
			break;
		case EMIE_MMOUSE_PRESSED_DOWN:
			input->keysDown[KEY_MBUTTON] = true;
			break;
		case EMIE_MMOUSE_LEFT_UP:
			input->keysDown[KEY_MBUTTON] = false;
			break;
		case EMIE_MOUSE_MOVED:
			input->mousePixPosition.X = event.MouseInput.X;
			input->mousePixPosition.Y = event.MouseInput.Y;
			input->mousePosition.X = (event.MouseInput.X - ((f32)driver->getScreenSize().Width * .5f)) / ((f32)driver->getScreenSize().Width * .5f);
			input->mousePosition.Y = (event.MouseInput.Y - ((f32)driver->getScreenSize().Height * .5f)) / ((f32)driver->getScreenSize().Height * .5f);
			break;
		default:
			break;
		}
	}
	if (event.EventType == EET_GUI_EVENT) {
		//handle GUI events here, but there probably aren't any that the HUD itself doesn't handle
	}
	return false;
}

void GameController::initScenario()
{
	baedsLogger::log("Scenario being loaded.\n");
	isPlayerAlive = true;

	if (stateController->inCampaign) {
		if (tutorial) {
			campaign->getSector()->buildScenarios();
			campaign->getSector()->selectCurrentScenario(0);
			currentScenario = campaign->getSector()->getCurrentScenario();

			currentScenario->setType(campaign->getDifficulty(), SCENARIO_TUTORIAL);
		}
		if (campaign->getFlag(L"ARNOLD_MISSION_AVAILABLE")) {//override after arnold's mission gets accepted
			baedsLogger::log("Hijacking scenario for Arnold.\n");
			currentScenario->setType(campaign->getDifficulty(), SCENARIO_ARNOLD);
			//m_runner.build(SCENARIO_ARNOLD, currentScenario->environment(), true);
			campaign->setFlag(L"ARNOLD_MISSION_AVAILABLE", false);
			campaign->setFlag(L"ARNOLD_MISSION_COMPLETED");
			campaign->setFlag(L"EVENT_AVAILABLE");
		}

		mapRunner.build(currentScenario->type(), currentScenario->environment(), true);
	}
	else {
		GigaPacket kingPacket;
		Packet finPacket;
		finPacket.packetType = PTYPE_SCENARIO_GEN_DONE;
		mapRunner.build(m_dummyScenario->type(), m_dummyScenario->environment()); //build line moved to host connection when we have the wait
		baedsLogger::log("Scenario built on this end.\n");
		if (stateController->listenSocket) {
			baedsLogger::log("I am the host; sending off my data...\n");
			//send packets
			auto& ships = mapRunner.mapShips();
			kingPacket.packetType = PTYPE_SCENARIO_GEN;
			size_t location = 0;

			for (auto& ship : ships) {
				ScenarioGenPacketData* dat = reinterpret_cast<ScenarioGenPacketData*>(reinterpret_cast<std::byte*>(&kingPacket.data) + location);
				dat->type = PSG_SHIP;
				bitpacker::store<MapGenShip&>(byte_span(dat->data, MAP_GEN_SHIP_BYTES_NEEDED), 0, ship);
				location += sizeof(ScenarioGenPacketData) + MAP_GEN_SHIP_BYTES_NEEDED;
				++kingPacket.numEntries;
			}
			auto& obstacles = mapRunner.mapObstacles();
			for (auto& obstacle : obstacles) {
				ScenarioGenPacketData* dat = reinterpret_cast<ScenarioGenPacketData*>(reinterpret_cast<std::byte*>(&kingPacket.data) + location);
				dat->type = PSG_OBSTACLE;
				bitpacker::store<MapGenObstacle&>(byte_span(dat->data, MAP_GEN_OBSTACLE_BYTES_NEEDED), 0, obstacle);
				location += sizeof(ScenarioGenPacketData) + MAP_GEN_OBSTACLE_BYTES_NEEDED;
				++kingPacket.numEntries;
			}
			for (auto& client : stateController->clientData) {
				if (client.active) {
					auto result = sendGigaPacket(client.connection, &kingPacket, k_nSteamNetworkingSend_Reliable);
				}
			}

			baedsLogger::log("Done sending my data.\n");

			//await packets from initialized other connections
			std::vector<MapGenShip> generatedShips;
			std::vector<MapGenObstacle> generatedObstacles;
			bool waiting = true;
			baedsLogger::log("Awaiting client ship generation...\n");
			while (waiting) {
				for (auto& client : stateController->clientData) {
					if (client.finishedLoading || !client.active) continue;
					GigaPacket packet = getGigaPacket(client.connection);
					if (packet.packetType == PTYPE_SCENARIO_GEN) {
						size_t location = 0;
						client.finishedLoading = true;
						for (u32 i = 0; i < packet.numEntries; ++i) {
							ScenarioGenPacketData* dat = reinterpret_cast<ScenarioGenPacketData*>(reinterpret_cast<std::byte*>(&packet.data) + location);
							if (dat->type == PSG_SHIP) {
								baedsLogger::log("We have a ship!\n");
								MapGenShip ship;
								bitpacker::get<>(const_byte_span(dat->data, MAP_GEN_SHIP_BYTES_NEEDED), 0, ship);
								generatedShips.push_back(ship);
								location += sizeof(ScenarioGenPacketData) + MAP_GEN_SHIP_BYTES_NEEDED;
							}
							else if (dat->type == PSG_OBSTACLE)
							{
								// clients probably shouldn't be sending obstacles but this is here for completeness
								MapGenObstacle obstacle;
								bitpacker::get<>(const_byte_span(dat->data, MAP_GEN_OBSTACLE_BYTES_NEEDED), 0, obstacle);
								generatedObstacles.push_back(obstacle);
								location += sizeof(ScenarioGenPacketData) + MAP_GEN_OBSTACLE_BYTES_NEEDED;
							}
						}
					}
				}
				waiting = false;
				for (auto& client : stateController->clientData) {
					if (client.active && !client.finishedLoading) {
						waiting = true;
						break;
					}
				}
			}
			baedsLogger::log("Clients done loading ships. Assembling ships...\n");
			//build other client ships
			for (auto& ship : generatedShips) {
				createShipFromMapGen(ship);
			}
			for (auto& obst : generatedObstacles) {
				//build obstacles
				createObstacleFromMapGen(obst);
			}
			baedsLogger::log("Done. Sending off final packet.\n");
			for (auto& client : stateController->clientData) {
				if (client.active) sendPacket(client.connection, &finPacket, k_nSteamNetworkingSend_Reliable);
			}
		}
		else {
			baedsLogger::log("I am a client, awaiting host data...\n");
			//wait for packets
			std::vector<MapGenShip> generatedShips;
			std::vector<MapGenObstacle> generatedObstacles;
			
			bool waiting = true;
			while (waiting) {
				GigaPacket packet = getGigaPacket(stateController->hostConnection);
				if (packet.packetType == PTYPE_SCENARIO_GEN) {
					waiting = false;
					size_t location = 0;
					for (u32 i = 0; i < packet.numEntries; ++i) {
						ScenarioGenPacketData* dat = reinterpret_cast<ScenarioGenPacketData*>(reinterpret_cast<std::byte*>(&packet.data) + location);
						if (dat->type == PSG_SHIP) {
							MapGenShip ship;
							baedsLogger::log("We have a ship!\n");
							bitpacker::get<>(const_byte_span(dat->data, MAP_GEN_SHIP_BYTES_NEEDED), 0, ship);
							generatedShips.push_back(ship);
							location += sizeof(ScenarioGenPacketData) + MAP_GEN_SHIP_BYTES_NEEDED;
						}
						else if (dat->type == PSG_OBSTACLE)
						{
							MapGenObstacle obstacle;
							bitpacker::get<>(const_byte_span(dat->data, MAP_GEN_OBSTACLE_BYTES_NEEDED), 0, obstacle);
							generatedObstacles.push_back(obstacle);
							location += sizeof(ScenarioGenPacketData) + MAP_GEN_OBSTACLE_BYTES_NEEDED;
						}
					}
				}
			}
			baedsLogger::log("Done grabbing host data, assembling ships...\n");
			//build from packets
			for (auto& ship : generatedShips) {
				createShipFromMapGen(ship);
			}
			for (auto& obst : generatedObstacles) {
				//build obstacles
				createObstacleFromMapGen(obst);
			}

			//build this player's ship, send off packet
			baedsLogger::log("Done building scenario, sending off my own ship...\n");
			auto& ships = mapRunner.mapShips();
			GigaPacket loadPacket;
			loadPacket.packetType = PTYPE_SCENARIO_GEN;
			size_t location = 0;
			for (auto& ship : ships) {
				ScenarioGenPacketData* dat = reinterpret_cast<ScenarioGenPacketData*>(reinterpret_cast<std::byte*>(&loadPacket.data) + location);
				dat->type = PSG_SHIP;
				bitpacker::store<MapGenShip&>(byte_span(dat->data, MAP_GEN_SHIP_BYTES_NEEDED), 0, ship);
				location += sizeof(ScenarioGenPacketData) + MAP_GEN_SHIP_BYTES_NEEDED;
				++loadPacket.numEntries;
			}
			sendGigaPacket(stateController->hostConnection, &loadPacket, k_nSteamNetworkingSend_Reliable);
			baedsLogger::log("Done. Waiting for the host to finish loading...\n");
			bool hostLoading = true;
			while (hostLoading) {
				auto packets = getPackets(stateController->hostConnection);
				if (packets.empty()) continue;

				for (auto& packet : packets) {
					if (packet.packetType == PTYPE_SCENARIO_GEN_DONE) {
						//we are done here
						hostLoading = false;
						break;
					}
				}
			}
			baedsLogger::log("Host says they're done.\n");
		}
	}
	device->getCursorControl()->setActiveIcon(stateController->crosshair());
	baedsLogger::log("Scenario loaded and ready for play.\n");
}