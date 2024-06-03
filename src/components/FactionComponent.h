#pragma once

#ifndef FACTIONCOMPONENT_H
#define FACTIONCOMPONENT_H
#include "BaseHeader.h"

//The different types of factions in the game.
//These act as flags on a bit-mask.
enum FACTION_TYPE
{
	FACTION_NEUTRAL = 1,
	FACTION_PLAYER = 2,
	FACTION_HOSTILE = 4,
	FACTION_UNCONTROLLED = 8
};

/*
* The faction component holds the type of faction (neutral, friendly, etc)
* as well as the factions that it's hostile to as an unsigned int (checked with bitmasking).
* Frankly, there's no need for this with the current three factions, but it's designed to be easy
* to expand later.
* IsHostile and IsFriendly will return whether or not the thing is hostile to a given other faction component.
*/
struct FactionComponent
{
	FACTION_TYPE type = FACTION_NEUTRAL;
	u32 hostileTo = 0;
	u32 friendlyTo = 0;
	//determines whether or not the thing gets a large icon on the HUD
	bool isImportant = true;
	//Checks whether or not the other faction component would be hostile to this one.
	bool isHostile(const FactionComponent* other) const {
		if (!other) return false;
		return !!(hostileTo & other->type);
	}
	//Checks whether or not the other faction component would be friendly to this one.
	bool isFriendly(const FactionComponent* other) const {
		if (!other) return false;
		return !!(friendlyTo & other->type);
	}
};
#endif 