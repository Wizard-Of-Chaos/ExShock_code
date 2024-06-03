#pragma once
#ifndef SENSORCALLBACKCOMPONENT_H
#define SENSORCALLBACKCOMPONENT_H
#include "BaseHeader.h"
#include <functional>
#include <memory>
#include <list>

struct SensorComponent;
struct SensorCallbackComponent;
struct BulletRigidBodyComponent;

struct SensorCallback
{
	flecs::entity self = INVALID_ENTITY;
	virtual void affect(SensorComponent* sensors, f32 dt) = 0;
	virtual ~SensorCallback() {}
};

struct SensorCallbackComponent
{
	std::shared_ptr<SensorCallback> callback; //thus marking my first ever use of shared pointers
};

struct SlowdownCallback : public SensorCallback
{
	f32 strength = 150.f;
	virtual void affect(SensorComponent* sensors, f32 dt);
};

struct ShieldDropSensorCallback : public SensorCallback
{
	f32 radius = 55.f;
	btVector3 selfPos = btVector3(0, 0, 0);
	f32 dps = 25.f;
	virtual void affect(SensorComponent* sensors, f32 dt);
};

struct GravityYankCallback : public SensorCallback
{
	virtual void affect(SensorComponent* sensors, f32 dt);
	btVector3 selfPos = btVector3(0, 0, 0);
	f32 strength = 180.f;
};

struct RadioactiveDamageCallback : public SensorCallback
{
	virtual void affect(SensorComponent* sensors, f32 dt);
	btVector3 selfPos = btVector3(0, 0, 0);
	f32 dps = 10.f;
};

struct ThrustChangeCallback : public SensorCallback
{
	struct _activeBoost {
		flecs::entity target = INVALID_ENTITY;
		const BulletRigidBodyComponent* rbc = nullptr;
		f32 lifetime = 0.f;
		f32 totalDuration = 5.f;
	};
	std::list<_activeBoost> actives;
	virtual void affect(SensorComponent* sensors, f32 dt);
	btVector3 selfPos = btVector3(0, 0, 0);
	f32 radius = 55.f;
	f32 multiplier = 2.5f;
	virtual ~ThrustChangeCallback();
};

#include "FactionComponent.h"

struct StationCaptureCallback : public SensorCallback
{
	f32 captureTime = 5.f;
	f32 currentCapTime = 0.f;
	btVector3 selfPos = btVector3(0, 0, 0);
	FACTION_TYPE currentlyCapturingFor = FACTION_NEUTRAL;
	virtual void affect(SensorComponent* sensors, f32 dt);
	virtual ~StationCaptureCallback() {}
};

struct SpawnWingCallback : public SensorCallback
{
	u32 charges = 1;
	f32 cooldown = 15.f;
	f32 currentCooldown = 15.f;
	f32 detectionRadius = 1600.f;
	u32 aceShipId = 1;
	u32 aceWepId = 1;
	u32 shipId = 1;
	u32 wepId = 1;
	u32 turretId = 1;
	u32 turretWepId = 1;
	virtual void affect(SensorComponent* sensors, f32 dt);
	virtual ~SpawnWingCallback() {}
};

struct ActivateRockCallback : public SensorCallback
{
	bool active = false;
	virtual void affect(SensorComponent* sensors, f32 dt);
	virtual ~ActivateRockCallback() {}
};

//constructors
SensorCallbackComponent spawnWingJumpscare(flecs::entity self, s32 ace, s32 reg, s32 aceWep, s32 regWep, s32 turret, s32 turretWep, u32 charges = 1);

#endif 