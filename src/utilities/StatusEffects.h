#pragma once
#ifndef STATUSEFFECTS_H
#define STATUSEFFECTS_H
#include "BaseHeader.h"
#include "StatusEffectComponent.h"

//Deals damage over time to the entity in question.
struct DPSEffect;
//Spawns waves of enemies at the given interval in front of the target.
//If charges is not set, it will continue spawning waves until it runs out of time.
struct StickySpawnerEffect : public StatusEffect
{
	virtual bool apply(f32 dt, flecs::entity self);
	s32 charges = -1;
	f32 spawnInterval = 15.f;
	f32 curInterval = 0.f;
	s32 aceShipId = 1;
	s32 aceWepId = 1;
	s32 shipId = 1;
	s32 wepId = 1;
	s32 turretId = 1;
	s32 turretWepId = 1;
};

//As above, but does it at random in the specified intervals and is permanent.
struct StickySpawnerScenarioEffect : public StickySpawnerEffect
{
	StickySpawnerScenarioEffect() : StickySpawnerEffect() { duration = 1.f; curDuration = 0.f; spawnInterval = 120.f; };
	virtual bool apply(f32 dt, flecs::entity self) override;
	u32 percentageChance = 15;
	bool aboutToSpawn = false;
	bool rolled = false;
};

//Spawns set waves in set intervals.
struct StickyVariableWaveSpawnerEffect : public StatusEffect
{
	virtual bool apply(f32 dt, flecs::entity self);
	struct _wave {
		s32 aceShipId = 1;
		s32 aceWepId = 1;
		s32 shipId = 1;
		s32 wepId = 1;
		s32 turretId = 1;
		s32 turretWepId = 1;
	};
	std::list<_wave> waves;
	std::list<flecs::entity> currentWave;
	bool constantSpawns = false;
	void addWave(s32 ace, s32 aceWep, s32 ship, s32 shipWep, s32 turret, s32 turretWep) {
		waves.push_back({ ace, aceWep, ship, shipWep, turret, turretWep });
	}
	f32 spawnInterval = 15.f;
	f32 curInterval = 0.f;
};

struct SlowdownEffect : public StatusEffect
{
	virtual bool apply(f32 dt, flecs::entity self);
	bool started = false;
	f32 strength = .75f;
};

#endif