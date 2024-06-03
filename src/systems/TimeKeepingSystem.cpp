#include "TimeKeepingSystem.h"
#include "BulletRigidBodyComponent.h"


void timeKeepingSystem(flecs::iter it, BulletRigidBodyComponent* rbcs)
{
	for (auto i : it)
	{
		auto rbc = &rbcs[i];
		auto ent = it.entity(i);
		rbc->timeAlive += it.delta_time();
		if (rbc->timeAlive >= .35f && ent.has<SpawnInvulnerable>()) {
			ent.remove<SpawnInvulnerable>();
		}
	}
}