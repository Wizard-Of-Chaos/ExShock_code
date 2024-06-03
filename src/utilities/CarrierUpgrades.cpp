#include "CarrierUpgrades.h"
#include "Campaign.h"
#include <iostream>

const std::unordered_map<std::string, carrierUpgradeCallback> carrUpgradeCallbacks =
{
	{"Improved Turret Weaponry", carrUpgradeTurretGuns},
	{"Alien Ablative Armor", carrUpgradeAblativeArmor},
	{"Upgraded Turret Schematics", carrUpgradeTurrets},
	{"Ship Attitude Thrusters", carrUpgradeTurns},
	{"Shield Generator", carrUpgradeShield},
	{"Improved Armor", carrUpgradeHP},
	{"Auxiliary Engine Thrusters", carrUpgradeSpeed}
};


void carrUpgradeAblativeArmor(const CarrierUpgrade& up)
{
	ChaosTheoryStats& stats = campaign->getCarrierStats();
	stats.hp.health = shipData[CHAOS_THEORY_ID]->hp.maxHealth + ((f32)up.tier * 30.f);
	stats.hp.maxHealth = shipData[CHAOS_THEORY_ID]->hp.maxHealth + ((f32)up.tier * 30.f);
}

void carrUpgradeTurrets(const CarrierUpgrade& up)
{
	ChaosTheoryStats& stats = campaign->getCarrierStats();
	stats.turretId = up.tier;
	if (stats.turretId > 1) stats.turretId = 1; //only a few types of turret right now...
}
void carrUpgradeTurretGuns(const CarrierUpgrade& up)
{
	ChaosTheoryStats& stats = campaign->getCarrierStats();
	switch (up.tier) {
	case 1:
		stats.turretWepId = 11;
		break;
	case 2:
		stats.turretWepId = 12;
		break;
	case 3:
		stats.turretWepId = 13;
		break;
	case 4:
		stats.turretWepId = 14;
		break;
	default:
		stats.turretWepId = 3;
		break;
	}

}
void carrUpgradeShield(const CarrierUpgrade& up)
{
	ChaosTheoryStats& stats = campaign->getCarrierStats();
	stats.hp.maxShields = 120.f * (f32)up.tier;
	stats.hp.shields = 120.f * (f32)up.tier;

}
void carrUpgradeHP(const CarrierUpgrade& up)
{
	ChaosTheoryStats& stats = campaign->getCarrierStats();
	//upgrade resistances here
}
void carrUpgradeSpeed(const CarrierUpgrade& up)
{
	ChaosTheoryStats& stats = campaign->getCarrierStats();
	stats.thrst.linearMaxVelocity = shipData[CHAOS_THEORY_ID]->thrust.linearMaxVelocity + (30.f * (f32)up.tier);
	stats.thrst.forward = shipData[0]->thrust.forward + (150.f * (f32)up.tier);
}
void carrUpgradeTurns(const CarrierUpgrade& up)
{
	ChaosTheoryStats& stats = campaign->getCarrierStats();
	stats.thrst.pitch = shipData[CHAOS_THEORY_ID]->thrust.pitch + (100.f * (f32)up.tier);
	stats.thrst.yaw = shipData[CHAOS_THEORY_ID]->thrust.yaw + (100.f * (f32)up.tier);

}