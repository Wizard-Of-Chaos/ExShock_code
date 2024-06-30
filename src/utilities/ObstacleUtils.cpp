#include "ObstacleUtils.h"
#include "GameController.h"
#include "BulletGhostComponent.h"
#include "BulletRigidBodyComponent.h"
#include "btUtils.h"
#include "SensorComponent.h"
#include "GameStateController.h"
#include "ObstacleAI.h"
#include "LargeShipUtils.h"
#include "SensorCallbackComponent.h"
#include "FadeInParticleAffector.h"
#include "NodeAnimators.h"
#include "TurretUtils.h"
#include "AttributeReaders.h"
#include "Campaign.h"
#include "AudioDriver.h"
#include "GameAssets.h"
#include "ShipUtils.h"
#include "IrrlichtComponent.h"
#include "GameFunctions.h"
#include "CrashLogger.h"
#include "NetworkingComponent.h"
#include "StationModuleUtils.h"
#include "Shaders.h"

flecs::entity createDynamicObstacle(u32 id, vector3df position, vector3df rotation, vector3df scale, f32 mass, f32 startLinVel, f32 startRotVel, bool startActivated, NetworkId net)
{
	auto obstacle = game_world->entity();

	loadObstacle(id, obstacle);
	auto irr = obstacle.get_mut<IrrlichtComponent>();
	if (!irr->node) {
		baedsLogger::errLog("Node unavailable for dynamic obstacle creation!\n");
		return INVALID_ENTITY;
	}
	irr->node->setID(ID_IsSelectable);
	irr->node->setPosition(position);
	irr->node->setScale(scale);
	irr->node->setMaterialFlag(EMF_NORMALIZE_NORMALS, true);
	irr->node->setRotation(rotation);
	if (startLinVel == 0.f && startRotVel == 0.f) {
		IMeshSceneNode* n = (IMeshSceneNode*)irr->node;
		n->getMesh()->setHardwareMappingHint(EHM_STATIC);
	}
	btVector3 btscale(irrVecToBt(scale));
	initializeBtConvexHull(obstacle, assets->getHull(obstacleData[id]->name), btscale, mass, startLinVel, startRotVel, net);
	auto rbc = obstacle.get_mut<BulletRigidBodyComponent>();
	if(startLinVel == 0 && startRotVel == 0) rbc->rigidBody->setActivationState(0);
	if (startActivated) rbc->rigidBody->setActivationState(1);
	//initializeHealth(obstacle, obstacleData[id]->health);

	return obstacle;

}
flecs::entity createStaticObstacle(u32 id, vector3df position, vector3df rotation, vector3df scale, NetworkId net)
{
	return createDynamicObstacle(id, position, rotation, scale, 0, 0, 0, false, net);
}

flecs::entity createAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 mass, f32 startVel, f32 startRotVel)
{
	u32 which = random.unum(3);
	which += 12;
	auto roid = createDynamicObstacle(which, position, rotation, scale, mass, startVel, startRotVel);
	return roid;
}

flecs::entity createMoneyAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel, f32 startRotVel, NetworkId net)
{
	auto roid = createDynamicObstacle(22, position, rotation, scale, scale.X, startVel, startRotVel, net);
	gameController->registerDeathCallback(roid, moneySplitExplosion);
	return roid;
}

flecs::entity createCashNugget(vector3df position, vector3df rotation, vector3df scale, f32 startVel, f32 startRotVel, NetworkId net)
{
	auto cash = createDynamicObstacle(41, position, rotation, scale, scale.X, startVel, startRotVel, true, net);
	gameController->registerDeathCallback(cash, supplyBoxDeath);
	AIComponent ai;
	auto mineAI = new MineAI();
	mineAI->self = cash;
	ai.aiControls = std::shared_ptr<AIType>(mineAI);
	ThrustComponent thrust;
	HardpointComponent hards;
	cash.set<AIComponent>(ai);
	cash.set<ThrustComponent>(thrust);
	cash.set<HardpointComponent>(hards);
	initializeSensors(cash, 175.f, DEFAULT_SENSOR_UPDATE_INTERVAL, true);
	initializeNeutralFaction(cash);
	return cash;
}

flecs::entity createIceAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel, f32 startRotVel, bool split, NetworkId networkId)
{
	auto roid = createDynamicObstacle(34, position, rotation, scale, scale.X, startVel, startRotVel, false, networkId);
	auto hp = roid.get_mut<HealthComponent>();

	auto irr = roid.get<IrrlichtComponent>();
	IParticleSystemSceneNode* ps = smgr->addParticleSystemSceneNode(true, irr->node);
	ps->setVisible(true);

	auto em = ps->createSphereEmitter(vector3df(0, 0, 0), scale.X / 25, vector3df(0, .0005f, 0),
		15, 45, SColor(0, 40, 120, 110), SColor(0, 80, 240, 220), 2500, 4500, 360,
		dimension2df(scale.X * 2, scale.X * 2), dimension2df(scale.X * 6, scale.X * 6));
	ps->setEmitter(em);
	auto animator = getTextureAnim("assets/effects/ice_cloud/", 30, true);
	ps->addAnimator(animator);
	animator->drop();
	ps->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);
	ps->setMaterialFlag(EMF_LIGHTING, false);
	IParticleAffector* paf = ps->createFadeOutParticleAffector();
	ps->addAffector(paf);
	paf->drop();
	paf = new CParticleFadeInAffector(SColor(0, 0, 0, 0), 1000U);
	ps->addAffector(paf);
	paf->drop();
	em->drop();

	if (split) gameController->registerDeathCallback(roid, iceSplitExplosion);

	return roid;
}

flecs::entity createIceSpikeAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel, f32 startRotVel)
{
	auto roid = createDynamicObstacle(42, position, rotation, scale, scale.X, startVel, startRotVel);
	gameController->registerDeathCallback(roid, iceSplitExplosion);
	return roid;
}

flecs::entity createHugeAsteroid(vector3df position, vector3df rotation, vector3df scale)
{
	auto roid = createStaticObstacle(16, position, rotation, scale);
	return roid;
}

flecs::entity createRadioactiveAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 startVel, f32 startRotVel, NetworkId net)
{
	auto roid = createDynamicObstacle(11, position, rotation, scale, scale.X, startVel, startRotVel, net);

	auto irr = roid.get_mut<IrrlichtComponent>();
	
	auto node = smgr->addBillboardSceneNode(irr->node, dimension2df(scale.X * 15.5f, scale.X * 15.5f), vector3df(0.f), ID_IsNotSelectable);
	node->setMaterialFlag(EMF_LIGHTING, false);
	node->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
	node->setMaterialFlag(EMF_FOG_ENABLE, false);
	auto anim = getTextureAnim("assets/effects/radiation/", 28, true);
	node->addAnimator(anim);
	anim->drop();

	initializeSensors(roid, scale.X * 12.5f, DEFAULT_SENSOR_UPDATE_INTERVAL * 1.5f);
	SensorCallbackComponent sensCb;
	RadioactiveDamageCallback* cb = new RadioactiveDamageCallback();
	cb->self = roid;
	cb->selfPos = irrVecToBt(position);

	sensCb.callback = std::shared_ptr<SensorCallback>(cb);
	roid.set<SensorCallbackComponent>(sensCb);

	gameController->registerDeathCallback(roid, fuelDeathExplosion);
	return roid;
}

void createAsteroidSwarm(vector3df position, vector3df rotation, vector3df scale, f32 mass, f32 startVel, f32 startRotVel)
{
	u32 num = random.urange(20U,40U);
	for (u32 i = 0; i < num; ++i) {
		vector3df rot = makeRotationVectorInaccurate(rotation, 100.f);

		vector3df pos = getPointInSphere(position, 1800.f);
		createAsteroid(pos, rot, scale, mass, startVel, startRotVel);
	}
}

flecs::entity createEngineDebris(vector3df position, vector3df rotation, vector3df scale, f32 mass, NetworkId id)
{
	auto engine = createDynamicObstacle(5, position, rotation, scale, mass, 0.f, 0.f, false, id);
	gameController->registerHitCallback(engine, onHitDebrisEngineTakeoff);
	ThrustComponent thrust;
	//thrust.ambient = audioDriver->playGameSound(engine, "hazard_broken_idle.ogg", 1.f, 20.f, 1000.f, true);
	engine.set<ThrustComponent>(thrust);
	return engine;
}
flecs::entity createSupplyBox(vector3df position, vector3df rotation, vector3df scale)
{
	auto box = createDynamicObstacle(7, position, rotation, scale, 100.f, 0.f, .2f);
	gameController->registerDeathCallback(box, supplyBoxDeath);
	AIComponent ai;
	auto mineAI = new MineAI();
	mineAI->self = box;
	ai.aiControls = std::shared_ptr<AIType>(mineAI);
	ThrustComponent thrust;
	HardpointComponent hards;
	box.set<AIComponent>(ai);
	box.set<ThrustComponent>(thrust);
	box.set<HardpointComponent>(hards);
	initializeNeutralFaction(box);
	initializeSensors(box, 100.f, DEFAULT_SENSOR_UPDATE_INTERVAL, true);
	return box;
}
flecs::entity createFuelTank(vector3df position, vector3df rotation, vector3df scale)
{
	auto tank = createDynamicObstacle(8, position, rotation, scale, 15.f);
	gameController->registerDeathCallback(tank, fuelDeathExplosion);
	return tank;
}

flecs::entity createBeacon(vector3df pos, vector3df rot, vector3df scale)
{
	auto beacon = createDynamicObstacle(32, pos, rot, scale, 900.f);
	auto light = createLightWithVolumeMesh(16, 2, 800.f, SColor(15, 255, 255, 255), SColor(0, 0, 255, 0), beacon.get<IrrlichtComponent>()->node);
	CSceneNodeScalePulseAnimator* anim = new CSceneNodeScalePulseAnimator(device->getTimer()->getTime(), vector3df(.5f), vector3df(6.f), 10000U);
	light->addAnimator(anim);
	anim->drop();
	return beacon;
}

flecs::entity createDeadShipAsObstacle(dataId which, vector3df pos, vector3df rot, dataId wepId)
{
	auto ent = createShip(which, wepId, pos, rot, false);
	std::string name = ent.doc_name();
	name += " [Wreck]";
	ent.set_doc_name(name.c_str());
	//castrate this fucker

	auto hp = ent.get_mut<HealthComponent>();
	hp->maxShields = 0.f;
	hp->shields = 0.f;

	ent.remove<ThrustComponent>();
	ent.remove<ShipComponent>();
	ent.remove<PowerComponent>();
	if (ent.has<TurretHardpointComponent>()) ent.remove<TurretHardpointComponent>();
	if (ent.has<HangarComponent>()) ent.remove<HangarComponent>();
	ent.set<ObstacleComponent>(ObstacleComponent(DEAD_SHIP));
	ent.get_mut<IrrlichtComponent>()->node->setMaterialType(shaders->getShaderMaterial(SHADE_3LIGHT_NORM));
	return ent;
}

flecs::entity createDormantShipAsObstacle(dataId which, vector3df pos, vector3df rot, dataId wepId)
{
	return createShip(which, wepId, pos, rot, false);
}

flecs::entity createWeaponAsObstacle(dataId which, HARDPOINT_TYPE type, vector3df pos, vector3df rot, vector3df scale, NetworkId net, bool sensPickup)
{
	flecs::entity wep = game_world->entity();
	loadWeapon(which, wep, type);

	wep.remove<WeaponFiringComponent>();

	auto irr = wep.get<IrrlichtComponent>();
	irr->node->setScale(scale);
	auto sphere = new btSphereShape(scale.X/3.f);
	btVector3 btScaleInternal = btVector3(1.f, 1.f, 1.f);
	initBtRBC(wep, sphere, btScaleInternal, 50.f, 0.f, 0.f, net);

	ObstacleComponent obst;
	obst.type = DEAD_WEAPON;
	obst.obstacleDataId = which;

	if (sensPickup) {
		gameController->registerDeathCallback(wep, weaponPickupDeath);
		AIComponent ai;
		auto mineAI = new MineAI();
		mineAI->self = wep;
		ai.aiControls = std::shared_ptr<AIType>(mineAI);
		ThrustComponent thrust;
		HardpointComponent hards;
		wep.set<AIComponent>(ai);
		wep.set<ThrustComponent>(thrust);
		wep.set<HardpointComponent>(hards);
		initializeNeutralFaction(wep);
		initializeSensors(wep, 100.f, DEFAULT_SENSOR_UPDATE_INTERVAL, true);
	}

	return wep;
}

flecs::entity createMine(vector3df position, vector3df rotation, vector3df scale, FACTION_TYPE triggeredBy, bool flashing)
{
	auto mine = createDynamicObstacle(9, position, rotation, scale, 5.f);
	AIComponent ai;
	auto mineAI = new MineAI();
	mineAI->self = mine;
	mineAI->triggeredBy = triggeredBy;
	ai.aiControls = std::shared_ptr<AIType>(mineAI);
	ThrustComponent thrust;
	HardpointComponent hards;
	mine.set<AIComponent>(ai);
	mine.set<ThrustComponent>(thrust);
	mine.set<HardpointComponent>(hards);
	gameController->registerDeathCallback(mine, mineExplosion);
	initializeNeutralFaction(mine);
	initializeSensors(mine, 160.f, DEFAULT_SENSOR_UPDATE_INTERVAL, true);

	if (flashing) {
		auto light = createLightWithVolumeMesh(64, 8, 75.f, SColor(10, 255, 255, 255), SColor(0, 255, 0, 0), mine.get<IrrlichtComponent>()->node);
		CSceneNodeScalePulseAnimator* anim = new CSceneNodeScalePulseAnimator(device->getTimer()->getTime(), vector3df(1, 1, 1), vector3df(3, 3, 3), 1200U);
		light->addAnimator(anim);
		anim->drop();
	}

	u32 roll = random.d20();
	if (roll == 20) {
		mine.set_doc_name("WHATISLOSTINTHEMINES");
	}
	//thrust.ambient = audioDriver->playGameSound(mine, "hazard_mine_idle.ogg", 1.f, 40.f, 800.f, true);
	return mine;
}

flecs::entity createDebrisMissile(vector3df position, vector3df rotation, vector3df scale)
{
	auto missile = createDynamicObstacle(10, position, rotation, scale, 1.f);
	AIComponent ai;
	ai.state = AI_STATE_IDLE;
	auto missAI = new MissileAI;
	missAI->self = missile;
	ThrustComponent thrust;
	thrust.forward = 400.f;
	thrust.brake = 300.f;
	thrust.pitch = 300.f;
	thrust.yaw = 300.f;
	thrust.strafe = 300.f;
	ai.aiControls = std::shared_ptr<AIType>(missAI);
	HardpointComponent hards;
	missile.set<AIComponent>(ai);
	missile.set<ThrustComponent>(thrust);
	missile.set<HardpointComponent>(hards);
	gameController->registerDeathCallback(missile, mineExplosion);
	initializeNeutralFaction(missile);
	initializeSensors(missile, 400.f, DEFAULT_SENSOR_UPDATE_INTERVAL, true);
	//thrust.ambient = audioDriver->playGameSound(missile, "hazard_warhead_idle.ogg", 1.f, 40.f, 800.f, true);
	return missile;
}
flecs::entity createDebris(vector3df position, vector3df rotation, vector3df scale, f32 mass)
{
	return createDynamicObstacle(3, position, rotation, scale, mass);
}

flecs::entity createDebrisTurret(vector3df position, vector3df rotation, vector3df scale)
{
	return createTurret(0, 3, position, rotation, FACTION_UNCONTROLLED, scale);
}

flecs::entity createStasisPod(vector3df position, vector3df rotation, NetworkId net)
{
	auto pod = createDynamicObstacle(0, position, rotation, vector3df(2.f, 2.f, 2.f), 300.f, 0.f, .2f, false, net);
	gameController->registerDeathCallback(pod, stasisPodCollect);
	AIComponent ai;
	auto mineAI = new MineAI();
	mineAI->self = pod;
	ai.aiControls = std::shared_ptr<AIType>(mineAI);
	mineAI->triggeredBy = FACTION_PLAYER;
	ThrustComponent thrust;
	HardpointComponent hards;
	pod.set<AIComponent>(ai);
	pod.set<ThrustComponent>(thrust);
	pod.set<HardpointComponent>(hards);
	initializeNeutralFaction(pod);
	initializeSensors(pod, 250.f, DEFAULT_SENSOR_UPDATE_INTERVAL, true);
	return pod;
}

flecs::entity createCaptureableWepPlatform(vector3df position, vector3df rotation, s32 turretId, s32 wepId)
{
	auto sdata = (StationData*)obstacleData[21];
	auto ent = createStaticObstacle(21, position, rotation, vector3df(sdata->scale, sdata->scale, sdata->scale));
	initializeNeutralFaction(ent);
	initializeTurretsOnOwner(ent, turretId, wepId);
	ent.set<PowerComponent>(FREE_POWER_COMPONENT);
	gameController->registerDeathCallback(ent, carrierDeathExplosionCallback);

	initializeSensors(ent, sdata->scale * 10, DEFAULT_SENSOR_UPDATE_INTERVAL);
	SensorCallbackComponent sensCb;
	StationCaptureCallback* cb = new StationCaptureCallback();
	cb->self = ent;
	cb->selfPos = irrVecToBt(position);
	sensCb.callback = std::shared_ptr<SensorCallback>(cb);
	ent.set<SensorCallbackComponent>(sensCb);

	return ent;
}

void createMinefield(std::vector<vector3df> positions, vector3df scale)
{
	for (auto& vec : positions) {
		createMine(vec, randomRotationVector(), scale);
	}
}
void createRandomMinefield(vector3df position, vector3df scale, u32 numMines, f32 radius)
{
	for (u32 i = 0; i < numMines; ++i) {
		createMine(getPointInSphere(position, radius), randomRotationVector(), scale);
	}
}

flecs::entity createRandomDebrisElement(vector3df position, vector3df rotation, vector3df scale)
{
	u32 rand = random.d100();
	if (rand >= 95) {
		return createSupplyBox(position, rotation, vector3df(5.f, 5.f, 5.f));
	}
	else if (rand >= 70) {
		return createMine(position, rotation, vector3df(5.f, 5.f, 5.f));
	}
	else if (rand >= 65) {
		return createDebrisMissile(position, rotation, vector3df(5.f, 5.f, 5.f));
	}
	else if (rand >= 58) {
		return createDebrisTurret(position, rotation, scale);
	}
	else if (rand >= 40) {
		return createEngineDebris(position, rotation, scale, scale.X);
	}
	else if (rand >= 30) { //bridge section
		return createDynamicObstacle(4, position, rotation, scale, scale.X); 
	}
	else if (rand >= 20) { //forward section
		return createDynamicObstacle(6, position, rotation, scale, scale.X);
	}
	return createDebris(position, rotation, scale, scale.X);
}

flecs::entity createRandomShipDebris(vector3df position, vector3df rotation, vector3df scale)
{
	u32 rand = random.d100();
	if (rand >= 75) {
		return createEngineDebris(position, rotation, scale, scale.X);
	}
	else if (rand >= 50) { //bridge section
		return createDynamicObstacle(4, position, rotation, scale, scale.X);
	}
	else if (rand >= 25) { //forward section
		return createDynamicObstacle(6, position, rotation, scale, scale.X);
	}
	return createDebris(position, rotation, scale, scale.X);

}

flecs::entity createExplosiveAsteroid(vector3df position, vector3df rotation, vector3df scale, f32 mass, NetworkId net)
{
	auto id = createDynamicObstacle(2,position, rotation, scale, mass, 0, 0, false, net);
	gameController->registerDeathCallback(id, deathExplosion);
	return id;
}

flecs::entity createDormantRockShip(vector3df position, vector3df rotation)
{
	flecs::entity id = createShip(16, 19, position, rotation, false);
	initializeSensors(id, 900, 2.f, true);
	//id.get_mut<ShipParticleComponent>()->engineLight[0]->setVisible(false);
	ActivateRockCallback* cb = new ActivateRockCallback();
	cb->self = id;
	SensorCallbackComponent cmp;
	cmp.callback = std::shared_ptr<SensorCallback>(cb);
	id.set<SensorCallbackComponent>(cmp);
	gameController->registerHitCallback(id, hugeRockJumpscareHitCb);
	gameController->registerDeathCallback(id, rockSplitEnemyDeath);
	return id;
}

void hugeRockJumpscare(flecs::entity rockShip)
{
	if (rockShip.has<AIComponent>()) return; //don't rebuild the thing if it exists already
	if (rockShip.has<FactionComponent>()) {
		if (rockShip.get<FactionComponent>()->type == FACTION_UNCONTROLLED) return;
	}
	initializeShipParticles(rockShip);
	//initialize other lights
	initializeFaction(rockShip, FACTION_UNCONTROLLED);
	setDamageDifficulty(rockShip);
	initializeDefaultSensors(rockShip);
	initializeAI(rockShip, AI_DEFAULT_REACTION_TIME, 1, 1.5, getCurAiAim(), getCurAiBehaviors() | AIBHVR_ATTEMPT_TO_RAM);
	auto irr = rockShip.get_mut<IrrlichtComponent>();
	irr->node->setMaterialType(shaders->getShaderMaterial(SHADE_8LIGHT_NORM));
	audioDriver->playGameSound(rockShip, "rock_ship_activate.ogg", 1, 150.f, 1200.f);
}

void hugeRockJumpscareHitCb(flecs::entity rockShip, flecs::entity attacker)
{
	hugeRockJumpscare(rockShip);
}

flecs::entity createCloudFromId(dataId id, vector3df position, vector3df scale, NetworkId net)
{
	flecs::entity ret = INVALID_ENTITY;
	switch (id) {
	case 33:
		ret = createDustCloud(position, scale);
		break;
	case 1:
		ret = createExplosiveCloud(position, scale);
		break;
	case 17:
		ret = createShieldDrainCloud(position, scale);
		break;
	case 18:
		ret = createGravityAnomaly(position, scale);
		break;
	case 19:
		ret = createSpeedBoostCloud(position, scale);
		break;
	case 20:
		ret = createSlowDownCloud(position, scale);
		break;
	default:
		ret = createDustCloud(position, scale);
		break;
	}
	if (ret != INVALID_ENTITY) {
		initializeNetworkingComponent(ret, 1, net);
	}
	return ret;
}

flecs::entity createDustCloud(vector3df position, vector3df scale)
{
	auto cloud = game_world->entity();
	if (!loadObstacle(33, cloud)) return INVALID_ENTITY;
	auto irr = cloud.get_mut<IrrlichtComponent>();
	irr->node->setID(ID_IsNotSelectable);
	irr->node->setPosition(position);
	irr->node->setScale(scale);

	IParticleSystemSceneNode* ps = (IParticleSystemSceneNode*)irr->node;
	auto em = ps->createSphereEmitter(vector3df(0, 0, 0), scale.X / 5, vector3df(0, .0005f, 0),
		15, 45, SColor(255, 50, 42, 32), SColor(255, 100, 84, 64), 2500, 4500, 360,
		dimension2df(scale.X, scale.X), dimension2df(scale.X * 15, scale.X * 15));
	ps->setEmitter(em);
	em->drop();
	BulletGhostComponent ghost;
	ghost.shape = new btSphereShape(.001f);
	ghost.ghost = new btGhostObject();
	btTransform transform;
	transform.setOrigin(irrVecToBt(position));
	ghost.ghost->setWorldTransform(transform);
	ghost.ghost->setCollisionShape(ghost.shape);
	bWorld->addCollisionObject(ghost.ghost);
	setIdOnBtObject(ghost.ghost, cloud);
	cloud.set<BulletGhostComponent>(ghost);
	return cloud;
}

flecs::entity createExplosiveCloud(vector3df position, vector3df scale)
{
	auto cloud = game_world->entity();
	if (!loadObstacle(1, cloud)) return INVALID_ENTITY;
	auto irr = cloud.get_mut<IrrlichtComponent>();
	irr->node->setID(ID_IsSelectable);
	irr->node->setPosition(position);
	irr->node->setScale(scale);

	IParticleSystemSceneNode* ps = (IParticleSystemSceneNode*)irr->node;
	auto em = ps->createSphereEmitter(vector3df(0,0,0), scale.X/5, vector3df(0, .0005f, 0),
		15, 45, SColor(0, 35, 10, 5), SColor(0, 70, 25, 10), 2500, 4500, 360,
		dimension2df(scale.X / 5, scale.X / 5), dimension2df(scale.X * 10, scale.X * 10));
	ps->setEmitter(em);
	em->drop();
	BulletGhostComponent ghost;
	ghost.shape = new btSphereShape(scale.X*4);
	ghost.ghost = new btGhostObject();
	btTransform transform;
	transform.setOrigin(irrVecToBt(position));
	ghost.ghost->setWorldTransform(transform);
	ghost.ghost->setCollisionShape(ghost.shape);
	bWorld->addCollisionObject(ghost.ghost);
	setIdOnBtObject(ghost.ghost, cloud);
	cloud.set<BulletGhostComponent>(ghost);
	gameController->registerDeathCallback(cloud, deathExplosion);
	return cloud;
}

flecs::entity createShieldDrainCloud(vector3df position, vector3df scale)
{
	auto cloud = game_world->entity();
	if (!loadObstacle(17, cloud)) return INVALID_ENTITY;
	auto irr = cloud.get_mut<IrrlichtComponent>();
	irr->node->setID(ID_IsSelectable);
	irr->node->setPosition(position);
	irr->node->setScale(scale);

	IParticleSystemSceneNode* ps = (IParticleSystemSceneNode*)irr->node;
	auto em = ps->createSphereEmitter(vector3df(0, 0, 0), scale.X / 5.5, vector3df(0, .0005f, 0),
		15, 45, SColor(0, 20, 50, 50), SColor(0, 30, 80, 80), 2500, 4500, 360,
		dimension2df(scale.X / 2, scale.X / 2), dimension2df(scale.X * 15, scale.X * 15));
	ps->setEmitter(em);
	em->drop();
	BulletGhostComponent ghost = BulletGhostComponent();
	ghost.shape = new btSphereShape(scale.X);
	ghost.ghost = new btGhostObject();
	btTransform transform;
	transform.setOrigin(irrVecToBt(position));
	ghost.ghost->setWorldTransform(transform);
	ghost.ghost->setCollisionShape(ghost.shape);
	bWorld->addCollisionObject(ghost.ghost);
	setIdOnBtObject(ghost.ghost, cloud);
	cloud.set<BulletGhostComponent>(ghost);
	gameController->registerDeathCallback(cloud, techDeathExplosion);

	initializeSensors(cloud, scale.X * 7, DEFAULT_SENSOR_UPDATE_INTERVAL);
	SensorCallbackComponent sensCb;
	ShieldDropSensorCallback* cb = new ShieldDropSensorCallback();
	cb->self = cloud;
	cb->radius = scale.X * 7;
	cb->selfPos = irrVecToBt(position);

	sensCb.callback = std::shared_ptr<SensorCallback>(cb);
	cloud.set<SensorCallbackComponent>(sensCb);
	return cloud;
}

flecs::entity createGravityAnomaly(vector3df position, vector3df scale)
{
	auto cloud = game_world->entity();
	if (!loadObstacle(18, cloud)) return INVALID_ENTITY;
	auto irr = cloud.get_mut<IrrlichtComponent>();
	irr->node->setID(ID_IsSelectable);
	irr->node->setPosition(position);
	//irr->node->setScale(scale);

	auto n = (IBillboardSceneNode*)irr->node;
	n->setSize(dimension2df(scale.X * 9, scale.X * 9));

	BulletGhostComponent ghost;
	ghost.shape = new btSphereShape(scale.X/2.f);
	ghost.ghost = new btGhostObject();
	btTransform transform;
	transform.setOrigin(irrVecToBt(position));
	ghost.ghost->setWorldTransform(transform);
	ghost.ghost->setCollisionShape(ghost.shape);
	bWorld->addCollisionObject(ghost.ghost);
	setIdOnBtObject(ghost.ghost, cloud);
	cloud.set<BulletGhostComponent>(ghost);
	gameController->registerDeathCallback(cloud, gravDeathExplosion);

	initializeSensors(cloud, scale.X * 11, DEFAULT_SENSOR_UPDATE_INTERVAL);
	SensorCallbackComponent sensCb;
	GravityYankCallback* cb = new GravityYankCallback();
	cb->self = cloud;
	cb->selfPos = irrVecToBt(position);

	sensCb.callback = std::shared_ptr<SensorCallback>(cb);
	cloud.set<SensorCallbackComponent>(sensCb);
	return cloud;
}

flecs::entity createSpeedBoostCloud(vector3df position, vector3df scale)
{
	auto cloud = game_world->entity();
	if (!loadObstacle(19, cloud)) return INVALID_ENTITY;
	auto irr = cloud.get_mut<IrrlichtComponent>();
	irr->node->setID(ID_IsSelectable);
	irr->node->setPosition(position);
	irr->node->setScale(scale);

	IParticleSystemSceneNode* ps = (IParticleSystemSceneNode*)irr->node;
	auto em = ps->createSphereEmitter(vector3df(0, 0, 0), scale.X / 5.5, vector3df(0, .0005f, 0),
		15, 45, SColor(0, 60, 50, 10), SColor(0, 100, 80, 25), 2500, 4500, 360,
		dimension2df(scale.X / 2, scale.X / 2), dimension2df(scale.X * 15, scale.X * 15));
	ps->setEmitter(em);
	em->drop();
	BulletGhostComponent ghost;
	ghost.shape = new btSphereShape(scale.X);
	ghost.ghost = new btGhostObject();
	btTransform transform;
	transform.setOrigin(irrVecToBt(position));
	ghost.ghost->setWorldTransform(transform);
	ghost.ghost->setCollisionShape(ghost.shape);
	bWorld->addCollisionObject(ghost.ghost);
	setIdOnBtObject(ghost.ghost, cloud);
	cloud.set<BulletGhostComponent>(ghost);
	gameController->registerDeathCallback(cloud, techDeathExplosion);

	initializeSensors(cloud, scale.X * 7, DEFAULT_SENSOR_UPDATE_INTERVAL);
	SensorCallbackComponent sensCb;
	ThrustChangeCallback* cb = new ThrustChangeCallback();
	cb->self = cloud;
	cb->radius = scale.X * 7;
	cb->selfPos = irrVecToBt(position);

	sensCb.callback = std::shared_ptr<SensorCallback>(cb);
	cloud.set<SensorCallbackComponent>(sensCb);
	return cloud;
}

flecs::entity createSlowDownCloud(vector3df position, vector3df scale)
{
	auto cloud = game_world->entity();
	if (!loadObstacle(20, cloud)) return INVALID_ENTITY;
	auto irr = cloud.get_mut<IrrlichtComponent>();
	irr->node->setID(ID_IsSelectable);
	irr->node->setPosition(position);
	irr->node->setScale(scale);

	IParticleSystemSceneNode* ps = (IParticleSystemSceneNode*)irr->node;
	auto em = ps->createSphereEmitter(vector3df(0, 0, 0), scale.X / 5.5, vector3df(0, .0005f, 0),
		15, 45, SColor(0, 60, 10, 60), SColor(0, 100, 25, 100), 2500, 4500, 360,
		dimension2df(scale.X / 2, scale.X / 2), dimension2df(scale.X * 15, scale.X * 15));
	ps->setEmitter(em);
	em->drop();
	BulletGhostComponent ghost;
	ghost.shape = new btSphereShape(scale.X);
	ghost.ghost = new btGhostObject();
	btTransform transform;
	transform.setOrigin(irrVecToBt(position));
	ghost.ghost->setWorldTransform(transform);
	ghost.ghost->setCollisionShape(ghost.shape);
	bWorld->addCollisionObject(ghost.ghost);
	setIdOnBtObject(ghost.ghost, cloud);
	cloud.set<BulletGhostComponent>(ghost);
	gameController->registerDeathCallback(cloud, techDeathExplosion);

	initializeSensors(cloud, scale.X * 7, DEFAULT_SENSOR_UPDATE_INTERVAL);
	SensorCallbackComponent sensCb;
	SlowdownCallback* cb = new SlowdownCallback();
	cb->self = cloud;
	//cb->radius = scale.X * 7;
	//cb->multiplier = .5f;
	//cb->selfPos = irrVecToBt(position);

	sensCb.callback = std::shared_ptr<SensorCallback>(cb);
	cloud.set<SensorCallbackComponent>(sensCb);
	return cloud;
}

void createCloudFormation(std::vector<vector3df> positions)
{
	u32 roll = random.unum(10U);
	for (auto& pos : positions) {
		f32 scale = random.frange(25.f, 40.f);
		createRolledCloud(pos, vector3df(scale,scale,scale), roll);
	}
}
flecs::entity createRandomCloud(vector3df position, vector3df scale)
{
	u32 roll = random.unum(10U);
	return createRolledCloud(position, scale, roll);
}

flecs::entity createStaticMeshCloud(vector3df position, vector3df rotation, vector3df scale, SColor innerColor, SColor outerColor)
{
	//todo: make this use the damn color
	auto cloud = game_world->entity();
	if(!loadObstacle(35, cloud)) return INVALID_ENTITY;

	auto irr = cloud.get<IrrlichtComponent>();

	irr->node->setPosition(position);
	irr->node->setScale(scale);
	irr->node->setRotation(rotation);

	IMeshSceneNode* node = (IMeshSceneNode*)irr->node;
	IMesh* mesh = node->getMesh();
	const u32 count = mesh->getMeshBufferCount();
	for (u32 i = 0; i < count; ++i) {
		auto buf = mesh->getMeshBuffer(i);
		const video::SColor clr = outerColor.getInterpolated(innerColor, ((f32)i / (f32)count));
		smgr->getMeshManipulator()->setVertexColors(buf, clr);
	}

	irr->node->setID(ID_IsNotSelectable);
	irr->node->setMaterialFlag(EMF_LIGHTING, false);
	auto anim = smgr->createRotationAnimator(vector3df(.01, .02, .03));
	irr->node->addAnimator(anim);
	anim->drop();
	anim = new CSceneNodeScalePulseAnimator(device->getTimer()->getTime(), scale, scale*2.f, 25000U, true);
	irr->node->addAnimator(anim);
	anim->drop();

	return cloud;
}

flecs::entity createRolledCloud(vector3df position, vector3df scale, u32 originalRoll)
{
	u32 roll = std::clamp(originalRoll, 0U, 10U);
	if (roll > 8) return createSpeedBoostCloud(position, scale);
	else if (roll > 6) return createSlowDownCloud(position, scale);
	else if (roll > 4) return createShieldDrainCloud(position, scale);
	else return createExplosiveCloud(position, scale);
}

static void _getRBCData(flecs::entity e, MapGenObstacle& gen)
{
	if (!e.has<BulletRigidBodyComponent>() || !e.has<IrrlichtComponent>())
		return;
	auto body = e.get<BulletRigidBodyComponent>()->rigidBody;
	auto node = e.get<IrrlichtComponent>()->node;

	gen.mass = body->getMass();
	gen.scale = node->getScale();

	btTransform motionStateTransform;
	body->getMotionState()->getWorldTransform(motionStateTransform);
	gen.position = btVecToIrr(motionStateTransform.getOrigin());
	btVector3 eulerOrientation;
	QuaternionToEuler(motionStateTransform.getRotation(), eulerOrientation);
	gen.rotation = btVecToIrr(eulerOrientation);
	gen.startLinVel = body->getLinearVelocity().length();
	gen.startRotVel = body->getAngularVelocity().length();
}

MapGenObstacle createMapGenObstacleFromEntity(flecs::entity ent)
{
	if (!ent.has<ObstacleComponent>() || !ent.has<NetworkingComponent>())
		return MapGenObstacle();
	MapGenObstacle ret;
	auto obst = ent.get<ObstacleComponent>();
	auto net = ent.get<NetworkingComponent>();
	ret.id = obst->obstacleDataId;
	ret.type = obst->type;
	ret.networkId = net->networkedId;
	if (ent.has<FactionComponent>())
		ret.faction = ent.get<FactionComponent>()->type;

	switch (obst->type) {
	case ASTEROID:
	case DEBRIS:
	case JET_DEBRIS:
	case MONEY_ASTEROID:
	case CASH_NUGGET:
	case STASIS_POD:
	case DERELICT_STATION_MODULE:
	case MINE:
	case MISSILE:
	case HAYWIRE_TURRET:
	case RADIOACTIVE_ASTEROID: {
		_getRBCData(ent, ret);
		break;
	}
	case ICE_ASTEROID:
		_getRBCData(ent, ret);
		ret.extraFloats[0] = gameController->hasDeathCallback(ent);
		break;
	case FLAT_BILLBOARD:
	case FLAT_BILLBOARD_ANIMATED:
	case GAS_CLOUD: {
		auto ghost = ent.get<BulletGhostComponent>()->ghost;
		auto node = ent.get<IrrlichtComponent>()->node;
		ret.position = btVecToIrr(ghost->getWorldTransform().getOrigin());
		ret.scale = node->getScale();
		break;
	}
	case DEAD_SHIP: {
		_getRBCData(ent, ret);
		break;
	}
	case DEAD_WEAPON: {
		_getRBCData(ent, ret);
		break;
	}
	case STATION_MODULE: {
		_getRBCData(ent, ret);
		if (ent.has<StationModuleComponent>()) {
			auto mod = ent.get<StationModuleComponent>();

			if(!ent.has<StationModuleOwnerComponent>())
				ret.extraFloats[0] = mod->ownedBy.get<NetworkingComponent>()->networkedId;

			ret.extraFloats[1] = mod->connectedOn;

			if (mod->connectedEntities[0] != INVALID_ENTITY)
				ret.extraFloats[2] = mod->connectedEntities[0].get<NetworkingComponent>()->networkedId;
			if (mod->connectedEntities[1] != INVALID_ENTITY)
				ret.extraFloats[3] = mod->connectedEntities[0].get<NetworkingComponent>()->networkedId;
			if (mod->connectedEntities[2] != INVALID_ENTITY)
				ret.extraFloats[4] = mod->connectedEntities[0].get<NetworkingComponent>()->networkedId;
			if (mod->connectedEntities[3] != INVALID_ENTITY)
				ret.extraFloats[5] = mod->connectedEntities[0].get<NetworkingComponent>()->networkedId;
		}
	}
	default:
		break;
	}
	return ret;
}

flecs::entity createObstacleFromMapGen(MapGenObstacle obst)
{
	switch (obst.type) {
		case ASTEROID:
		case DEBRIS: {
			return createDynamicObstacle(obst.id, obst.position, obst.rotation, obst.scale, obst.mass, obst.startLinVel, obst.startRotVel, false, obst.networkId);
		}
		case JET_DEBRIS: {
			return createEngineDebris(obst.position, obst.rotation, obst.scale, obst.mass, obst.networkId);
		}
		case ICE_ASTEROID:
			return createIceAsteroid(obst.position, obst.rotation, obst.scale, obst.startLinVel, obst.startRotVel, obst.extraFloats[0], obst.networkId);
		case RADIOACTIVE_ASTEROID:
			return createRadioactiveAsteroid(obst.position, obst.rotation, obst.scale, obst.startLinVel, obst.startRotVel, obst.networkId);
		case MONEY_ASTEROID:
			return createMoneyAsteroid(obst.position, obst.rotation, obst.scale, obst.startLinVel, obst.startRotVel, obst.networkId);
		case CASH_NUGGET:
			return createCashNugget(obst.position, obst.rotation, obst.scale, obst.startLinVel, obst.startRotVel, obst.networkId);
		case GAS_CLOUD:
			return createCloudFromId(obst.id, obst.position, obst.rotation, obst.networkId);
		case STASIS_POD:
			return createStasisPod(obst.position, obst.rotation, obst.networkId);
		case EXPLOSIVE_ASTEROID:
			return createExplosiveAsteroid(obst.position, obst.rotation, obst.scale, obst.mass, obst.networkId);
		case DERELICT_STATION_MODULE:
			return createLooseHumanModule(obst.position, obst.rotation, obst.scale, obst.id, obst.networkId);
		default:
			break;
	}

	baedsLogger::errLog("Invalid obstacle from network - data ID: " + std::to_string(obst.id) + ", network ID: " + std::to_string(obst.networkId) + ", type: " + std::to_string(obst.type) + "\n");
	return INVALID_ENTITY;
}

void deathExplosion(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	vector3df pos = irr->node->getAbsolutePosition();
	vector3df scale = irr->node->getScale();
	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale;
	explode(pos, 3.f, avgscale, rad, 90.f, 1200.f);
	audioDriver->playGameSound(pos, "death_explosion_fighter.ogg");
}

void rockSplitEnemyDeath(flecs::entity id)
{
	btVector3 velocity = id.get_mut<BulletRigidBodyComponent>()->rigidBody->getLinearVelocity();

	fighterDeathExplosionCallback(id); //blow up as normal
	auto pos = id.get<IrrlichtComponent>()->node->getAbsolutePosition();
	const u32 splits = 4;
	vector3df positions[splits] = { pos + vector3df(0, 60, 0) , pos + vector3df(-70, 70, 0), pos + vector3df(20, 0, 20), pos + vector3df(75,-20,10)};
	for (u32 i = 0; i < splits; ++i) {
		auto ship = createHostileShip(17, 19, positions[i], randomRotationVector());
		initializeAI(ship, AI_DEFAULT_REACTION_TIME, 1, 1.5, getCurAiAim(), getCurAiBehaviors() | AIBHVR_ATTEMPT_TO_RAM);
		initializeFaction(ship, FACTION_UNCONTROLLED);
		ship.get_mut<BulletRigidBodyComponent>()->rigidBody->setLinearVelocity(velocity + irrVecToBt(randomDirectionalVector()*150.f));
	}
}

void iceSplitExplosion(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	auto pos = irr->node->getAbsolutePosition();
	auto& box = irr->node->getBoundingBox();
	f32 boxdist = (box.MinEdge - box.MaxEdge).getLength() / 2.f;

	u32 amt = random.urange(6, 9);
	auto scale = irr->node->getScale() / amt;

	for (u32 i = 0; i < amt; ++i) {
		createIceAsteroid(getPointInSphere(pos, boxdist), randomRotationVector(), scale, 250, 0.7f, false);
	}
	
	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale;

	explode(pos, 3.f, avgscale, rad, 10.f, 600.f, EXTYPE_TECH);
	audioDriver->playGameSound(pos, "asteroid_death.ogg");
}

void moneySplitExplosion(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	auto pos = irr->node->getAbsolutePosition();
	auto& box = irr->node->getBoundingBox();
	f32 boxdist = (box.MinEdge - box.MaxEdge).getLength() / 2.f;

	u32 amt = random.urange(10, 12);
	auto scale = irr->node->getScale() / (f32)(amt/4);

	for (u32 i = 0; i < amt; ++i) {
		createCashNugget(getPointInSphere(pos, boxdist), randomRotationVector(), scale, 15, 1.4f);
	}

	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale;

	explode(pos, 3.f, avgscale, rad, 10.f, 150.f, EXTYPE_TECH);
	audioDriver->playGameSound(pos, "asteroid_death.ogg");
}

void techDeathExplosion(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	vector3df pos = irr->node->getAbsolutePosition();
	vector3df scale = irr->node->getScale();
	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale;
	explode(pos, 3.f, avgscale, rad, 50.f, 300.f, EXTYPE_TECH);
	audioDriver->playGameSound(pos, "death_explosion_fighter.ogg");
}

void gravDeathExplosion(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	vector3df pos = irr->node->getAbsolutePosition();
	vector3df scale = irr->node->getScale();
	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale * .35f;
	implode(pos, 3.f, avgscale, rad, 20.f, 700.f, EXTYPE_TECH);
	audioDriver->playGameSound(pos, "death_explosion_fighter.ogg");
}

void fuelDeathExplosion(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	vector3df pos = irr->node->getAbsolutePosition();
	vector3df scale = irr->node->getScale();
	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale *1.5f;
	explode(pos, 3.f, avgscale, rad, 120.f, 800.f);
	audioDriver->playGameSound(pos, "hazard_mine_explode.ogg", 1.f, 30.f, 1000.f);
}

void mineExplosion(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	vector3df pos = irr->node->getAbsolutePosition();
	vector3df scale = irr->node->getScale();
	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = 350.f;
	explode(pos, 3.f, avgscale, rad, 150.f, 850.f);
	audioDriver->playGameSound(pos, "hazard_mine_explode.ogg", 1.f, 30.f, 1000.f);
}

void supplyBoxDeath(flecs::entity id)
{
	if (random.coinflip()) {
		campaign->addSupplies(random.frange(5.f, 15.f));
		audioDriver->playGameSound(id, "supplies.ogg");
		return;
	}

	u32 amount = random.urange(2U, 10U);
	campaign->addAmmo(amount);
	audioDriver->playGameSound(id, "reload.ogg");
}

void weaponPickupDeath(flecs::entity id)
{
	auto obst = id.get<ObstacleComponent>();
	auto wep = id.get<WeaponInfoComponent>();
	campaign->createNewWeaponInstance(*wep);
	audioDriver->playGameSound(id, "supplies.ogg");
}

void onHitDebrisEngineTakeoff(flecs::entity target, flecs::entity attacker)
{
	ThrustComponent* thrust = target.get_mut<ThrustComponent>();
	if (thrust->forward != 0.f) return;
	thrust->forward = 900.f;
	thrust->moves[THRUST_FORWARD] = true;
	thrust->nonstopThrust = true;
}

void stasisPodCollect(flecs::entity id)
{
	audioDriver->playGameSound(id, "order_confirm.ogg", 1.0f, 100.f, 500.f);
}