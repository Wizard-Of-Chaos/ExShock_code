#pragma once

#ifndef HUDHAZARD_H
#define HUDHAZARD_H
#include "BaseHeader.h"
#include "HUDElement.h"

enum class HAZARD
{
	SHIELD_DRAIN,
	SLOWDOWN,
	GRAVITY_ANOMALY,
	INCOMING_MISSILE,
	RADIATION,
	SPEED_BOOST,
	CAPTURING_STATION,
	NONE
};

const extern std::unordered_map<HAZARD, std::string> hazNames;

class HUDHazard : public HUDElement
{
	public:
		HUDHazard(IGUIElement* root, flecs::entity cause, HAZARD which, u32 hazCount=0);
		virtual ~HUDHazard();
		virtual void updateElement(flecs::entity playerId);
		flecs::entity cause = INVALID_ENTITY;
		HAZARD which;
		const bool valid() const;
		//Override to say that a HUD element is no longer valid
		void setInvalid(bool isInvalid = true) { invalid = isInvalid; }
		void setTimeout(bool timeout = true, f32 time = .25f) { this->timeout = timeout; this->timeoutTime = time; this->timeoutTimer = 0.f; }
		f32 flashTimer = 0.f;
		f32 timeoutTimer = 0.f;
	private:
		IGUIImage* img = nullptr;
		IGUIStaticText* txt = nullptr;
		const f32 flashTime = .5f;
		bool invalid = false;
		bool timeout = false;
		f32 timeoutTime = .25f;
};

#endif 