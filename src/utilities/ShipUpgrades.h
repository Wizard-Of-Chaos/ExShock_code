#pragma once
#ifndef SHIPUPGRADES_H
#define SHIPUPGRADES_H
#include "BaseHeader.h"

enum SHIPUPGRADE_TYPE
{
	SUP_INVALID = -1,
	SUP_HEALTH = 0,
	SUP_SHIELD = 1,
	SUP_ACCELERATION = 2,
	SUP_DECELERATION = 3,
	SUP_TURNING = 4,
	SUP_BOOST = 5,
	SUP_BFG = 6
};

const std::unordered_map<std::string, SHIPUPGRADE_TYPE> shipUpgradeStrings = {
	{"health", SUP_HEALTH },
	{"shield", SUP_SHIELD},
	{"acceleration", SUP_ACCELERATION },
	{"deceleration", SUP_DECELERATION },
	{"turning", SUP_TURNING },
	{"boost", SUP_BOOST },
	{"finale", SUP_BFG }
};

struct ShipUpgradeData
{
	s32 id = -1;
	SHIPUPGRADE_TYPE type;
	bool canBuild = false;
	bool permanent = false;
	u32 minSector = 0;
	std::string name="Upgrade";
	std::string description="This upgrade has not been set properly.";
	std::string upgradeDescription="This description has not been set properly.";
	f32 baseCost = 0;
	f32 scaleCost = 0;
	f32 baseValue = 0;
	f32 scaleValue = 0;
	f32 maxValue = 0;
};

struct ShipInstance;

class ShipUpgradeInstance
{
	public:
		instId id = -1;
		s32 dataId = -1;
		f32 value = 0;
		virtual void upgrade(flecs::entity inst) = 0;
		instId usedBy = -1;
};

class SUIShield : public ShipUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};

class SUIHealth : public ShipUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};

class SUIAccel : public ShipUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};
class SUIDecel : public ShipUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};
class SUITurn : public ShipUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};
class SUIBoost : public ShipUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};

class SUIFinale : public ShipUpgradeInstance
{
public:
	virtual void upgrade(flecs::entity inst);
};

#endif