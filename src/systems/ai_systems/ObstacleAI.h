#pragma once
#ifndef OBSTACLEAI_H
#define OBSTACLEAI_H
#include "BaseHeader.h"
#include "AITypes.h"
#include "FactionComponent.h"

class MineAI : public AIType
{
public:
	virtual void stateCheck(aiSystemInfo& inf);
	virtual void pursue(aiSystemInfo& inf);
	flecs::entity self;
	FACTION_TYPE triggeredBy = FACTION_NEUTRAL;
protected:

};

class MissileAI : public AIType
{
public:
	virtual void stateCheck(aiSystemInfo& inf);
	virtual void pursue(aiSystemInfo& inf);

	flecs::entity self;
	flecs::entity target;
	f32 timeChasing = 0;
	f32 lifetime = 15.f;
	f32 kablooeyDistance = 40.f;
	f32 lowAngle = 10.f;
	f32 highAngle = 70.f;
};
#endif 