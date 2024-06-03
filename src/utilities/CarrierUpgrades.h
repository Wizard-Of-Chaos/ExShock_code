#pragma once
#ifndef CARRIERUPGRADES_H
#define CARRIERUPGRADES_H
#include <functional>
#include <unordered_map>
#include "BaseHeader.h"

struct CarrierUpgrade;

typedef std::function<void(CarrierUpgrade&)> carrierUpgradeCallback;

struct CarrierUpgrade
{
	std::string name;
	std::string desc;
	u32 tier=0;
	u32 maxtier=4;
	f32 cost=80;
	bool canBuild = false;
	carrierUpgradeCallback upgrade;
};

const extern std::unordered_map<std::string, carrierUpgradeCallback> carrUpgradeCallbacks;

void carrUpgradeAblativeArmor(const CarrierUpgrade& up);
void carrUpgradeTurrets(const CarrierUpgrade& up);
void carrUpgradeTurretGuns(const CarrierUpgrade& up);
void carrUpgradeShield(const CarrierUpgrade& up);
void carrUpgradeHP(const CarrierUpgrade& up);
void carrUpgradeSpeed(const CarrierUpgrade& up);
void carrUpgradeTurns(const CarrierUpgrade& up);

#endif 