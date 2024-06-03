#include "ObstacleAI.h"
#include "ObstacleComponent.h"
#include "SensorComponent.h"
#include "AIComponent.h"
#include "HealthComponent.h"
#include "GameAssets.h"
#include "AudioDriver.h"
#include "ShipMovementUtils.h"
#include "ThrustComponent.h"
#include "GameFunctions.h"
#include "IrrlichtComponent.h"
#include "GameController.h"
#include "CrashLogger.h"

void MineAI::stateCheck(aiSystemInfo& inf)
{
	inf.ai->state = AI_STATE_PURSUIT;
}
void MineAI::pursue(aiSystemInfo& inf)
{
	for (auto contact : inf.sns->contacts) {
		flecs::entity id = contact.ent;
		try {
			if (!id.is_alive()) game_throw("Mine AI pursuing invalid target: " + entDebugStr(id) + " being pursued by " + entDebugStr(self) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		if (!id.has<ShipComponent>()) continue;

		auto targRBC = id.get<BulletRigidBodyComponent>();
		//btVector3 dist = targRBC->rigidBody->getCenterOfMassPosition() - inf.rbc->rigidBody->getCenterOfMassPosition();
		if (triggeredBy != FACTION_NEUTRAL) {
			if (id.has<FactionComponent>()) {
				if (id.get<FactionComponent>()->type != triggeredBy) {
					continue;
				}
			}
		}
		destroyObject(self); // this had a health call but it really didn't need one
	}
}

void MissileAI::stateCheck(aiSystemInfo& inf)
{
	if (inf.ai->state == AI_STATE_PURSUIT) return; //we found a mans

	for (auto contact : inf.sns->contacts) {
		flecs::entity id = contact.ent;
		try {
			if (!id.is_alive()) game_throw("Missile AI pursuing invalid target: " + entDebugStr(id) + " being pursued by " + entDebugStr(self) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		if (!id.has<ShipComponent>()) continue;

		auto targRBC = id.get<BulletRigidBodyComponent>();
		auto selfRBC = self.get<BulletRigidBodyComponent>();
		btVector3 dist = targRBC->rigidBody->getCenterOfMassPosition() - selfRBC->rigidBody->getCenterOfMassPosition();
		if (dist.length() <= 350.f) {
			inf.ai->state = AI_STATE_PURSUIT;
			audioDriver->playGameSound(self, "hazard_warhead_activate.ogg", 1.f, 50.f, 800.f);
			if(id == gameController->getPlayer()) gameController->addHazard(self, HAZARD::INCOMING_MISSILE);
			target = id;
			selfRBC->rigidBody->setActivationState(1);
			auto irr = self.get_mut<IrrlichtComponent>();

			SColor basic = SColor(255, 255, 255, 255);
			auto edgeColor = SColor(64, 180, 180, 180);
			auto tailColor = SColor(0,180,180,180);
			auto trail = addMotionTrailTimeBased(irr->node, 16, 750, true);
			trail->getMaterial(0).Lighting = false;
			trail->getMaterial(0).MaterialType = EMT_TRANSPARENT_ALPHA_CHANNEL;
			trail->setVertexColors(
				basic, //tip center
				edgeColor, //tip edge
				edgeColor, //end center
				tailColor); //end edge
			trail->setWidth(2.8f);
			trail->setShrinkDirection(false, true);

			return;
		}
	}
}

void MissileAI::pursue(aiSystemInfo& inf)
{
	timeChasing += inf.dt;
	if (timeChasing >= lifetime) {
		self.get_mut<HealthComponent>()->health = 0.f; //if it's chasing for longer than 15 seconds blow up
		return;
	}
	if (!target.is_alive()) {
		inf.thrst->moves[THRUST_FORWARD] = true;
		return;
	}

	if (!target.has<BulletRigidBodyComponent>()) return;
	auto targetRBC = target.get<BulletRigidBodyComponent>();

	btVector3 targetPos = targetRBC->rigidBody->getCenterOfMassPosition();
	btVector3 pos = inf.rbc->rigidBody->getCenterOfMassPosition();

	btVector3 dist = targetPos - pos;

	goToPointPilot(inf.rbc->rigidBody, inf.thrst, targetPos, 0.f, highAngle, lowAngle, false);

	if (dist.length() <= kablooeyDistance) {
		self.get_mut<HealthComponent>()->health = 0.f; //if it's close enough to the target blow up
	}
}