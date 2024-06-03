#include "StationModuleComponent.h"
#include "HealthComponent.h"
#include "ShipUtils.h"

void _toggleShield(bool shieldsUp, flecs::entity mod)
{
	if (!mod.is_alive()) return;
	auto hp = mod.get_mut<HealthComponent>();
	auto smod = mod.get_mut<StationModuleComponent>();
	if (shieldsUp && (smod->type != SPIECE_SHIELD && smod->type != SPIECE_ALIEN_SHIELD && smod->type != SPIECE_BOSS_SHIELD)) { //never shield the generators thats no fun
		hp->shields = 1000.f;
		hp->maxShields = 1000.f;
		hp->rechargeRate = 80.f;
		hp->rechargeDelay = .01f;
	}
	else {
		hp->shields = 0.f;
		hp->maxShields = 0.f;
		hp->rechargeRate = 0.f;
		hp->rechargeDelay = 100000.f;
	}
}

void StationModuleOwnerComponent::toggleShields(bool shieldsUp) {
	for (u32 i = 0; i < MAX_STATION_MODULES; ++i) {
		//work this so that you can bring up shields as well?
		if (!modules[i].is_alive()) continue;
		_toggleShield(shieldsUp, modules[i]);
	}
	_toggleShield(shieldsUp, self);
}

void StationModuleOwnerComponent::removeModule(flecs::entity mod) {
	for (u32 i = 0; i < MAX_STATION_MODULES; ++i) {
		if (modules[i] == mod) {
			modules[i] = INVALID_ENTITY;
			--modCount;
			break;
		}
	}
	for (u32 i = 0; i < MAX_STATION_MODULES; ++i) {
		if (docks[i] == mod) {
			docks[i] = INVALID_ENTITY;
			--dockCount;
			break;
		}
	}
	if (!hasShieldModule()) toggleShields();
}

void StationModuleOwnerComponent::neutralizeStation()
{
	for (u32 i = 0; i < MAX_STATION_MODULES; ++i) {
		if (!modules[i].is_alive()) continue;
		neutralizeModule(modules[i]);
	}
}

void StationModuleOwnerComponent::neutralizeModule(flecs::entity mod)
{
	initializeFaction(mod, FACTION_NEUTRAL, false);
	if (mod.has<TurretHardpointComponent>()) {
		auto turr = mod.get_mut<TurretHardpointComponent>();
		for (u32 i = 0; i < turr->turretCount; ++i) {
			if (turr->turrets[i].is_alive()) initializeFaction(turr->turrets[i], FACTION_NEUTRAL, false);
		}
	}
}

bool StationModuleOwnerComponent::hasShieldModule() {
	for (u32 i = 0; i < MAX_STATION_MODULES; ++i) {
		if (!modules[i].is_alive()) continue;
		auto mod = modules[i].get<StationModuleComponent>();
		if (mod->type == SPIECE_SHIELD || mod->type == SPIECE_ALIEN_SHIELD || mod->type == SPIECE_BOSS_SHIELD) return true;
	}
	return false;
}