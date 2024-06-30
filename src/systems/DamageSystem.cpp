#include "DamageSystem.h"
#include "AudioDriver.h"
#include "GameController.h"
#include "GameStateController.h"
#include "StatTrackerComponent.h"
#include "PlayerComponent.h"
#include "HealthComponent.h"
#include "CrashLogger.h"
#include "GameFunctions.h"
#include "ShipComponent.h"
#include "AIComponent.h"
#include "IrrlichtComponent.h"
#include "Networking.h"
#include "NetworkingComponent.h"

static void _handleInstance(flecs::entity id, DamageInstance& inst, HealthComponent* hp, bool useShields=true)
{
	if (inst.amount <= HEALTH_RES) return;

	f32 overflow = inst.amount;
	bool shieldsDown = false;
	auto type = inst.type;

	if (useShields && hp->maxShields > 0.f) {
		if (hp->shields >= 0) shieldsDown = true;
		hp->shields -= (inst.amount * (1.f - hp->shieldResistances[inst.type]));
		if (hp->shields <= 0) {
			if (!shieldsDown) audioDriver->playGameSound(id, "shield_down.ogg");
			overflow = -hp->shields;
			hp->shields = 0;
		}
		else {
			if (inst.amount > 10.f) audioDriver->playGameSound(id, "shield_impact_major.ogg", .6f);
			else if (inst.amount > 1.f) audioDriver->playGameSound(id, "shield_impact_minor.ogg", .6f);
			particleEffectBill("assets/effects/shieldhit/", inst.hitPos, .8f, 4.2f);
			overflow = 0;
		}
		hp->timeSinceLastHit = 0;
	}

	if (id == gameController->getPlayer()) {
		if (inst.from.is_alive()) {
			if (inst.from.has<IrrlichtComponent>()) {
				auto pos = inst.from.get<IrrlichtComponent>()->node->getAbsolutePosition();
				if (shieldsDown) gameController->smackHealth(pos);
				else gameController->smackShields(pos);
			}
		}
	}

	if (overflow == 0 || type == DAMAGE_TYPE::EMP) return;
	hp->health -= (inst.amount * (1.f - hp->healthResistances[inst.type]));
}

void damageSystem(flecs::iter it, HealthComponent* hc)
{
	baedsLogger::logSystem("Damage");
	game_world->defer_suspend();
	for (auto i : it) {
		auto hp = &hc[i];
		auto id = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("Damage entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		for (DamageInstance inst : hp->instances) {
			if (gameController->isNetworked() && !clientOwnsThisEntity(id))
				continue;

			switch (inst.type) {
			case DAMAGE_TYPE::NONE:
				break;
			case DAMAGE_TYPE::ENERGY:
			case DAMAGE_TYPE::EXPLOSIVE:
			case DAMAGE_TYPE::KINETIC:
			case DAMAGE_TYPE::EMP:
				_handleInstance(id, inst, hp);
				break;
			case DAMAGE_TYPE::IMPACT:
				if (inst.time >= hp->lastDamageTime + 500) audioDriver->playGameSound(id, "impact_1.ogg");
				_handleInstance(id, inst, hp, false);
				break;
			case DAMAGE_TYPE::VELOCITY:
				_handleInstance(id, inst, hp, false);
				break;
			default: //should be an error here
				baedsLogger::errLog("Unrecognized damage type on instance for entity " + entDebugStr(id) + "\n");
				break;
			}
			hp->lastDamageType = inst.type;
			hp->lastDamageTime = inst.time;

			if(inst.type != DAMAGE_TYPE::VELOCITY) hp->recentInstances.push_front(inst);
			if (hp->recentInstances.size() > MAX_TRACKED_DAMAGE_INSTANCES) hp->recentInstances.pop_back();

			//if we own the damage instance, but we do *not* own what it got applied to, we send that damage over the network.
			if (!gameController->isNetworked())
				continue;

			if (clientOwnsThisEntity(inst.from) && !clientOwnsThisEntity(inst.to)) {
				if (inst.from.is_alive() && inst.to.is_alive()) {
					NetworkId from = INVALID_NETWORK_ID; NetworkId to = INVALID_NETWORK_ID;
					if (inst.from.has<NetworkingComponent>())
						from = inst.from.get<NetworkingComponent>()->networkedId;
					if (inst.to.has<NetworkingComponent>())
						to = inst.to.get<NetworkingComponent>()->networkedId;

					//don't send data to dead entities
					if (to) {
						Net_DamageInstance netInst = Net_DamageInstance(from, to, inst.type, inst.amount, inst.time, inst.hitPos);
						gameController->sendDamageToNetwork(netInst);
					}
				}
			}

		}
		//checked all the instances, now we can adjust stats
		if (hp->health <= 0 && !hp->instances.empty()) {
			const DamageInstance last = hp->instances.back();
			if (last.from.is_alive()) {
				if (last.from.has<StatTrackerComponent>() && last.to.has<ShipComponent>()) {
					auto stats = last.from.get_mut<StatTrackerComponent>();
					++stats->kills;
					if (last.from.has<AIComponent>()) {
						auto ai = last.from.get<AIComponent>();
						gameController->regPopup(ai->AIName, ai->killLine);
					}
				}
			}
		}

		hp->instances.clear();
	}
	game_world->defer_resume();
}