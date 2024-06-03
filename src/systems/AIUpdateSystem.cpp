#include "AIUpdateSystem.h"
#include "AIComponent.h"
#include "SensorComponent.h"
#include "PlayerComponent.h"
#include "ThrustComponent.h"
#include "HealthComponent.h"
#include "HardpointComponent.h"
#include "IrrlichtComponent.h"
#include "AITypes.h"
#include "GameController.h"
#include "AudioDriver.h"
#include "CrashLogger.h"

static bool handleOrder(aiSystemInfo inf)
{
	Order& ord = inf.ai->mostRecentOrder;
	if (!ord.from.is_alive()) return false;
	switch (ord.type) {
	case ORDER_TYPE::ORD_ATTACK: {
		if (!ord.target.is_alive()) return false;
		if (ord.target.has<FactionComponent>()) {
			if (ord.target.get<FactionComponent>()->isFriendly(inf.ent.get<FactionComponent>())) return false;
		}
		inf.sns->targetContact = ord.target;
		inf.ai->state = AI_STATE_PURSUIT;
		return true;
	}
	case ORDER_TYPE::ORD_FORMUP: {
		if (!inf.ai->wingCommander.is_alive()) return true; //if the wing commander is dead whatever, nothing to form up with
		inf.ai->state = AI_STATE_IDLE;
		return true;
	}
	case ORDER_TYPE::ORD_HALT: {
		inf.ai->state = AI_STATE_HALT;
		return true;
	}
	case ORDER_TYPE::ORD_DOCK: {
		if (!inf.ai->hasBehavior(AIBHVR_CAN_DOCK)) return false;
		if (!ord.target.is_alive()) return false;
		inf.sns->targetContact = ord.target;
		inf.ai->state = AI_STATE_DOCKING;
		return true;
	}
	case ORDER_TYPE::ORD_HONK_HELP: {
		if (!inf.ai->wingCommander.is_alive()) return false;
		if (!ord.target.is_alive()) return false;
		if (ord.target.has<FactionComponent>()) {
			if (ord.target.get<FactionComponent>()->isFriendly(inf.ent.get<FactionComponent>())) return false;
		}
		inf.sns->targetContact = ord.target;
		inf.ai->state = AI_STATE_PURSUIT;
		return true;
	}
	case ORDER_TYPE::ORD_DISENGAGE: {
		if (!inf.ai->wingCommander.is_alive()) return false;
		if (inf.ai->wingCommander == gameController->getPlayer()) {
			inf.ai->timeInCurrentState = 0.f;
			inf.ai->timeSinceLastStateCheck = 0.f;
			inf.ai->reactionSpeed = 4000.f;
			inf.ai->state = AI_STATE_PATROL;
			inf.ai->onPatrol = true;
			inf.ai->fixedPatrolRoute = true;
			inf.ai->movingToPoint = 0;
			inf.ai->route.push_back(gameController->playerStart());
		}
		inf.ai->onWing = false;
		inf.ai->wingCommander = INVALID_ENTITY;
		inf.ai->unitsOnWing.clear();
		return true;
	}
	default:
		break;
	}

	return false;
}

void AIUpdateSystem(flecs::iter it, 
	AIComponent* aic, IrrlichtComponent* irrc, BulletRigidBodyComponent* rbcs, ThrustComponent* thrc, HardpointComponent* hardsc, SensorComponent* sensc, HealthComponent* hpc)
{
	baedsLogger::logSystem("AI");
	game_world->defer_suspend();
	for (auto i : it) {
		auto ai = &aic[i];
		auto hards = &hardsc[i];
		auto rbc = &rbcs[i];
		auto irr = &irrc[i];
		auto sensors = &sensc[i];
		auto hp = &hpc[i];
		auto thrust = &thrc[i];
		auto ent = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("AI entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		if (rbc->timeAlive <= 1.f) {
			continue;
		}
		f32 dt = it.delta_time();
		aiSystemInfo inf = {ent, ai, thrust, hards, rbc, irr, sensors, hp, dt };
		
		if (!ai->orderHandled) {
			if (handleOrder(inf)) {
				ai->orderOverride = true;
				ai->orderHandled = true;
				if (ai->mostRecentOrder.from == gameController->getPlayer()) {
					audioDriver->playMenuSound("order_confirm_ship.ogg");
					auto player = gameController->getPlayer().get_mut<PlayerComponent>();
					player->timeSinceLastOrder = 0;
					gameController->regPopup(ai->AIName, ai->orderLines[ai->mostRecentOrder.type]);
				}
				//popups?
			}
			else {
				ai->orderOverride = false;
				ai->orderHandled = true;
				if (ai->mostRecentOrder.from == gameController->getPlayer()) {
					audioDriver->playMenuSound("order_negative_ship.ogg");
					auto player = gameController->getPlayer().get_mut<PlayerComponent>();
					player->timeSinceLastOrder = 0;
					gameController->regPopup(ai->AIName, ai->negLine);
				}
			}
		}

		ai->timeSinceLastStateCheck += dt;
		if (AIBHVR_AVOIDS_OBSTACLES) ai->obstacleAvoidTimer += dt;
		if (!ai->aiControls) continue;

		if (ai->timeSinceLastStateCheck >= ai->reactionSpeed) {
			ai->aiControls->stateCheck(inf);
			ai->timeSinceLastStateCheck = 0;
		}
		//if (!ai->aiControls) continue;

		switch (ai->state) {
			case AI_STATE_IDLE:
				ai->aiControls->idle(inf);
				break;
			case AI_STATE_FLEE:
				ai->aiControls->flee(inf);
				break;
			case AI_STATE_PURSUIT:
				ai->aiControls->pursue(inf);
				break;
			case AI_STATE_PATROL:
				ai->aiControls->patrol(inf);
				break;
			case AI_STATE_HALT:
				ai->aiControls->halt(inf);
				break;
			case AI_STATE_DOCKING:
				ai->aiControls->docking(inf);
				break;
			case AI_STATE_DOCKED:
				ai->aiControls->docked(inf);
				break;
			case AI_STATE_SHATTERED:
				ai->aiControls->shatter(inf);
			default:
				break;
		}
	}
	game_world->defer_resume();
}
