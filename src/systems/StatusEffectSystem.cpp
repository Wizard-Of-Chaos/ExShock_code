#include "StatusEffectSystem.h"
#include "StatusEffectComponent.h"
#include "CrashLogger.h"

void statusEffectSystem(flecs::iter it, StatusEffectComponent* efc)
{
	baedsLogger::logSystem("Status Effects");
	game_world->defer_suspend();

	for (auto i : it) {
		auto& stat = efc[i];
		auto ent = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("Status effect entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		if (stat.effects.empty()) continue;
		auto lst = stat.effects.begin();
		while (lst != stat.effects.end()) {
			if ((*lst)->apply(it.delta_time(), ent)) lst = stat.effects.erase(lst);
			if (lst == stat.effects.end()) break;
			++lst;
		}
	}
	game_world->defer_resume();

}