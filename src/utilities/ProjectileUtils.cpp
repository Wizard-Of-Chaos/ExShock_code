#include "ProjectileUtils.h"
#include "GameController.h"
#include "GameStateController.h"
#include "IrrlichtUtils.h"
#include "ObstacleAI.h"
#include "NodeAnimators.h"
#include "Shaders.h"
#include "SensorComponent.h"
#include "HardpointComponent.h"
#include "Config.h"
#include "TurretHardpointComponent.h"
#include "GameAssets.h"
#include "AudioDriver.h"
#include "BulletRigidBodyComponent.h"
#include "AIComponent.h"
#include "ShipUtils.h"
#include "IrrlichtComponent.h"
#include "GameFunctions.h"
#include "CrashLogger.h"

static bool _parentCheck(flecs::entity weaponId) 
{
	if (!weaponId.has<WeaponInfoComponent>() || weaponId.has_relation(flecs::ChildOf)) {
		std::string err = "Can't fire weapon " + entDebugStr(weaponId) + "-";
		if (!weaponId.has<WeaponInfoComponent>()) err += "Weapon info ";
		if (weaponId.has_relation(flecs::ChildOf)) err += "Parent info ";
		err += "is nonexistent";
		baedsLogger::errLog(err + "\n");

		return false;
	}
	return true;
}

ProjectileInfoComponent _getProjectileInfo(const WeaponInfoComponent* wepInfo, const WeaponFiringComponent* wepFire)
{
	ProjectileInfoComponent projectileInfo;
	projectileInfo.type = wepInfo->type;
	projectileInfo.dmgtype = wepInfo->dmgtype;
	projectileInfo.speed = wepInfo->projectileSpeed;
	projectileInfo.startPos = wepFire->spawnPosition;
	projectileInfo.lifetime = wepInfo->lifetime;
	projectileInfo.currentLifetime = 0.f;
	projectileInfo.damage = wepInfo->damage;
	projectileInfo.impactSound = wepInfo->impactSound;
	projectileInfo.particle = wepInfo->particle;
	projectileInfo.length = wepInfo->length;
	projectileInfo.fireDir = wepFire->firingDirection;
	projectileInfo.hitEffects = wepInfo->hitEffects;
	return projectileInfo;
}

static BulletRigidBodyComponent _getRBC(flecs::entity id, btVector3& initForce, btVector3& initVelocity, vector3df& spawn, btQuaternion& initRot, f32 radius)
{
	f32 mass = .1f;
	BulletRigidBodyComponent rbc;

	btTransform transform;
	transform.setIdentity();

	transform.setRotation(initRot);
	transform.setOrigin(irrVecToBt(spawn));

	auto motionState = new btDefaultMotionState(transform); //doesn't get deleted anywhere - check for a leak later

	rbc.shape = new btSphereShape(radius);
	btVector3 localInertia;

	rbc.shape->calculateLocalInertia(mass, localInertia);
	rbc.rigidBody = new btRigidBody(mass, motionState, rbc.shape, localInertia);
	setIdOnBtObject(rbc.rigidBody, id);

	rbc.rigidBody->setLinearVelocity(initVelocity);
	rbc.rigidBody->applyCentralImpulse(initForce);
	bWorld->addRigidBody(rbc.rigidBody);

	return rbc;
}

static flecs::entity _getProjectileEntity(const WeaponInfoComponent* info, const WeaponFiringComponent* fire, flecs::entity weapon, 
	flecs::entity owner, bool turret=false, flecs::entity_t shot=INVALID_ENTITY_ID)
{
	flecs::entity proj;

	flecs::entity player = gameController->getPlayer();

	if(shot != INVALID_ENTITY_ID) 
		proj = game_world->entity(shot).child_of(owner);
	else 
		proj = game_world->entity().child_of(owner);

	if (!proj.is_alive() || proj == INVALID_ENTITY) {
		baedsLogger::errLog("Projectile invalid; can't get\n");
		return INVALID_ENTITY;
	}
	if (owner.has<FactionComponent>()) proj.set<FactionComponent>(*owner.get<FactionComponent>());
	proj.set<ProjectileInfoComponent>(_getProjectileInfo(info, fire));
	proj.add(gameController->firedBy(), weapon);
	proj.add(gameController->doNotCollideWith(), owner);

	if (turret) {
		flecs::entity owningShip = owner.target(flecs::ChildOf); //if its a turret
		if (owningShip.is_alive()) {
			proj.add(gameController->doNotCollideWith(), owningShip);
			auto turrhdp = owningShip.get<TurretHardpointComponent>();
			if (turrhdp) {
				for (u32 i = 0; i < turrhdp->turretCount; ++i) {
					proj.add(gameController->doNotCollideWith(), turrhdp->turrets[i]); //don't shoot other turrets!
				}
			}
		}
	}
	return proj;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static void _addLightShaftFx(flecs::entity proj, const WeaponInfoComponent* wepInfo, const WeaponFiringComponent* wepFire, vector3df rot)
{
	IrrlichtComponent irr;
	proj.set_doc_name("Shot");

	u32 lvl = (u32)cfg->vid.particleLevel + 1;
	u32 subdv = lvl * 10;
	
	auto node = smgr->addVolumeLightSceneNode(0, ID_IsNotSelectable, subdv, subdv, SColor(150, 180, 180, 180), SColor(0, 0, 0, 0),
		wepFire->spawnPosition, rot);
	//node->setScale(vector3df(wepInfo->scale, 1.5f, wepInfo->scale));
	node->setScale(vector3df(wepInfo->scale, wepInfo->length, wepInfo->scale));
	node->getMaterial(0).setTexture(0, wepInfo->particle);
	node->setMaterialFlag(EMF_LIGHTING, false);
	node->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);
	
	irr.node = node;
	irr.node->setName(idToStr(proj).c_str());

	//note: incredibly stupid hack because there's sometimes a weird error where projectiles aren't facing the correct direction on the frame they're spawned
	//projectiles spawn as invisible and then get set visible on the first frame after they've had their collision body adjusted
	irr.node->setVisible(false);

	auto cleanup = smgr->createDeleteAnimator((u32)(wepInfo->lifetime * 1000.f) + 25);
	irr.node->addAnimator(cleanup);
	cleanup->drop();

	//todo: actual particle shot trails
	/*
	auto edgeColor = wepInfo->projectileLightColor;
	edgeColor.setAlpha(32);
	auto tailColor = wepInfo->projectileLightColor;
	tailColor.setAlpha(0);
	auto trail = addMotionTrailLengthBased(node, 8, wepInfo->length, true);
	trail->getMaterial(0).Lighting = false;
	trail->getMaterial(0).MaterialType = EMT_TRANSPARENT_ALPHA_CHANNEL;
	trail->setVertexColors(
		wepInfo->projectileLightColor, //tip center
		edgeColor, //tip edge
		edgeColor, //end center
		tailColor); //end edge
	trail->setWidth(wepInfo->scale);
	trail->setShrinkDirection(false, true);

	trail = addMotionTrailTimeBased(node, 8, 500, true);
	trail->getMaterial(0).Lighting = false;
	trail->getMaterial(0).MaterialType = EMT_TRANSPARENT_ALPHA_CHANNEL;
	trail->setVertexColors(
		wepInfo->projectileLightColor, //tip center
		edgeColor, //tip edge
		edgeColor, //end center
		tailColor); //end edge
	trail->setWidth(wepInfo->scale / 1.75f);
	trail->setShrinkDirection(false, true);
	//*/
	auto light = smgr->addLightSceneNode(node, vector3df(0, 0, 0), SColorf(wepInfo->projectileLightColor), 150.f);
	light->setID(ID_IsNotSelectable);

	gameController->registerDeathCallback(proj, projectileImpactCallback);

	proj.set<IrrlichtComponent>(irr);
}

static void _addRockFx(flecs::entity proj, const WeaponInfoComponent* wepInfo, const WeaponFiringComponent* wepFire, vector3df rot)
{
	IrrlichtComponent irr;
	proj.set_doc_name("Rockball");

	u32 lvl = (u32)cfg->vid.particleLevel + 1;

	bool firstLoad = true;
	auto mesh = assets->getMesh("assets/models/asteroids/roid_2/roid_2.obj", firstLoad);
	auto node = smgr->addMeshSceneNode(mesh, 0, ID_IsNotSelectable, wepFire->spawnPosition, rot);
	node->setScale(vector3df(wepInfo->scale, wepInfo->scale, wepInfo->scale));
	node->getMaterial(0).setTexture(0, assets->getTexture("assets/effects/rockball.png"));
	node->setMaterialFlag(EMF_LIGHTING, false);
	node->setMaterialType(EMT_SOLID);
	node->setVisible(true);

	auto ps = smgr->addParticleSystemSceneNode(true, node);
	auto em = ps->createPointEmitter(-wepFire->firingDirection * .0005f, lvl * 4, lvl * 15);
	em->setMinStartSize(dimension2df(4.5f, 4.5f));
	em->setMaxStartSize(dimension2df(7.5f, 7.5f));
	ps->setEmitter(em);
	em->drop();
	ps->setMaterialTexture(0, wepInfo->particle);
	ps->setMaterialFlag(EMF_LIGHTING, false);
	ps->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);

	irr.node = node;
	irr.node->setID(ID_IsNotSelectable);
	irr.node->setName(idToStr(proj).c_str());

	auto cleanup = smgr->createDeleteAnimator((u32)(wepInfo->lifetime * 1000.f) + 50);
	irr.node->addAnimator(cleanup);
	cleanup->drop();

	auto light = smgr->addLightSceneNode(node, vector3df(0, 0, 0), SColorf(wepInfo->projectileLightColor), 30.f);
	light->setID(ID_IsNotSelectable);

	proj.set<IrrlichtComponent>(irr);

	gameController->registerDeathCallback(proj, projectileImpactCallback);

}

static void _addMissileFx(flecs::entity projId, const WeaponInfoComponent* wepInfo, const WeaponFiringComponent* wepFire, vector3df rotation)
{
	IrrlichtComponent irr;
	projId.set_doc_name("Missile");

	u32 lvl = (u32)cfg->vid.particleLevel + 1;

	flecs::entity launcher = projId.target(gameController->firedBy());
	flecs::entity ship = launcher.target(flecs::ChildOf);
	auto missInfo = launcher.get<MissileInfoComponent>();
	auto sensors = ship.get<SensorComponent>();
	auto mesh = missInfo->missileMesh;
	auto tex = missInfo->missileTexture;

	auto node = smgr->addMeshSceneNode(mesh, 0, ID_IsNotSelectable, wepFire->spawnPosition);
	node->setMaterialTexture(0, tex);
	node->setMaterialType(shaders->getShaderMaterial(SHADE_8LIGHT_NORM));
	node->setVisible(true);
	node->setScale(vector3df(wepInfo->scale));
	node->setName(idToStr(projId).c_str());
	irr.node = node;

	auto edgeColor = wepInfo->projectileLightColor;
	edgeColor.setAlpha(32);
	auto tailColor = wepInfo->projectileLightColor;
	tailColor.setAlpha(0);
	auto trail = addMotionTrailTimeBased(node, 16, 750, true);
	trail->getMaterial(0).Lighting = false;
	trail->getMaterial(0).MaterialType = EMT_TRANSPARENT_ALPHA_CHANNEL;
	trail->setVertexColors(
		wepInfo->projectileLightColor, //tip center
		edgeColor, //tip edge
		edgeColor, //end center
		tailColor); //end edge
	trail->setWidth(wepInfo->scale);
	trail->setShrinkDirection(false, true);

	AIComponent ai;
	ai.state = AI_STATE_PURSUIT;
	auto missAI = new MissileAI;
	missAI->self = projId;
	missAI->target = sensors->targetContact;
	missAI->lifetime = wepInfo->lifetime;
	missAI->kablooeyDistance = 8.f;
	missAI->highAngle = 3.5f;
	missAI->lowAngle = .001f;
	ai.aiControls = std::shared_ptr<AIType>(missAI);
	ai.setBehavior(AIBHVR_ATTEMPT_TO_RAM);

	projId.set<FactionComponent>(*ship.get<FactionComponent>());
	initializeSensors(projId, 2000.f, 2000.f);

	projId.set<ThrustComponent>(missInfo->missThrust);
	projId.set<HardpointComponent>(HardpointComponent());
	projId.set<AIComponent>(ai);

	initializeHealth(projId, 20.f);

	auto cleanup = smgr->createDeleteAnimator((u32)(wepInfo->lifetime * 1000.f) + 50);
	irr.node->addAnimator(cleanup);
	cleanup->drop();

	projId.set<IrrlichtComponent>(irr);

	gameController->registerDeathCallback(projId, missileDeathCallback);

	audioDriver->playGameSound(projId, "weapon_hum_missile.ogg", 1.f, 8.f, 550.f, true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static bool _fireProj(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, 
	std::function<void(flecs::entity, const WeaponInfoComponent*, const WeaponFiringComponent*, vector3df)> fx_func = _addLightShaftFx, Network_ShotFired* chaser=nullptr)
{
	if (!_parentCheck(wepId)) return false;
	flecs::entity owner = wepId.target(flecs::ChildOf);
	auto shipRBC = owner.get<BulletRigidBodyComponent>();

	//TODO: turret check but for reals
	bool isTurret = true;

	btVector3 initVelocity = shipRBC->rigidBody->getLinearVelocity();
	btVector3 initialForce;
	btQuaternion initRot = shipRBC->rigidBody->getOrientation();

	vector3df rotvec;
	initRot.getEulerZYX(rotvec.Z, rotvec.Y, rotvec.X);
	vector3df dir;
	vector3df spawnPos;
	flecs::entity ids[MAX_PROJECTILES_PER_SHOT];
	vector3df directions[MAX_PROJECTILES_PER_SHOT];

	for (u32 i = 0; i < wep->projectilesPerShot; ++i) {
		flecs::entity_t shotId = INVALID_ENTITY_ID;
		if (chaser) {
			shotId = chaser->projIds[i];
			if (shotId == INVALID_ENTITY_ID) {
				baedsLogger::log("Projectile shot requested for weapon, but projectile ID is invalid? Weapon: " + entDebugStr(wepId) + "\n");
				continue;
			}
		}
		auto proj = _getProjectileEntity(wep, fire, wepId, owner, isTurret, shotId);
		if (proj == INVALID_ENTITY) 
			continue;
		
		dir = fire->firingDirection;
		if (wep->accuracy < 20.f) 
			dir = makeVectorInaccurate(dir, wep->accuracy);

		if (chaser) 
			dir = chaser->directions[i];

		initialForce = irrVecToBt(dir) * wep->projectileSpeed;
		if(wep->type != WEP_HEAVY_MISSILE) initRot = shortestArcQuat(irrVecToBt(vector3df(0, -1, 0)), irrVecToBt(dir));

		spawnPos = fire->spawnPosition;
		if (chaser) spawnPos = chaser->spawn;

		BulletRigidBodyComponent rbc = _getRBC(proj, initialForce, initVelocity, spawnPos, initRot, wep->radius);
		proj.set<BulletRigidBodyComponent>(rbc);
		fx_func(proj, wep, fire, rotvec);

		ids[i] = proj;
		directions[i] = dir;
	}
	if (!chaser && gameController->isNetworked()) {//this was a shot generated by the client; we need to send it
		Network_ShotFired shot;
		shot.firingWeapon = wepId.id();
		shot.spawn = spawnPos;
		for (u32 i = 0; i < wep->projectilesPerShot; ++i) {
			shot.directions[i] = directions[i];
			shot.projIds[i] = ids[i];
		}
		gameController->sendShotToNetwork(shot);
	}
	return true;
}

bool wepFire_kineticKick(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	auto shipRbc = wepId.target(flecs::ChildOf).get_mut<BulletRigidBodyComponent>();
	shipRbc->rigidBody->applyCentralImpulse(getRigidBodyBackward(shipRbc->rigidBody) * (wep->projectileSpeed / 2.2f));
	
	return _fireProj(wep, fire, power, wepId, dt, _addLightShaftFx, chaser);
}

bool wepFire_kinetic(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	return _fireProj(wep, fire, power, wepId, dt, _addLightShaftFx, chaser);
}

bool wepFire_plasma(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	return _fireProj(wep, fire, power, wepId, dt, _addLightShaftFx, chaser);
}

bool wepFire_rock(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	return _fireProj(wep, fire, power, wepId, dt, _addRockFx, chaser);
}

bool wepFire_missile(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	return _fireProj(wep, fire, power, wepId, dt, _addMissileFx, chaser);
}

bool wepFire_railgun(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	if (!_parentCheck(wepId)) return false;

	if (wep->hasFired) {
		auto shipRbc = wepId.target(flecs::ChildOf).get_mut<BulletRigidBodyComponent>();
		shipRbc->rigidBody->applyCentralImpulse(getRigidBodyBackward(shipRbc->rigidBody) * 300.f);
		wepFire_kinetic(wep, fire, power, wepId, dt, chaser);
		audioDriver->playGameSound(wepId, "weapon_fire_engsniper.ogg");
		wep->hasFired = false;
		return true;
	}

	wep->hasFired = true;
	return true;
}

bool wepFire_laser(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	if (!_parentCheck(wepId)) return false;
	flecs::entity ship = wepId.target(flecs::ChildOf);

	btSphereShape shape(wep->radius);
	btVector3 from = irrVecToBt(fire->spawnPosition);
	vector3df dir = fire->firingDirection;
	btVector3 to;
	btTransform tFrom, tTo;
	auto rbc = ship.get<BulletRigidBodyComponent>();
	for (u32 i = 0; i < wep->projectilesPerShot; ++i) {

		if (wep->accuracy < 20.f) {
			dir = makeVectorInaccurate(dir, wep->accuracy);
		}
		to = from + (irrVecToBt(dir) * wep->projectileSpeed);

		btClosestNotMeRayResultCallback cb(rbc->rigidBody, from, to);

		tFrom = btTransform(rbc->rigidBody->getOrientation(), from);
		tTo = btTransform(rbc->rigidBody->getOrientation(), to);
		bWorld->rayTest(from, to, cb);
		if (cb.hasHit()) {
			auto hit = getIdFromBt(cb.m_collisionObject);
			audioDriver->playGameSound(hit, wep->impactSound);
			for (auto& eff : wep->hitEffects) {
				eff(wepId, hit, cb.m_hitPointWorld);
			}
			to = cb.m_hitPointWorld;
		}

		btVector3 dist = from - to; //to - from;
		btQuaternion rot = shortestArcQuat(irrVecToBt(vector3df(0, 1, 0)), dist.normalized());
		btVector3 btRot;
		QuaternionToEuler(rot, btRot);
		f32 len = dist.length();

		u32 lvl = 16;
		lvl *= ((u32)cfg->vid.particleLevel + 1);
		auto node = smgr->addVolumeLightSceneNode(0, ID_IsNotSelectable, lvl, lvl, SColor(255, 255, 255, 255), SColor(100, 100, 100, 100),
			btVecToIrr(to), btVecToIrr(btRot), vector3df(wep->scale, len * .84f, wep->scale));
		node->setMaterialTexture(0, wep->particle);
		node->setMaterialFlag(EMF_LIGHTING, false);
		node->setMaterialFlag(EMF_FOG_ENABLE, false);
		u32 duration = 500U + ((u32)(len / 200.f) * 80U);
		auto anim = new CScaleToNothingAndDeleteAnimator(device->getTimer()->getTime(), node->getScale(), 500U, smgr);
		node->addAnimator(anim);
		anim->drop();
	}
	return true;
}

bool wepFire_thickLaser(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	if (!_parentCheck(wepId)) return false;
	flecs::entity ship = wepId.target(flecs::ChildOf);

	btSphereShape shape(wep->radius);
	btVector3 from = irrVecToBt(fire->spawnPosition);
	vector3df dir = fire->firingDirection;
	btVector3 to;
	btTransform tFrom, tTo;
	auto rbc = ship.get<BulletRigidBodyComponent>();
	for (u32 i = 0; i < wep->projectilesPerShot; ++i) {

		if (wep->accuracy < 20.f) {
			dir = makeVectorInaccurate(dir, wep->accuracy);
		}
		to = from + (irrVecToBt(dir) * wep->projectileSpeed);

		btClosestNotMeConvexResultCallback cb(rbc->rigidBody, from, to, bWorld->getPairCache(), bWorld->getDispatcher());
		btSphereShape shape = btSphereShape(wep->radius);

		tFrom = btTransform(rbc->rigidBody->getOrientation(), from);
		tTo = btTransform(rbc->rigidBody->getOrientation(), to);
		tTo.setOrigin(to);
		bWorld->convexSweepTest(&shape, tFrom, tTo, cb);
		if (cb.hasHit()) {
			auto hit = getIdFromBt(cb.m_hitCollisionObject);
			audioDriver->playGameSound(hit, wep->impactSound);

			for (auto& eff : wep->hitEffects) {
				eff(wepId, hit, cb.m_hitPointWorld);
			}
			to = cb.m_hitPointWorld;
		}

		btVector3 dist = from - to; //to - from;
		btQuaternion rot = shortestArcQuat(irrVecToBt(vector3df(0, 1, 0)), dist.normalized());
		btVector3 btRot;
		QuaternionToEuler(rot, btRot);
		f32 len = dist.length();

		u32 lvl = 16;
		lvl *= ((u32)cfg->vid.particleLevel + 1);
		auto node = smgr->addVolumeLightSceneNode(0, ID_IsNotSelectable, lvl, lvl, SColor(255, 255, 255, 255), SColor(100, 100, 100, 100),
			btVecToIrr(to), btVecToIrr(btRot), vector3df(wep->scale, len * .84f, wep->scale));
		node->setMaterialTexture(0, wep->particle);
		node->setMaterialFlag(EMF_LIGHTING, false);
		node->setMaterialFlag(EMF_FOG_ENABLE, false);
		u32 duration = 500U + ((u32)(len / 200.f) * 80U);
		auto anim = new CScaleToNothingAndDeleteAnimator(device->getTimer()->getTime(), node->getScale(), 500U, smgr);
		node->addAnimator(anim);
		anim->drop();
	}
	return true;
}

bool wepFire_goron(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser)
{
	if (!_parentCheck(wepId)) return false;
	flecs::entity ship = wepId.target(flecs::ChildOf);
	if (ship.has<HealthComponent>()) {
		auto hp = ship.get_mut<HealthComponent>();
		for (u32 i = 0; i < MAX_DAMAGE_TYPES; ++i) {
			hp->healthResistances[i] += .8f;
			hp->shieldResistances[i] += .8f;
		}
	}
	if (ship.has<IrrlichtComponent>()) {
		auto irr = ship.get<IrrlichtComponent>();
		ISceneNode* node = smgr->addBillboardSceneNode(irr->node, dimension2df(wep->radius, wep->radius), vector3df(0), ID_IsNotSelectable);
		node->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
		node->setMaterialFlag(EMF_LIGHTING, false);
		ISceneNodeAnimator* anim = getTextureAnim("assets/effects/radiation/", 8, true, false);
		node->addAnimator(anim);
		anim->drop();
		anim = smgr->createDeleteAnimator(8000U);
		node->addAnimator(anim);
		anim->drop();
	}
	fire->hasFired = true;
	return true;
}

void projectileImpactCallback(flecs::entity ent)
{
	auto rbc = ent.get<BulletRigidBodyComponent>();
	if (!rbc) return;
	vector3df pos = btVecToIrr(rbc->rigidBody->getCenterOfMassPosition());
	ITexture* tex = ent.get<ProjectileInfoComponent>()->particle;
	auto node = smgr->addParticleSystemSceneNode(true, 0, ID_IsNotSelectable, btVecToIrr(rbc->rigidBody->getCenterOfMassPosition()));
	u32 lvl = (u32)cfg->vid.particleLevel;

	++lvl;//if 0

	auto em = node->createSphereEmitter(vector3df(0, 0, 0), .2f, vector3df(0.01f, 0.f, 0.f), lvl*8, lvl*15, SColor(0, 255, 255, 255), SColor(0, 255, 255, 255),
		50, 100, 360, dimension2df(1.f, 1.f), dimension2df(1.5f, 1.5f));
	node->setEmitter(em);
	em->drop();
	IParticleAffector* paf = node->createFadeOutParticleAffector(SColor(0, 0, 0, 0), 100);
	node->addAffector(paf);
	paf->drop();
	node->setMaterialFlag(EMF_LIGHTING, false);
	node->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
	node->setMaterialTexture(0, tex);
	node->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);

	auto anim = smgr->createDeleteAnimator(400);
	node->addAnimator(anim);
	anim->drop();
}

void missileDeathCallback(flecs::entity miss)
{
	auto proj = miss.get<ProjectileInfoComponent>();
	auto irr = miss.get<IrrlichtComponent>();
	vector3df pos = irr->node->getPosition();
	explode(pos, 1.5f, 3.f, 170.f, proj->damage, 450.f);
	audioDriver->playGameSound(pos, "weapon_hit_missile.ogg", 1.f, 30.f, 1000.f);
}