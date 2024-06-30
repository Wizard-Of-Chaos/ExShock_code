#include "WeaponUpgrades.h"
#include "Campaign.h"
#include "GameController.h"

void WUIDamage::upgrade(flecs::entity inst)
{
	auto wep = inst.get_mut<WeaponInfoComponent>();
	wep->multiplier += value;
}

void WUIFirerate::upgrade(flecs::entity inst)
{
	auto wep = inst.get_mut<WeaponInfoComponent>();
	wep->firingSpeed += (wep->firingSpeed * value);
}

void WUIClipSize::upgrade(flecs::entity inst)
{
	auto wep = inst.get_mut<WeaponInfoComponent>();
	auto fire = inst.get_mut<WeaponFiringComponent>();

	if (!wep->usesAmmunition) return;
	wep->maxClip += (u32)(wep->maxClip * value);
	fire->clip = wep->maxClip;
}

void WUIVelocity::upgrade(flecs::entity inst)
{
	auto wep = inst.get_mut<WeaponInfoComponent>();
	wep->projectileSpeed += (wep->projectileSpeed * value);
}

void WUIMaxAmmo::upgrade(flecs::entity inst)
{
	auto wep = inst.get_mut<WeaponInfoComponent>();
	auto fire = inst.get_mut<WeaponFiringComponent>();

	if (!wep->usesAmmunition) return;
	u32 val = (u32)(wep->maxAmmunition * value);
	wep->maxAmmunition += val;
	fire->ammunition += val;
}