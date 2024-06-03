#include "ShipUpgrades.h"
#include "Campaign.h"
#include "WeaponUtils.h"
#include "GameController.h"

void SUIShield::upgrade(flecs::entity inst)
{
	auto shld = inst.get_mut<HealthComponent>();
	shld->maxShields += value;
	shld->shields += value;
}

void SUIHealth::upgrade(flecs::entity inst)
{

}

void SUIAccel::upgrade(flecs::entity inst)
{
	auto thrst = inst.get_mut<ThrustComponent>();
	thrst->forward += value;
	thrst->linearMaxVelocity += value;
}

void SUIDecel::upgrade(flecs::entity inst)
{
	auto thrst = inst.get_mut<ThrustComponent>();
	thrst->brake += value;
}

void SUITurn::upgrade(flecs::entity inst)
{
	auto thrst = inst.get_mut<ThrustComponent>();
	thrst->pitch += value;
	thrst->yaw += value;
}

void SUIBoost::upgrade(flecs::entity inst)
{
	auto thrst = inst.get_mut<ThrustComponent>();
	thrst->boost += value;
}

void SUIFinale::upgrade(flecs::entity inst)
{
	auto thrst = inst.get_mut<ThrustComponent>();
	thrst->pitch *= 2.f;
	thrst->yaw *= 2.f;
	thrst->roll *= 2.f;
	thrst->forward *= 2.f;
	thrst->brake *= 2.5f;
	thrst->strafe *= 2.5f;
	thrst->boost *= 2.5f;

	auto shld = inst.get_mut<HealthComponent>();
	shld->maxShields *= 2.f;
	shld->shields = shld->maxShields;
	shld->rechargeRate *= 2.f;
	shld->rechargeDelay /= 1.5f;

	auto power = inst.get_mut<PowerComponent>();
	power->powerRegen *= 2.5f;
	power->maxPower *= 2.f;
	power->power = power->maxPower;

	auto hards = inst.get_mut<HardpointComponent>();
	for (u32 i = 0; i < hards->hardpointCount; ++i) {
		if (!hards->weapons[i].is_alive()) continue;
		auto wepInfo = hards->weapons[i].get_mut<WeaponInfoComponent>();
		auto wepFire = hards->weapons[i].get_mut<WeaponFiringComponent>();
		wepInfo->accuracy *= 2.f;
		wepInfo->damage *= 2.f;
		wepInfo->hitEffects.push_back(wepHit_bfgExplosion);

		if (wepInfo->usesPower) wepInfo->powerCost /= 2.f;
		if (wepInfo->usesAmmunition) {
			wepInfo->maxAmmunition *= 2;
			wepInfo->maxClip *= 2;
			wepFire->ammunition *= 2;
			wepFire->clip = wepInfo->maxClip;
			wepInfo->reloadTime /= 2.f;
		}
	}
}