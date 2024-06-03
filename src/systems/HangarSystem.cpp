#include "HangarSystem.h"
#include "LargeShipUtils.h"
#include "CrashLogger.h"
#include "IrrlichtComponent.h"
#include "IrrlichtUtils.h"
#include "ShipUtils.h"
#include "GameController.h"

void hangarSystem(flecs::iter it, HangarComponent* carr, IrrlichtComponent* irr, FactionComponent* fac)
{
	baedsLogger::logSystem("Hangar");

	for (auto i : it) {
		auto hangar = &carr[i];
		auto irrCmp = &irr[i];
		auto facCmp = &fac[i];
		try {
			if (!it.entity(i).is_alive()) game_throw("Hangar entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		hangar->curLaunchTimer += it.delta_system_time();
		if (hangar->curLaunchTimer >= hangar->launchRate && hangar->reserveShips > 0) {
			vector3df spawnPos = irrCmp->node->getAbsolutePosition() + getNodeDown(irrCmp->node) * 20.f * irr->node->getScale().Y;
			vector3df spawnRot = irrCmp->node->getRotation();
			gameController->triggerShipSpawn([hangar, spawnPos, spawnRot, facCmp]() {
				loadShipFromArchetype(hangar->spawnShipArchetypes[0], spawnPos, spawnRot, facCmp->type, true);
				});
			--hangar->reserveShips;

			hangar->curLaunchTimer = 0.f;
		}
	}
}