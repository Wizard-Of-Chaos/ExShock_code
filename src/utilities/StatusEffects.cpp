#include "StatusEffects.h"
#include "IrrlichtComponent.h"
#include "ShipUtils.h"
#include "IrrlichtUtils.h"
#include "HealthComponent.h"
#include "GameController.h"
#include "BulletRigidBodyComponent.h"

struct DPSEffect : public StatusEffect
{
	DAMAGE_TYPE type = ENERGY;
	f32 damagePerSecond = 10.f;
	flecs::entity from = INVALID_ENTITY;

	virtual bool apply(f32 dt, flecs::entity self) {
		if (done()) return true;
		if (!self.is_alive()) return true;
		if (!self.has<HealthComponent>()) return true;

		curDuration += dt;
		
		auto hp = self.get_mut<HealthComponent>();
		hp->registerDamageInstance(DamageInstance(from, self, type, dt * damagePerSecond, device->getTimer()->getTime()));
	}
};

bool StickySpawnerEffect::apply(f32 dt, flecs::entity self)
{
	if (done()) return true;
	if (!self.is_alive()) return true;
	if (!self.has<IrrlichtComponent>()) return true;

	curInterval += dt;
	curDuration += dt;
	auto irr = self.get<IrrlichtComponent>();
	if (!irr) return true; //no irr? we're outta here
	if (charges == 0) return true; //we outta charges? we're outta here
	if (curInterval >= spawnInterval) {
		vector3df pos = irr->node->getPosition();
		pos += getNodeForward(irr->node) * 700.f;
		spawnScenarioWing(pos, vector3df(0, 0, 0), shipId, wepId, aceShipId, aceWepId, turretId, turretWepId);
		charges -= 1;
		curInterval = 0.f;
	}
	return false;
}

bool StickySpawnerScenarioEffect::apply(f32 dt, flecs::entity self)
{

	if (done()) return true;
	if (!self.is_alive()) return true;
	if (!self.has<IrrlichtComponent>()) return true;

	curInterval += dt;

	auto irr = self.get<IrrlichtComponent>();
	if (!irr) return true; //no irr? we're outta here

	if (curInterval >= spawnInterval - 3.5f) {
		if (random.percentChance(percentageChance) && !aboutToSpawn && !rolled) { //only roll ONCE
			aboutToSpawn = true;
			gameController->addLargePopup(L"Sir, we've detected hostile signals on an intercept course! You've got incoming fighters!", L"Kate Dietric");
		}
		rolled = true;
	}

	if (curInterval >= spawnInterval) {
		if (aboutToSpawn) {
			vector3df pos = irr->node->getPosition();
			vector3df wingpos = pos + (getNodeForward(irr->node) * 700.f);
			vector3df rot = (pos - wingpos).getHorizontalAngle();
			spawnScenarioWing(wingpos, rot, shipId, wepId, aceShipId, aceWepId, turretId, turretWepId);
		}
		curInterval = 0.f;
		aboutToSpawn = false;
		rolled = false;
	}
	return false;
}

bool StickyVariableWaveSpawnerEffect::apply(f32 dt, flecs::entity self)
{
	if (done()) return true;
	if (!self.is_alive()) return true;
	if (!self.has<IrrlichtComponent>()) return true;

	curInterval += dt;
	curDuration += dt;
	auto irr = self.get<IrrlichtComponent>();
	if (!irr) return true; //no irr? we're outta here
	if (waves.empty()) return true;

	bool shouldSpawn = false;

	bool waveDead = true;
	for (auto& ent : currentWave) {
		if (ent.is_alive()) {
			waveDead = false;
			break;
		}
	}
	if (!constantSpawns && waveDead) shouldSpawn = true;
	else if (curInterval >= spawnInterval) shouldSpawn = true;

	if (shouldSpawn) {
		vector3df pos = irr->node->getPosition();
		pos += getNodeForward(irr->node) * 900.f;
		_wave w = waves.front();
		waves.pop_front();
		currentWave = spawnScenarioWing(pos, vector3df(0, 0, 0), w.shipId, w.wepId, w.aceShipId, w.aceWepId, w.turretId, w.turretWepId);
		curInterval = 0.f;
	}
	return false;
}

bool SlowdownEffect::apply(f32 dt, flecs::entity self)
{
	if (done()) return true;
	if (!self.is_alive()) return true;
	if (!self.has<BulletRigidBodyComponent>()) return true;
	curDuration += dt;

	auto rbc = self.get_mut<BulletRigidBodyComponent>();

	btVector3 antiVelocity = -rbc->rigidBody->getLinearVelocity();
	f32 antiVelMag = antiVelocity.length();
	btVector3 antiTorque = -rbc->rigidBody->getAngularVelocity();
	f32 antiAngMag = antiTorque.length();
	antiVelocity = antiVelocity.safeNormalize();
	antiTorque = antiTorque.safeNormalize();

	rbc->rigidBody->applyCentralImpulse(antiVelocity * (antiVelMag * .95f) * dt);
	rbc->rigidBody->applyTorqueImpulse(antiTorque * (antiAngMag * .95f) * dt);

	if (!started) {
		rbc->rigidBody->applyCentralImpulse(antiVelocity * (antiVelMag * strength)); //initial slow if it hasn't started
		rbc->rigidBody->applyTorqueImpulse(antiTorque * (antiAngMag * strength));
		started = true;
	}
	return false;
}