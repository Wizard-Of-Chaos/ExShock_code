#pragma once
#ifndef POWERCOMPONENT_H
#define POWERCOMPONENT_H

#include "BaseHeader.h"

struct PowerComponent
{
	//this one is just to check whether or not the damn thing is disabled (like from a weapon)
	bool disabled = false;
	//isPowered is used to see if the thing is actually capable of *using* power at all, either from its own generation or from a connected entity
	bool isPowered = false;
	//is this sucker capable of generating power?
	bool generator = false;
	f32 power=0.f;
	f32 maxPower=0.f;
	f32 powerRegen=0.f;
	flecs::entity receivingPowerFrom = INVALID_ENTITY;
	//returns 0 if the requested power is not available
	f32 getPower(const f32& amt) {
		if (disabled) return 0.f;
		//if it's not powered that's just an L bozo
		if (!isPowered) return 0.f;
		//if it's generating its own power we just check that straight-up
		if (generator) {
			if(power >= amt) {
				power -= amt;
				return amt;
			}
			return 0.f;
		}
		//if it's getting power from another entity we do the check on that entity
		if (isPowered && !generator && receivingPowerFrom.is_alive()) {
			if (!receivingPowerFrom.has<PowerComponent>()) return 0.f; //what the fuck?
			return receivingPowerFrom.get_mut<PowerComponent>()->getPower(amt);
		}
		return 0.f; //dunno why we'd be here
	}
	//Checks to see if this component can pull the requested amount of power.
	const bool hasPower(const f32& amt) const {
		if (disabled) return false;
		if (!isPowered) return false;
		if (generator) {
			if (power >= amt) {
				return true;
			}
			return false;
		}
		if (isPowered && !generator && receivingPowerFrom.is_alive()) {
			if (!receivingPowerFrom.has<PowerComponent>()) return false;
			return receivingPowerFrom.get<PowerComponent>()->hasPower(amt);
		}
		return false; //dunno why we'd be here

	}
};

//FREE?????????????
const PowerComponent FREE_POWER_COMPONENT =
{
	false, true, true, 10000.f, 10000.f, 10000.f
};

//for things like turrets
const PowerComponent OBSTACLE_POWER_COMPONENT =
{
	false, true, true, 1000.f, 1000.f, 10.f
};

#endif 