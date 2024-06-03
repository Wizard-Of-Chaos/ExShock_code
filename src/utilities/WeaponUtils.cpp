#include "WeaponUtils.h"
#include "GameController.h"
#include "GameStateController.h"
#include "StatusEffects.h"
#include "BulletRigidBodyComponent.h"
#include "ProjectileUtils.h"
#include "AudioDriver.h"
#include "Config.h"
#include "FactionComponent.h"
#include "IrrlichtComponent.h"
#include "GameFunctions.h"
#include "BolasComponent.h"
#include "CrashLogger.h"

const std::unordered_map<std::string, ProjInfo_hitCb> impactCbStrings =
{
	{"basic_damage", wepHit_projDamage},
	{"laser_damage", wepHit_laserDamage},
	{"instant_kill", wepHit_instantKill},
	{"blast", wepHit_impulseBlast},
	{"bolas", wepHit_bolas},
	{"missile_explosion", wepHit_missileExplosion},
	{"knockback", wepHit_projKnockback},
	{"phys_grapple", wepHit_energyGrapple},
	{"phys_hook", wepHit_physHook},
	{"phys_slowdown_bang", wepHit_physSlowdownExplosion},
	{"bfg_explosion", wepHit_bfgExplosion}
};

const std::unordered_map<std::string, WepInfo_UpdateCb> updateCbStrings =
{
	{"basic", wepUpdate_basic},
	{"bolas", wepUpdate_bolas},
	{"railgun", wepUpdate_railgun},
	{"goron", wepUpdate_goron}
};

const std::unordered_map<std::string, WepInfo_FireCb> fireCbStrings =
{
	{"kinetic", wepFire_kinetic},
	{"kinetic_kick", wepFire_kineticKick},
	{"plasma", wepFire_plasma},
	{"rock", wepFire_rock},
	{"missile", wepFire_missile},
	{"railgun", wepFire_railgun},
	{"laser", wepFire_laser},
	{"thick_laser", wepFire_thickLaser},
	{"goron", wepFire_goron}
};


void handleProjectileHit(flecs::entity projectile, flecs::entity impacted)
{
	auto proj = projectile.get_mut<ProjectileInfoComponent>();
	proj->hit = true;
	audioDriver->playGameSound(impacted, proj->impactSound);
	auto rbc = projectile.get<BulletRigidBodyComponent>();

	for (ProjInfo_hitCb effect : proj->hitEffects) {
		effect(projectile, impacted, rbc->rigidBody->getCenterOfMassPosition());
	}

	if (gameController->hasHitCallback(impacted)) gameController->hitCallbacks[impacted](impacted, projectile);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void wepHit_projDamage(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint)
{
	if (!impacted.has<HealthComponent>()) return;
	auto proj = projectile.get<ProjectileInfoComponent>();
	auto rbc = projectile.get<BulletRigidBodyComponent>();

	vector3df posLocal = btVecToIrr(rbc->rigidBody->getCenterOfMassTransform().inverse()(rbc->rigidBody->getCenterOfMassPosition()));

	auto hp = impacted.get_mut<HealthComponent>();
	if (impacted.has<HealthComponent>()) {
		auto hp = impacted.get_mut<HealthComponent>();
		hp->registerDamageInstance(
			DamageInstance(projectile.target(flecs::ChildOf), impacted, proj->dmgtype, proj->damage,
				device->getTimer()->getTime(), posLocal));
	}
}

void wepHit_laserDamage(flecs::entity wep, flecs::entity impacted, btVector3 hitPoint)
{
	if (!impacted.has<HealthComponent>()) return;
	auto wepInfo = wep.get<WeaponInfoComponent>();

	if (impacted.has<HealthComponent>()) {
		auto hp = impacted.get_mut<HealthComponent>();
		hp->registerDamageInstance(
			DamageInstance(wep.target(flecs::ChildOf), impacted, wepInfo->dmgtype, wepInfo->damage,
				device->getTimer()->getTime()));
	}
}

void wepHit_instantKill(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint)
{
	if (!impacted.has<HealthComponent>()) return;
	auto hp = impacted.get_mut<HealthComponent>();
	hp->health = 0.f;
}

void wepHit_projKnockback(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint)
{
	if (!impacted.has<BulletRigidBodyComponent>()) return;
	auto rbc = impacted.get_mut<BulletRigidBodyComponent>();
	auto projInfo = projectile.get<ProjectileInfoComponent>();
	vector3df firingDir;
	if (projectile.has<ProjectileInfoComponent>()) {
		//this isn't a laser
		firingDir = projectile.get<ProjectileInfoComponent>()->fireDir;
	}
	else {
		//this is a laser
		firingDir = projectile.get<WeaponFiringComponent>()->firingDirection;
	}
	rbc->rigidBody->applyCentralForce(irrVecToBt(firingDir) * 450.f);
	audioDriver->playGameSound(impacted, "impact_1.ogg");
}

void wepHit_impulseBlast(flecs::entity projId, flecs::entity impacted, btVector3 hitPoint)
{
	auto proj = projId.get<ProjectileInfoComponent>();
	explode(btVecToIrr(hitPoint), 1.5f, 9.f, 190.f, proj->damage, 575.f, EXTYPE_TECH);
}

void wepHit_bfgExplosion(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint)
{
	explode(btVecToIrr(hitPoint), 1.5f, 9.f, 240.f, 30.f, 125.f, EXTYPE_TECH);
	wepHit_projKnockback(projectile, impacted, hitPoint);
}

void wepHit_missileExplosion(flecs::entity projId, flecs::entity impacted, btVector3 hitPoint)
{
	auto proj = projId.get<ProjectileInfoComponent>();
	auto irr = projId.get<IrrlichtComponent>();

	explode(irr->node->getAbsolutePosition(), 1.f, 1.f, 180.f, proj->damage, 200.f);
}

void wepHit_bolas(flecs::entity projId, flecs::entity impacted, btVector3 hitPoint)
{
	flecs::entity weapon = projId.target(gameController->firedBy());
	if (!weapon.is_alive()) return;
	if (!weapon.has<BolasInfoComponent>()) return;

	auto bolasInfo = weapon.get_mut<BolasInfoComponent>();

	if (bolasInfo->target1.is_alive() && bolasInfo->target2.is_alive()) return; //one set of targets at a time

	if (!bolasInfo->target1.is_alive()) {
		bolasInfo->target1 = impacted;
	}
	else if (!bolasInfo->target2.is_alive()) {
		if (bolasInfo->target1 == impacted) return;
		bolasInfo->target2 = impacted;

		if (!bolasInfo->target1.has<BulletRigidBodyComponent>() || !bolasInfo->target2.has<BulletRigidBodyComponent>()) {
			bolasInfo->target1 = INVALID_ENTITY;
			bolasInfo->target2 = INVALID_ENTITY;
			return;
		}
		auto rbcA = bolasInfo->target1.get_mut<BulletRigidBodyComponent>();
		auto rbcB = bolasInfo->target2.get_mut<BulletRigidBodyComponent>();

		audioDriver->playGameSound(impacted, bolasInfo->latchSound);

		btTransform tr;
		btVector3 ori(0, 0, 0);
		tr.setIdentity();
		tr.setOrigin(ori);
		auto p2p = new btGeneric6DofConstraint(*rbcA->rigidBody, *rbcB->rigidBody, tr, tr, false);
		p2p->setLinearLowerLimit(btVector3(0, 0, 0));
		p2p->setLinearUpperLimit(btVector3(0, 0, 0));
		p2p->setAngularLowerLimit(btVector3(-PI, -PI, -PI));
		p2p->setAngularUpperLimit(btVector3(PI, PI, PI));
		for (u32 i = 0; i < 6; ++i) {
			f32 forceFactor = .005f * bolasInfo->force;
			p2p->setParam(BT_CONSTRAINT_STOP_ERP, forceFactor, i);
		}
		//bolasInfo->constraint = p2p;
		//bWorld->addConstraint(bolasInfo->constraint);
		bWorld->registerConstraint(p2p, bolasInfo->target1, bolasInfo->target2, bolasInfo->duration);
		u32 lvl = 64;
		lvl *= ((u32)cfg->vid.particleLevel + 1);

		btVector3 dist = (rbcB->rigidBody->getCenterOfMassPosition() - rbcA->rigidBody->getCenterOfMassPosition());
		btQuaternion rot = shortestArcQuat(irrVecToBt(vector3df(0, 1, 0)), dist.normalized());
		btVector3 btRot;
		QuaternionToEuler(rot, btRot);

		f32 size = 5.f;
		f32 mult = 1.f;
		if (bolasInfo->target1.has<IrrlichtComponent>()) mult += bolasInfo->target1.get<IrrlichtComponent>()->node->getScale().X / 2.f;
		if (bolasInfo->target2.has<IrrlichtComponent>()) mult += bolasInfo->target2.get<IrrlichtComponent>()->node->getScale().X / 2.f;

		bolasInfo->effect = smgr->addVolumeLightSceneNode(0, ID_IsNotSelectable, lvl, lvl, SColor(0, 255, 255, 255), SColor(0, 255, 255, 255),
			btVecToIrr(rbcA->rigidBody->getCenterOfMassPosition()), btVecToIrr(btRot), vector3df(size * mult, dist.length(), size * mult));

		auto anim = getBolasTextureAnim();
		bolasInfo->effect->addAnimator(anim);
		auto deleteAnim = smgr->createDeleteAnimator((u32)(bolasInfo->duration * 1000.f) + 50);
		bolasInfo->effect->addAnimator(deleteAnim);
		anim->drop();
	}
}

void wepHit_energyGrapple(flecs::entity weapon, flecs::entity impacted, btVector3 hitPoint)
{
	auto ship = weapon.target(flecs::ChildOf);
	if (ship == INVALID_ENTITY) {
		baedsLogger::errLog("Ship firing energy grapple is not valid.\n");
		return;
	}
	if (impacted == INVALID_ENTITY) {
		baedsLogger::errLog("Ship firing energy grapple hit invalid target.\n");
	}

	btVector3 targPos;
	if (impacted.has<BulletRigidBodyComponent>()) {
		auto impactRBC = impacted.get_mut<BulletRigidBodyComponent>();
		auto body = impactRBC->rigidBody;
		targPos = body->getCenterOfMassPosition();

		//chop the speed on the target
		std::shared_ptr<StatusEffect> effect(new SlowdownEffect);
		effect->duration = 2.5f;
		impacted.get_mut<StatusEffectComponent>()->effects.push_back(effect);

	}
	else if (impacted.has<IrrlichtComponent>()) {
		targPos = irrVecToBt(impacted.get<IrrlichtComponent>()->node->getAbsolutePosition());
	}
	else {
		baedsLogger::errLog("Target has no method to position grapple.\n");
	}

	if (ship.has<BulletRigidBodyComponent>()) {
		auto rbc = ship.get_mut<BulletRigidBodyComponent>();
		btVector3 dir = (targPos - rbc->rigidBody->getCenterOfMassPosition());
		dir.safeNormalize();
		rbc->rigidBody->applyCentralImpulse(dir * 500.f);
	}

}

void wepHit_physHook(flecs::entity weapon, flecs::entity impacted, btVector3 hitPoint)
{
	auto ship = weapon.target(flecs::ChildOf);
	if (ship == INVALID_ENTITY || !ship.has<BulletRigidBodyComponent>()) {
		baedsLogger::errLog("Ship firing physics hook is not valid.\n");
		return;
	}
	if (impacted == INVALID_ENTITY || !impacted.has<BulletRigidBodyComponent>()) {
		baedsLogger::errLog("Ship firing physics hook hit invalid target.\n");
		return;
	}

	auto impactRBC = impacted.get_mut<BulletRigidBodyComponent>();
	auto body = impactRBC->rigidBody;
	btVector3 targPos = body->getCenterOfMassPosition();
	btVector3 shipPos = ship.get<BulletRigidBodyComponent>()->rigidBody->getCenterOfMassPosition();

	btVector3 dir = (shipPos - targPos);
	dir.safeNormalize();
	body->applyCentralImpulse(dir * 500.f);

	std::shared_ptr<StatusEffect> effect(new SlowdownEffect);
	effect->duration = 2.5f;
	impacted.get_mut<StatusEffectComponent>()->effects.push_back(effect);
}

void wepHit_physSlowdownExplosion(flecs::entity projId, flecs::entity impacted, btVector3 hitPoint)
{
	auto rbc = projId.get<BulletRigidBodyComponent>();

	btVector3 center = rbc->rigidBody->getCenterOfMassPosition();
	btSphereShape shape(350.f);
	btPairCachingGhostObject ghost;
	btTransform transform;
	transform.setOrigin(center);
	ghost.setCollisionShape(&shape);
	ghost.setWorldTransform(transform);
	bWorld->addCollisionObject(&ghost);

	for (s32 i = 0; i < ghost.getNumOverlappingObjects(); ++i) {
		btCollisionObject* obj = ghost.getOverlappingObject(i);
		flecs::entity objId = getIdFromBt(obj);

		if (!objId.is_alive()) continue;
		if (!objId.has<BulletRigidBodyComponent>()) continue;
		if (objId.has<FactionComponent>() && cfg->game.toggles[GTOG_FRIENDLY_FIRE]) {
			if(objId.get<FactionComponent>()->isFriendly(projId.get<FactionComponent>())) continue;
		}
		if (objId.has<ProjectileInfoComponent>()) continue;

		std::shared_ptr<StatusEffect> effect(new SlowdownEffect);
		effect->duration = 5.f;
		objId.get_mut<StatusEffectComponent>()->effects.push_back(effect);

		btRigidBody* body = (btRigidBody*)obj;
		explode(btVecToIrr(body->getCenterOfMassPosition()), 3.f, 5.f, 25.f, 0.f, 0.f, EXTYPE_TECH);
	}
	bWorld->removeCollisionObject(&ghost);

	explode(btVecToIrr(center), 3.f, 35.f, 350.f, 0.f, 0.f, EXTYPE_TECH);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void wepUpdate_basic(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt)
{
	if (wep->usesAmmunition)
	{
		if (fire->clip == 0)
		{
			fire->timeReloading += dt;
			if (fire->timeReloading >= wep->reloadTime && fire->ammunition > 0) {
				fire->timeReloading = 0;
				fire->clip = wep->maxClip;
				fire->ammunition -= wep->maxClip;
				fire->hasFired = false;
				fire->timeSinceLastShot = wep->firingSpeed;
				audioDriver->playGameSound(wepEnt, "reload.ogg", .9f, 20.f, 1200.f, false, true);
			}
		}
	}
	fire->timeSinceLastShot += dt;
}

void wepUpdate_railgun(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt)
{
	if (wep->hasFired) {
		if (fire->timeReloading >= wep->firingSpeed) {
			gameController->setFire(wepEnt);
		}
	}
}

void wepUpdate_bolas(WeaponInfoComponent* wepComp, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt)
{
	if (!wepEnt.has<BolasInfoComponent>() || !wepEnt.is_alive()) return;
	auto bolasInfo = wepEnt.get_mut<BolasInfoComponent>();
	if (!bolasInfo->target1.is_alive()) {
		bolasInfo->target2 = INVALID_ENTITY;
		if (bolasInfo->effect) bolasInfo->effect->remove();
		bolasInfo->effect = nullptr;
		return;
	}
	if (bolasInfo->target1.is_alive() && !bolasInfo->target2.is_alive()) {
		bolasInfo->currentTimeToHit += dt;
		if (bolasInfo->currentTimeToHit >= bolasInfo->timeToHit) {
			bolasInfo->target1 = INVALID_ENTITY;
			bolasInfo->target2 = INVALID_ENTITY;
			bolasInfo->currentTimeToHit = 0;
			if (bolasInfo->effect) bolasInfo->effect->remove();
			bolasInfo->effect = nullptr;
			return;
		}
	}
	else if (bolasInfo->target1.is_alive() && bolasInfo->target2.is_alive()) {
		bolasInfo->currentDuration += dt;
		btVector3 dist = (bolasInfo->target2.get<BulletRigidBodyComponent>()->rigidBody->getCenterOfMassPosition() 
			- bolasInfo->target1.get<BulletRigidBodyComponent>()->rigidBody->getCenterOfMassPosition());
		btQuaternion rot = shortestArcQuat(irrVecToBt(vector3df(0, 1, 0)), dist.normalized());
		btVector3 btRot;
		QuaternionToEuler(rot, btRot);
		bolasInfo->effect->setRotation(btVecToIrr(btRot));
		vector3df scale = bolasInfo->effect->getScale();
		scale.Y = dist.length();
		bolasInfo->effect->setScale(scale);
		bolasInfo->effect->setPosition(btVecToIrr(bolasInfo->target1.get<BulletRigidBodyComponent>()->rigidBody->getCenterOfMassPosition()));

		if (bolasInfo->currentDuration >= bolasInfo->duration) {
			bolasInfo->target1 = INVALID_ENTITY;
			bolasInfo->target2 = INVALID_ENTITY;
			bolasInfo->currentDuration = 0.f;
			if(bolasInfo->effect) bolasInfo->effect->remove();
			bolasInfo->effect = nullptr;
		}
	}
}

void wepUpdate_goron(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt)
{
	fire->timeSinceLastShot += dt;
	if (!wep->hasFired) {
		fire->timeSinceLastShot = wep->firingSpeed;
		wep->hasFired = true;
	}
	if (fire->timeSinceLastShot >= 8.f && fire->hasFired) {
		flecs::entity ship = wepEnt.target(flecs::ChildOf);
		if (!ship.is_alive()) {
			return;
		}
		if (ship.has<HealthComponent>()) {
			auto hp = ship.get_mut<HealthComponent>();
			for (u32 i = 0; i < MAX_DAMAGE_TYPES; ++i) {
				hp->healthResistances[i] -= .8f;
				hp->shieldResistances[i] -= .8f;
			}
		}
		fire->hasFired = false;
	}
}