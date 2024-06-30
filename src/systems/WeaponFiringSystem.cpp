#include "WeaponFiringSystem.h"
#include "AudioDriver.h"
#include "PowerComponent.h"
#include "IrrlichtComponent.h"
#include "IrrlichtUtils.h"
#include "WeaponUtils.h"
#include "GameController.h"
#include "CrashLogger.h"

static bool _canFire(const WeaponInfoComponent* wep, const WeaponFiringComponent* fire, const PowerComponent* power)
{
	if (power->disabled) return false;
	if (fire->timeSinceLastShot <= wep->firingSpeed) return false;
	return true;
}

static bool _hasResourcesToFire(const WeaponInfoComponent* wep, const WeaponFiringComponent* fire, const PowerComponent* power)
{
	if (wep->usesAmmunition && fire->clip <= 0) return false;
	if (wep->usesPower && !power->hasPower(wep->powerCost)) return false;
	return true;
}

static void _fireCost(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power)
{
	if (wep->usesAmmunition) {
		fire->clip -= 1;
		fire->timeReloading = 0.f;
	}
	if (wep->usesPower) {
		power->getPower(wep->powerCost);
	}
	//wep->timeSinceLastShot = 0.f;
}

vector3df _wepProjSpawnPos(const WeaponInfoComponent* wep, const IrrlichtComponent* irr)
{
	vector3df pos = wep->barrelStart + irr->node->getAbsolutePosition();
	pos -= irr->node->getAbsolutePosition();
	matrix4 rotMat;
	rotMat.setRotationDegrees(irr->node->getAbsoluteTransformation().getRotationDegrees());
	rotMat.rotateVect(pos);
	pos += irr->node->getAbsolutePosition();
	pos += getNodeForward(irr->node) * 4.f;
	return pos;
}

void weaponFiringSystem(flecs::iter it, WeaponInfoComponent* wic, WeaponFiringComponent* fireC, IrrlichtComponent* irrC, PowerComponent* pwrc)
{
	baedsLogger::logSystem("Weapon Firing");
	for (auto i : it) 
	{
		auto wepInfo = &wic[i];
		if (wepInfo->type == WEP_NONE) continue;
		auto entityId = it.entity(i);

		try {
			if (!it.entity(i).is_alive()) game_throw("Weapon entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		auto irrComp = &irrC[i];
		auto power = &pwrc[i];
		auto fire = &fireC[i];

		fire->spawnPosition = _wepProjSpawnPos(wepInfo, irrComp);
		for (auto& update : wepInfo->updates) {
			update(wepInfo, fire, power, entityId, it.delta_system_time());
		}

		bool gunFired = false; //gunfired reborn!
		if (fire->isFiring)
		{
			if (_canFire(wepInfo, fire, power)) {
				fire->timeSinceLastShot = 0.f;
				if (_hasResourcesToFire(wepInfo, fire, power)) {
					_fireCost(wepInfo, fire, power);
					gunFired = true; //wepInfo->fire(wepInfo, fire, power, entityId, it.delta_system_time());
					gameController->setFire(entityId);
				}
				else {
					audioDriver->playGameSound(entityId, "weapon_dry.ogg", .4f);
				}
			}
		}
		if(gunFired) 
			audioDriver->playGameSound(entityId, wepInfo->fireSound);

		if (fire->timeSinceLastShot > wepInfo->firingSpeed)
		{
			// what is this?
			//manual check to keep it from spiralling off into infinity
			fire->timeSinceLastShot = wepInfo->firingSpeed + .005f;
		}

		fire->flashTimer += it.delta_time();
		if (gunFired) {
			fire->flashTimer = 0;
			fire->muzzleFlash->setVisible(true);
			fire->muzzleFlash->setScale(vector3df(1, 1, 3));
		}
		else {
			f32 percent = 1.f - (fire->flashTimer / .25f);
			if (percent < .0001f) {
				fire->muzzleFlash->setVisible(false);
				continue;
			}
			f32 xyscale = percent * 1.f;
			f32 zscale = percent * 3.f;
			fire->muzzleFlash->setScale(vector3df(xyscale, xyscale, zscale));
			fire->muzzleFlashLight->setRadius(400.f * percent);
		}
	}
}
