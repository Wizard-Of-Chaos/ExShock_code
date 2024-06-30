#include "HealthAndShieldSystems.h"

#include "GameStateController.h"
#include "GameController.h"
#include "PlayerComponent.h"
#include "GameFunctions.h"
#include "HealthComponent.h"
#include "CrashLogger.h"

void healthSystem(flecs::iter it, HealthComponent* hc)
{
	baedsLogger::logSystem("Health");
	for (auto i : it)
	{
		auto hp = &hc[i];
		auto e = it.entity(i);
		try {
			if (!e.is_alive()) game_throw("Health entity is NOT ALIVE " + entDebugStr(e) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		if (hp->health <= 0)
		{
			if (e.has<PlayerComponent>())
			{
				gameController->isPlayerAlive = false;
				//stateController->setState(GAME_FINISHED);
			}
			if (gameController->isNetworked()) {
				if (clientOwnsThisEntity(e))
					destroyObject(e);
				else
					//we don't own this entity and we cannot kill it.
					hp->health = .01f;
			}
			else
				destroyObject(e);
		}
		hp->timeSinceLastHit += it.delta_time();
		if (hp->healthRegen > 0.f && hp->health < hp->maxHealth) {
			hp->health += hp->healthRegen * it.delta_time();
		}
		if (hp->maxShields == 0.f) continue;
		if (hp->timeSinceLastHit >= hp->rechargeDelay)
		{
			if (hp->shields < hp->maxShields)
			{
				hp->shields += hp->rechargeRate * it.delta_time();
				if (hp->shields > hp->maxShields)
				{
					hp->shields = hp->maxShields;
				}
			}
		}

		hp->wasHitLastFrame = false;
		if (hp->shieldsLastFrame > hp->shields) hp->wasHitLastFrame = true;
		hp->shieldsLastFrame = hp->shields;

	}
}