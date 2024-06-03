#include "SensorCallbackComponent.h"
#include "SensorComponent.h"
#include "HealthComponent.h"
#include "PlayerComponent.h"
#include "GameController.h"
#include "ObstacleUtils.h"
#include "ThrustComponent.h"
#include "TurretHardpointComponent.h"
#include "ShipUtils.h"
#include "IrrlichtComponent.h"
#include "IrrlichtUtils.h"

void SlowdownCallback::affect(SensorComponent* sensors, f32 dt) {
	if (gameController->stillLoadingHang()) return;
	for (auto& info : sensors->contacts) {
		if (!info.ent.is_alive()) continue;
		if (!info.rbc) continue;
		if (!info.rbc->rigidBody) continue;

		btVector3 antiVelocity = -info.rbc->rigidBody->getLinearVelocity();
		antiVelocity = antiVelocity.safeNormalize();
		info.rbc->rigidBody->applyCentralImpulse(antiVelocity * strength * dt);


		if (info.ent == gameController->getPlayer()) {
			HUDHazard* haz = gameController->getHazard(self, HAZARD::SLOWDOWN);
			if (!haz) {
				haz = gameController->addHazard(self, HAZARD::SLOWDOWN);
				haz->setTimeout();
			}
			haz->timeoutTimer = 0.f;
		}
	}
}

void ShieldDropSensorCallback::affect(SensorComponent* sensors, f32 dt)
{
	if (gameController->stillLoadingHang()) return;
	for (auto &info : sensors->contacts) {
		if (!info.ent.is_alive()) continue;
		f32 dist = std::abs((info.rbc->rigidBody->getCenterOfMassPosition() - selfPos).length());
		if (dist > radius) continue;
		if (!info.ent.has<HealthComponent>()) continue;

		auto shield = info.ent.get_mut<HealthComponent>();
		shield->registerDamageInstance(DamageInstance(self, info.ent, DAMAGE_TYPE::EMP, (dps * dt), device->getTimer()->getTime()));

		if (info.ent == gameController->getPlayer()) {
			HUDHazard* haz = gameController->getHazard(self, HAZARD::SHIELD_DRAIN);
			if (!haz) {
				haz = gameController->addHazard(self, HAZARD::SHIELD_DRAIN);
				haz->setTimeout();
			}
			haz->timeoutTimer = 0.f;
		}
	}
}

void GravityYankCallback::affect(SensorComponent* sensors, f32 dt)
{
	if (gameController->stillLoadingHang()) return;
	for (auto& info : sensors->contacts) {
		if (!info.ent.is_alive()) continue;
		if (!info.rbc) continue;
		if (!info.rbc->rigidBody) continue;

		btVector3 path = selfPos - info.rbc->rigidBody->getCenterOfMassPosition();
		path = path.safeNormalize();
		//this sucker needs to be YOINKED
		info.rbc->rigidBody->applyCentralImpulse(path * strength * dt);
		//initially I tried to do this with constraints, which just seems... silly and unnecessary in retrospect

		if (info.ent == gameController->getPlayer()) {
			HUDHazard* haz = gameController->getHazard(self, HAZARD::GRAVITY_ANOMALY);
			if (!haz) {
				haz = gameController->addHazard(self, HAZARD::GRAVITY_ANOMALY);
				haz->setTimeout();
			}
			haz->timeoutTimer = 0.f;
		}
	}
}

void RadioactiveDamageCallback::affect(SensorComponent* sensors, f32 dt)
{
	if (gameController->stillLoadingHang()) return;
	for (auto& info : sensors->contacts) {
		if (!info.ent.is_alive()) continue;
		if (!info.ent.has<HealthComponent>()) continue;
		auto hp = info.ent.get_mut<HealthComponent>();
		hp->registerDamageInstance(DamageInstance(self, info.ent, DAMAGE_TYPE::ENERGY, dt * dps, device->getTimer()->getTime()));

		if (info.ent == gameController->getPlayer()) {
			HUDHazard* haz = gameController->getHazard(self, HAZARD::RADIATION);
			if (!haz) {
				haz = gameController->addHazard(self, HAZARD::RADIATION);
				haz->setTimeout();
			}
			haz->timeoutTimer = 0.f;
		}
	}
}

void ThrustChangeCallback::affect(SensorComponent* sensors, f32 dt)
{
	if (gameController->stillLoadingHang()) return;
	auto it = actives.begin();
	while (it != actives.end()) {
		if (!it->rbc) {
			++it;
			continue;
		}
		if (!it->rbc->rigidBody) {
			++it;
			continue;
		}
		if (!it->target.is_alive()) {
			++it;
			continue;
		}
		if (!it->target.has<ThrustComponent>()) {
			++it;
			continue;
		}

		f32 dist = std::abs((it->rbc->rigidBody->getCenterOfMassPosition() - selfPos).length());
		if (dist <= radius) {
			it->lifetime = 0;
		}
		it->lifetime += dt;
		if (it->lifetime >= it->totalDuration) {
			it->target.get_mut<ThrustComponent>()->multiplier = 1.f;
			it = actives.erase(it); //get RID of it
			continue;
		}
		++it;
	}

	for (auto& contact : sensors->contacts) {
		if (contact.ent.has<ThrustComponent>()) {
			bool found = false;
			for (auto& active : actives) {
				if (active.target == contact.ent) {
					found = true;
					break;
				}
			}
			if (found) continue;

			f32 dist = std::abs((contact.rbc->rigidBody->getCenterOfMassPosition() - selfPos).length());
			if (dist > radius) continue;
			contact.ent.get_mut<ThrustComponent>()->multiplier = multiplier;
			actives.push_back({ contact.ent, contact.rbc, 0, 5.f });
		}
	}
}

ThrustChangeCallback::~ThrustChangeCallback()
{
	//if the cloud blows up just reset all the damn effects
	for (auto& active : actives) {
		if (!active.target.is_alive()) continue;
		if (!active.target.has<ThrustComponent>()) continue;

		active.target.get_mut<ThrustComponent>()->multiplier = 1.f;
	}
}

void StationCaptureCallback::affect(SensorComponent* sensors, f32 dt)
{
	if (gameController->stillLoadingHang()) return;
	u32 numHostile=0;
	u32 numFriendly=0;
	if (sensors->contacts.empty()) return;

	for (auto& ctct : sensors->contacts) {
		if (ctct.fac->type == FACTION_HOSTILE) ++numHostile;
		if (ctct.fac->type == FACTION_PLAYER) ++numFriendly;
	}
	if (numHostile == 0 && numFriendly == 0) {
		currentCapTime = 0.f;
		currentlyCapturingFor = FACTION_NEUTRAL;
		return;
	}
	if (numHostile > numFriendly) {
		if (currentlyCapturingFor != FACTION_HOSTILE) {
			currentCapTime = 0.f;
			currentlyCapturingFor = FACTION_HOSTILE;
		}
		currentCapTime += dt;
	}
	else if (numFriendly > numHostile) {
		if (currentlyCapturingFor != FACTION_PLAYER) {
			currentCapTime = 0.f;
			currentlyCapturingFor = FACTION_PLAYER;

		}
		currentCapTime += dt;
	}
	//if the number of hostiles and number of friendlies balances, do nothing
	//otherwise swap factions
	if (currentCapTime >= captureTime) {
		currentCapTime = 0.f;
		std::function<void(flecs::entity, bool)> func = initializeNeutralFaction;
		if (currentlyCapturingFor == FACTION_HOSTILE) func = initializeHostileFaction;
		else if (currentlyCapturingFor == FACTION_PLAYER) func = initializePlayerFaction;

		func(self, true); //something about this feels extremely silly
		if (self.has<TurretHardpointComponent>()) {
			auto turr = self.get<TurretHardpointComponent>();
			for (u32 i = 0; i < turr->turretCount; ++i) {
				func(turr->turrets[i], true); //definitely silly
			}
		}
	}
}

void SpawnWingCallback::affect(SensorComponent* sensors, f32 dt)
{
	if (charges == 0) return;
	if (currentCooldown < cooldown) {
		currentCooldown += dt;
		return;
	}
	for (auto& ctct : sensors->contacts) {
		if (!ctct.ent.has<PlayerComponent>()) continue;
		if (ctct.dist <= detectionRadius) {
			auto irr = ctct.ent.get<IrrlichtComponent>();
			vector3df pos = irr->node->getPosition();
			pos += getNodeForward(irr->node) * 700.f;
			spawnScenarioWing(pos, vector3df(0, 0, 0), shipId, wepId, aceShipId, aceWepId, turretId, turretWepId);
			charges -= 1;
			currentCooldown = 0;
			return;
		}
	}
}

void ActivateRockCallback::affect(SensorComponent* sensors, f32 dt)
{
	if (gameController->stillLoadingHang()) return;
	if (active) return;
	for (auto& ctct : sensors->contacts) {
		if (!ctct.ent.has<PlayerComponent>()) continue;
		//player is too close and this hasn't been activated yet
		hugeRockJumpscare(self);
		active = true;
	}
}

//constructors
SensorCallbackComponent spawnWingJumpscare(flecs::entity self, s32 ace, s32 reg, s32 aceWep, s32 regWep, s32 turret, s32 turretWep, u32 charges)
{
	SensorCallbackComponent cb;
	auto spawner = new SpawnWingCallback();
	spawner->aceShipId = ace;
	spawner->aceWepId = aceWep;
	spawner->shipId = reg;
	spawner->wepId = regWep;
	spawner->turretId = turret;
	spawner->turretWepId = turretWep;
	spawner->self = self;
	spawner->detectionRadius = 1600.f;
	spawner->charges += charges;
	cb.callback = std::shared_ptr<SensorCallback>(spawner);
	return cb;
}
