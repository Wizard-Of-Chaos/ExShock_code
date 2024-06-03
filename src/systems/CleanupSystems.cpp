#include "CleanupSystems.h"
#include "CrashLogger.h"
#include "PowerComponent.h"
#include "StationModuleComponent.h"
#include <iostream>

void powerSystem(flecs::iter it, PowerComponent* pwrc)
{
	baedsLogger::logSystem("Power");

	for (auto i : it) {
		try {
			if (!it.entity(i).is_alive()) game_throw("Power entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		auto power = &pwrc[i];
		//if it's not actively powered, we don't care.
		if (!power->isPowered) continue;
		//if it's receiving power from someone else we need to check it and de-power the module.
		if (!power->generator && !power->receivingPowerFrom.is_alive()) {
			power->isPowered = false;
			power->receivingPowerFrom = INVALID_ENTITY;
		}
		//restores energy on bar (assuming the generator is active)
		if (power->generator && !power->disabled) {
			power->power += power->powerRegen * it.delta_time();
			if (power->power >= power->maxPower) power->power = power->maxPower;
		}
	}
}

void stationModuleSystem(flecs::iter it, StationModuleComponent* smod)
{
	baedsLogger::logSystem("Station Module");

	for (auto i : it) {
		auto stationModule = &smod[i];
		auto ent = it.entity(i);
		if (!ent.is_alive()) continue;
		//NOTE FOR FUTURE ME: AS IT STANDS THE STATION MODULE SYSTEM DOES JACKSHIT AND IS NOT LOADED INTO GAMECONTROLLER
#ifdef _DEBUG
		//std::cout << vecStr(ent.get<IrrlichtComponent>()->node->getRotation()) << std::endl;
#endif
	}
}