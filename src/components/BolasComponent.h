#pragma once
#ifndef BOLASCOMPONENT_H
#define BOLASCOMPONENT_H
#include "BaseHeader.h"

/*
* The gravity bolas component tracks extra data for the gravity bolas weapon, such as the constraint used for it, the duration,
* the targets involved and a string for the latch sound.
*/
struct BolasInfoComponent
{
	flecs::entity target1;
	flecs::entity target2;
	f32 duration;
	f32 currentDuration;
	f32 timeToHit; //the time required between two hits
	f32 currentTimeToHit;
	f32 force;
	//btTypedConstraint* constraint=nullptr;
	std::string latchSound;
	IVolumeLightSceneNode* effect=nullptr;
};

#endif 