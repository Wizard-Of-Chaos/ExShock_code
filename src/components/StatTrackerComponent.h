#pragma once
#ifndef STATTRACKERCOMPONENT_H
#define STATTRACKERCOMPONENT_H
#include "BaseHeader.h"

/*
* The stat tracker component is purely for wingmen and the player to keep track of how many kills they get in a given scenario so the campaign
* can get updated with the appropriate count.
*/
struct StatTrackerComponent
{
	u32 kills = 0;
};

#endif 