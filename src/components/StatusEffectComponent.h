#pragma once
#ifndef STATUSEFFECTCOMPONENT_H
#define STATUSEFFECTCOMPONENT_H
#include "BaseHeader.h"

enum UNIQUE_STATUS_EFFECT
{
	STATEFF_NOT_UNIQUE,
	STATEFF_SLOW
};

//The basic structure of a status effect is what it does over time (overriding the "apply" function) and how long it lasts.
//The "apply" effect should return true when the effect is complete (or if it's invalid for whatever reason).
struct StatusEffect
{
	f32 duration = 0.f;
	f32 curDuration = 0.f;
	//if unique is set to true, additional applications will set the current duration to 0 instead of adding another
	UNIQUE_STATUS_EFFECT unique = STATEFF_NOT_UNIQUE;
	virtual bool apply(f32 dt, flecs::entity self) = 0;
	bool done() { return (curDuration >= duration); }
	virtual ~StatusEffect() {}
};

//A status effect component describes a specific status effect that it's giving to its entity.
//This can be absolutely anything - damage over time, a slow effect, a speed effect...
struct StatusEffectComponent
{
	std::list<std::shared_ptr<StatusEffect>> effects;
};

#endif 