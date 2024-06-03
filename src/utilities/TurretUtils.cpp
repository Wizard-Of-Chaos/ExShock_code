#include "TurretUtils.h"
#include "AttributeReaders.h"
#include "GameFunctions.h"
#include "btUtils.h"
#include "GameController.h"
#include "Archetypes.h"
#include "BulletRigidBodyComponent.h"
#include "Config.h"
#include "ShipUtils.h"
#include "CrashLogger.h"

flecs::entity createTurret(const s32 id, const s32 wepId, const vector3df pos, const vector3df rot, FACTION_TYPE faction,
	const vector3df scale, s32 slot, flecs::entity owner)
{
	flecs::entity turret = game_world->entity();
	if (turret == INVALID_ENTITY) {
		//this shouldnt happen but I should grab it anyway
		baedsLogger::errLog("Turret creation failed!\n");
		return INVALID_ENTITY;
	}
	//irrlicht and component data
	loadTurret(id, turret);
	turret.add<TurretTag>();
	//put it in place
	auto irr = turret.get_mut<IrrlichtComponent>();
	irr->node->setPosition(pos);
	irr->node->setRotation(rot);
	irr->node->setScale(scale);
	auto sphere = new btSphereShape(irr->node->getScale().X / 2.f);
	btVector3 btScaleInternal =btVector3(1.f,1.f,1.f);
	initBtRBC(turret, sphere, btScaleInternal, .1f);

	gameController->registerDeathCallback(turret, fighterDeathExplosionCallback);

	//factions, sensors, and weapons
	initializeFaction(turret, faction, (owner != INVALID_ENTITY));
	initializeDefaultSensors(turret);
	auto hards = turret.get<HardpointComponent>();
	for (u32 i = 0; i < hards->hardpointCount; ++i) {
		initializeWeaponFromId(wepId, turret, i);
	}
	setDamageDifficulty(turret);

	if (faction == FACTION_PLAYER && !cfg->game.toggles[GTOG_IMPACT]) {
		auto hp = turret.get_mut<HealthComponent>();
		hp->healthResistances[IMPACT] = 1.f;
	}

	initializeAI(turret, AI_DEFAULT_REACTION_TIME, 0.01f, 1.f, getCurAiAim(), getCurAiBehaviors());
	auto ai = turret.get_mut<AIComponent>();
	if (ai->hasBehavior(AIBHVR_FLEES_WHEN_TARGETED)) ai->removeBehavior(AIBHVR_FLEES_WHEN_TARGETED);
	if (ai->hasBehavior(AIBHVR_AVOIDS_OBSTACLES)) ai->removeBehavior(AIBHVR_AVOIDS_OBSTACLES);

	//set up the power component
	if (owner != INVALID_ENTITY) {

		PowerComponent dummy;
		dummy.receivingPowerFrom = owner;
		dummy.isPowered = true;
		turret.set<PowerComponent>(dummy);

		setTurretConstraints(turret, owner, slot);
		turret.add<ObstacleDoesNotCollide>();
		turret.add(gameController->doNotCollideWith(), owner);
	}
	else {
		turret.set<PowerComponent>(OBSTACLE_POWER_COMPONENT);
	}
	return turret;
}

flecs::entity createTurretFromArchetype(const dataId archId, const vector3df pos, const vector3df rot, FACTION_TYPE fac,
	const vector3df scale, s32 slot, flecs::entity owner)
{
	auto tdata = turretArchetypeData[archId];
	//note: doesnt apply wep upgrades
	flecs::entity turr = createTurret(tdata->turretDataId, tdata->weps[0], pos, rot, fac, scale, slot, owner);
	turr.set_doc_name(tdata->name.c_str());
	return turr;
}

vector3df getTurretPosition(const vector3df& turretPos, const ISceneNode* parentNode)
{
	vector3df pos = (turretPos * parentNode->getScale());

	matrix4 rotMat;
	rotMat.setRotationDegrees(parentNode->getRotation());
	rotMat.rotateVect(pos);

	pos += parentNode->getPosition();

	return pos;
}

void setTurretConstraints(flecs::entity turret, flecs::entity owner, u32 hardpoint)
{
	if (owner == INVALID_ENTITY || turret == INVALID_ENTITY) {
		return;
	}
	if (!owner.has<BulletRigidBodyComponent>() || !turret.has<BulletRigidBodyComponent>()) return;

	auto turrRBC = turret.get<BulletRigidBodyComponent>();
	auto ownerRBC = owner.get<BulletRigidBodyComponent>();
	auto turrHdp = owner.get<TurretHardpointComponent>();
	auto ownerIrr = owner.get<IrrlichtComponent>();

	btTransform trA, trB;
	trA.setIdentity();
	trB.setIdentity();
	trA.setOrigin(irrVecToBt(turrHdp->turretPositions[hardpoint] * ownerIrr->node->getScale()));
	trB.setOrigin(btVector3(0, 0, 0));
	auto constraint = new btGeneric6DofConstraint(*ownerRBC->rigidBody, *turrRBC->rigidBody, trA, trB, false);

	//turrHdp->turretConstraints[hardpoint] = constraint;
	constraint->setLinearLowerLimit(btVector3(0, 0, 0));
	constraint->setLinearUpperLimit(btVector3(0, 0, 0));
	constraint->setAngularLowerLimit(btVector3(-PI, -PI, -PI));
	constraint->setAngularUpperLimit(btVector3(PI, PI, PI));

	for (u32 j = 0; j < 6; ++j) {
		constraint->setParam(BT_CONSTRAINT_STOP_ERP, 1.f, j);
	}
	bWorld->registerConstraint(constraint, turret, owner);
}

void initializeTurretsOnOwner(flecs::entity owner, const s32 turretId, const s32 wepId)
{
	if (!owner.has<TurretHardpointComponent>()) return;

	auto turrHdp = owner.get_mut<TurretHardpointComponent>();
	auto irr = owner.get<IrrlichtComponent>();
	auto fac = owner.get<FactionComponent>();

	for (u32 i = 0; i < turrHdp->turretCount; ++i) {
		auto turr = createTurret(turretId, wepId, getTurretPosition(turrHdp->turretPositions[i], irr->node), vector3df(0,0,0), fac->type, vector3df(turrHdp->turretScale), i, owner);
		turrHdp->turrets[i] = turr;
	}
}

void initializeTurretsOnOwnerFromArchetypes(flecs::entity owner, const ShipArchetype* ship)
{
	if (!owner.has<TurretHardpointComponent>()) return;

	auto turrHdp = owner.get_mut<TurretHardpointComponent>();
	auto irr = owner.get<IrrlichtComponent>();
	auto fac = owner.get<FactionComponent>();

	//NOTE: this doesnt upgrade turret weapons if there isnt an archetype
	for (u32 i = 0; i < turrHdp->turretCount; ++i) {
		flecs::entity turret = INVALID_ENTITY;
		if (ship->turretArchetypes[i] == INVALID_DATA_ID) continue;
		if (ship->usesTurretArchetype[i] == false) createTurret(ship->turretArchetypes[i], ship->turretWeps[i],
			getTurretPosition(turrHdp->turretPositions[i], irr->node), vector3df(0.f), fac->type, vector3df(5.f), i, owner);
		else turret = createTurretFromArchetype(ship->turretArchetypes[i], getTurretPosition(turrHdp->turretPositions[i], irr->node), 
			vector3df(0.f), fac->type, vector3df(5.f), i, owner);
		turrHdp->turrets[i] = turret;
	}

}