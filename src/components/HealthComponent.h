#pragma once

#ifndef HEALTHCOMPONENT_H
#define HEALTHCOMPONENT_H
#include "BaseHeader.h"
#include <unordered_map>

// Different types of damage.
enum DAMAGE_TYPE {
	NONE,
	ENERGY,
	EXPLOSIVE,
	KINETIC, //bullets
	IMPACT, //smacking into something
	VELOCITY, // moving too fast
	EMP,
	MAX_DAMAGE_TYPES
};

//Convenience map for strings to types used for loading weapon data.
const std::unordered_map<std::string, DAMAGE_TYPE> damageStrings = {
	{"none", DAMAGE_TYPE::NONE},
	{"energy", ENERGY},
	{"explosive", EXPLOSIVE},
	{"kinetic", KINETIC},
	{"impact", IMPACT},
	{"velocity", VELOCITY},
	{"emp", EMP}
};

//Default maximum for health is 100.
const f32 DEFAULT_MAX_HEALTH = 100.f;
const f32 DEFAULT_HEALTH_REGEN = 0.f;

const f32 DEFAULT_MAX_SHIELDS = 100.f;
const f32 DEFAULT_RECHARGE_RATE = 10.f;
const f32 DEFAULT_RECHARGE_DELAY = 5.f;

const s32 MAX_TRACKED_DAMAGE_INSTANCES = 4;


/*
* A damage instance incldues the time of being struck, who its from, and who it's to, along with the type and amount.
* They are handled properly in the DamageSystem.cpp file.
*/
struct DamageInstance
{
	DamageInstance(flecs::entity fr, flecs::entity to, DAMAGE_TYPE type, f32 amt, u32 time, vector3df hitPos=vector3df(0,0,0)) : from(fr), to(to), type(type), amount(amt), time(time), hitPos(hitPos) {}
	u32 time;
	flecs::entity from;
	flecs::entity to;
	vector3df hitPos;
	DAMAGE_TYPE type;
	f32 amount;
};

struct Net_DamageInstance
{
	Net_DamageInstance(NetworkId fr, NetworkId to, DAMAGE_TYPE type, f32 amt, u32 time, vector3df hitPos = vector3df(0, 0, 0)) : from(fr), to(to), type(type), amount(amt), time(time), hitPos(hitPos) {}
	Net_DamageInstance() { 
		time = 0;
		from = INVALID_NETWORK_ID;
		to = INVALID_NETWORK_ID;
		hitPos = vector3df();
		type = MAX_DAMAGE_TYPES;
		amount = 0;
	}
	u32 time = 0;
	NetworkId from = INVALID_NETWORK_ID;
	NetworkId to = INVALID_NETWORK_ID;
	vector3df hitPos = vector3df();
	DAMAGE_TYPE type = MAX_DAMAGE_TYPES;
	f32 amount = 0;
};

/*
* It feels like this should be obvious, but the health component stores the current and max
* health of whatever it's attached to. In the health system, an object at 0 health gets deleted.
* A health component also tracks damage instances, which handle what type of damage and how much.
*/
struct HealthComponent
{
	std::vector<DamageInstance> instances;
	std::list<DamageInstance> recentInstances;
	DAMAGE_TYPE lastDamageType = DAMAGE_TYPE::NONE;
	u32 lastDamageTime;
	f32 healthResistances[MAX_DAMAGE_TYPES] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
	void registerDamageInstance(DamageInstance dmg) {
		instances.push_back(dmg);
	}
	f32 health=DEFAULT_MAX_HEALTH;
	f32 maxHealth = DEFAULT_MAX_HEALTH;
	f32 healthRegen = DEFAULT_HEALTH_REGEN;

	f32 shieldResistances[MAX_DAMAGE_TYPES] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
	bool wasHitLastFrame = false;
	f32 shieldsLastFrame = 0.f;
	f32 shields = 0.f;
	f32 maxShields = 0.f;
	f32 rechargeRate = 0.f; //how much shields per second
	f32 rechargeDelay = 0.f; //how long before recharge, in seconds
	f32 timeSinceLastHit = 0.f; //how long since a hit, in seconds
};

#endif