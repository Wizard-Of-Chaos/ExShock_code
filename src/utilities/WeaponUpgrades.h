#pragma once
#ifndef WEAPONUPGRADES_H
#define WEAPONUPGRADES_H
#include "BaseHeader.h"

enum WEAPONUPGRADE_TYPE
{
	WUP_INVALID = -1,
	WUP_DAMAGE = 0,
	WUP_FIRERATE = 1,
	WUP_CLIPSIZE = 2,
	WUP_VELOCITY = 3,
	WUP_MAXAMMO = 4
};

const std::unordered_map<std::string, WEAPONUPGRADE_TYPE> weaponUpgradeStrings = {
	{"damage", WUP_DAMAGE},
	{"firerate", WUP_FIRERATE},
	{"clipsize", WUP_CLIPSIZE},
	{"velocity", WUP_VELOCITY},
	{"maxammo", WUP_MAXAMMO}
};

struct WeaponUpgradeData
{
	s32 id = -1;
	WEAPONUPGRADE_TYPE type;
	bool canBuild = false;
	bool permanent = false;
	bool requiresAmmo = false;
	u32 minSector = 0;
	std::string name = "Upgrade";
	std::string description = "This upgrade has not been set properly.";
	std::string upgradeDescription = "This description has not been set properly.";
	f32 baseCost = 0;
	f32 scaleCost = 0;
	f32 baseValue = 0;
	f32 scaleValue = 0;
	f32 maxValue = 0;
};

struct WeaponInstance;

class WeaponUpgradeInstance
{
	public:
		instId id = -1;
		s32 dataId = -1;
		f32 value = 0;
		virtual void upgrade(flecs::entity inst) = 0;
		instId usedBy = -1;
};

class WUIDamage : public WeaponUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};
class WUIFirerate : public WeaponUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};
class WUIClipSize : public WeaponUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};
class WUIVelocity : public WeaponUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};
class WUIMaxAmmo : public WeaponUpgradeInstance
{
	public:
		virtual void upgrade(flecs::entity inst);
};


#endif 