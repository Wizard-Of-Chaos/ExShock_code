#include "ProjectileSystem.h"
#include "IrrlichtUtils.h"
#include "BulletRigidBodyComponent.h"
#include "WeaponInfoComponent.h"
#include "CrashLogger.h"
#include "IrrlichtComponent.h"
#include "GameFunctions.h"
#include "GameController.h"

void projectileSystem(flecs::iter it, BulletRigidBodyComponent* rbcs, ProjectileInfoComponent* pic, IrrlichtComponent* irrs)
{
	baedsLogger::logSystem("Projectile Tracking");

	for (auto i : it) {
		auto proj = &pic[i];
		auto irr = &irrs[i];
		auto rbc = &rbcs[i];
		auto ent = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("Projectile entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		proj->currentLifetime += it.delta_time();

		//note: incredibly stupid hack because there's sometimes a weird error where projectiles aren't facing the correct direction on the frame they're spawned
		//projectiles spawn as invisible and then get set visible on the first frame after they've had their collision body adjusted
		if(proj->currentLifetime > gameController->getDt()*2.f)
			irr->node->setVisible(true);

		//TODO: make this a fucking animator for gods sake
		f32 percentTotalLifetime = (proj->currentLifetime / proj->lifetime);
		f32 percentToFullLength = percentTotalLifetime / .08f; //projectile reaches max length at 8% of its total lifetime
		vector3df scale = irr->node->getScale();
		scale.Y = std::max(1.f, std::min(proj->length, (proj->length * percentToFullLength)));
		if(proj->type != WEP_ROCK && proj->type != WEP_HEAVY_MISSILE) irr->node->setScale(scale);
		if (proj->currentLifetime >= proj->lifetime) {
			destroyObject(ent);
		}
	}
}