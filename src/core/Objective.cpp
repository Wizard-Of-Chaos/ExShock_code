#include "Objective.h"
#include "GameController.h"
#include "GameFunctions.h"
#include "StatusEffects.h"
#include "ShipUtils.h"
#include "IrrlichtComponent.h"
#include "CrashLogger.h"

const std::unordered_map<OBJECTIVE_TYPE, std::string> objNameStrings = {
	{OBJ_COLLECT, "Retrieve"},
	{OBJ_DESTROY, "Destroy"},
	{OBJ_GO_TO, "Move To"},
	{OBJ_ESCORT, "Escort"},
	{OBJ_CAPTURE, "Capture"},
	{OBJ_PROTECT, "Protect"},
	{OBJ_TUTORIAL, "tutorial"}
};

const bool Objective::isObjOver(f32 dt) {
	this->dt = dt;
	currentTime += dt;
	if (m_timesUp()) {
		baedsLogger::log("Time's up on this objective. Finishing.\n");
		return true;
	}
	return objectiveUpdate();
}
void Objective::onComplete() {
	if (completionCb && !completeTriggered) {
		baedsLogger::log("Objective " + objNameStrings.at(m_type) + " completed!\n");
		completionCb();
		completeTriggered = true;
	}
}

const bool RemoveEntityObjective::objectiveUpdate()
{
	return success();
}

const bool RemoveEntityObjective::success()
{
	for (auto& ent : targets) {
		if (ent.is_alive()) return false;
	}
	return true;
}

const bool WaveDefenseObjective::objectiveUpdate()
{
	auto it = targets.begin();
	while (it != targets.end()) {
		if (!it->is_alive()) it = targets.erase(it);
		if (it != targets.end()) ++it;
	}

	if (targets.empty()) {
		if (!m_waves.empty()) {
			auto w = m_waves.front();
			m_waves.pop_front();
			auto playerNode = gameController->getPlayer().get<IrrlichtComponent>()->node;
			vector3df pos = playerNode->getPosition();
			vector3df forward = getNodeForward(playerNode);
			targets = spawnScenarioWing(pos +(forward*400.f), vector3df(0, 0, 0), w.shipId, w.wepId, w.aceShipId, w.aceWepId, w.turretId, w.turretWepId);
		}
	}
	return(success() && m_waves.empty());
}

const bool WaveDefenseObjective::success()
{
	for (auto& ent : targets) {
		if (ent.is_alive()) return false;
	}
	return true;
}

const bool GoToPointObjective::objectiveUpdate()
{
	auto player = gameController->getPlayer();
	vector3df playerpos;
	if (!player.is_alive()) return true; //what the fuck?
	playerpos = player.get<IrrlichtComponent>()->node->getPosition();

	//if we're using a point do this
	if (!m_usesEntity) {
		if ((m_position - playerpos).getLength() < m_dist) return true;

		return false;
	}
	if (targets.empty()) {
		return true;
	}
	auto& targ = targets.front();
	vector3df targpos;

	f32 dist = 0;

	if (!targ.is_alive()) {
		return true; //targ is dead; moving to dead target = fail
	}
	if (targ.has<IrrlichtComponent>()) {
		if (targ.get<IrrlichtComponent>()->node) {
			targpos = targ.get<IrrlichtComponent>()->node->getPosition();
			dist = (targpos - playerpos).getLength();
			if (dist < m_dist * (targ.get<IrrlichtComponent>()->node->getScale().X / 2.f)) goto continueObj;
			else return false;
		} else {
			goto continueObj; //no node
		}
	}
	else {
		goto continueObj; //no irr
	}
continueObj:
	if (!targets.empty()) {
		targets.pop_front();
		return false;
	}
	return true;
}

const bool GoToPointObjective::success()
{
	if (targets.empty()) return true;
	return false;
}
const bool EscortObjective::objectiveUpdate()
{
	vector3df escortpos;

	if (!m_escorted.is_alive()) {
		baedsLogger::log("Escorted entity is dead; finishing.\n");
		return true; //what the fuck?
	}
	escortpos = m_escorted.get<IrrlichtComponent>()->node->getPosition();

	//if we're using a point do this
	if (!m_usesEntity) {
		if ((m_position - escortpos).getLength() < m_dist) {
			baedsLogger::log("Escorted entity has closed to required distance of " + std::to_string(m_dist) + ".\n");
			return true;
		}
		return false;
	}
	if (targets.empty()) {
		baedsLogger::log("No more targets to escort to.\n");
		return true;
	}
	auto& targ = targets.front();
	vector3df targpos;

	f32 dist = 0;

	if (!targ.is_alive()) {
		baedsLogger::log("Escort target is dead; finishing.\n");
		return true; //targ is dead; moving to dead target = fail
	}
	if (targ.has<IrrlichtComponent>()) {
		if (targ.get<IrrlichtComponent>()->node) {
			targpos = targ.get<IrrlichtComponent>()->node->getPosition();
			dist = (targpos - escortpos).getLength();
			f32 closeDist = m_dist * (targ.get<IrrlichtComponent>()->node->getScale().X / 2.f);
			if (dist < closeDist) {
				baedsLogger::log("Target has closed to required distance of " + std::to_string(closeDist) + ".\n");
				goto continueObj;
			}
			else return false;
		}
		else {
			baedsLogger::log("Irrlicht node not available - ditching this target.\n");
			goto continueObj; //no node
		}
	}
	else {
		baedsLogger::log("Irrlicht component not available - ditching this target.\n");
		goto continueObj; //no irr
	}
continueObj:
	if (!targets.empty()) {
		targets.pop_front();
		return false;
	}
	baedsLogger::log("Escort successful! Finishing.\n");
	return true;
}

const bool EscortObjective::success()
{
	if (targets.empty()) return true;
	return false;

}

const bool DockObjective::objectiveUpdate()
{
	//return false;
	flecs::entity dockModule = targets.front();
	flecs::entity shipToDock = *(++targets.begin()); //the docking ship should be the second target
	if (!shipToDock.is_alive() || !dockModule.is_alive()) return true;
	auto ai = shipToDock.get<AIComponent>();
	if (ai->state == AI_STATE_DOCKED && appliesStickySpawner) {
		if (!isCurrentlyDocked) {
			StickySpawnerEffect* spawner = new StickySpawnerEffect;
			gameController->setSpawns(
				spawner->aceShipId,
				spawner->shipId,
				spawner->aceWepId,
				spawner->wepId,
				spawner->turretId,
				spawner->turretWepId
				);
			spawner->duration = timeToCapture;
			spawner->spawnInterval = enemySpawnRate;

			addStatusEffectToEntity(spawner, gameController->getPlayer());

			gameController->addLargePopup(dockStr, spkr);
			isCurrentlyDocked = true;
		}
		timeSpentCapturing += dt;
	}
	else if (isCurrentlyDocked) {
		isCurrentlyDocked = false;
		gameController->addLargePopup(breakStr, spkr);
	}
	if (timeSpentCapturing >= timeToCapture) {
		gameController->addLargePopup(completeStr, spkr);
		return true;
	}
	return false;
}
const bool DockObjective::success()
{
	if (timeSpentCapturing >= timeToCapture) return true;
	return false;
}

const bool ProtectObjective::objectiveUpdate()
{
	if (!success()) return true;
	return false;
}

const bool ProtectObjective::success()
{
	for (auto& ent : targets) {
		if (!ent.is_alive()) return false;
	}
	return true;
}

const bool KeyPressObjective::objectiveUpdate()
{
	auto player = gameController->getPlayer();
	auto in = player.get<InputComponent>();
	for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
		if (!inputsRequired[i])
			continue;
		if (in->isKeyDown((INPUT)i))
			inputsTriggered[i] = true; //trigger it but don't revert it
	}
	return success();
}

const bool KeyPressObjective::success()
{
	for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
		if (inputsRequired[i] && !inputsTriggered[i])
			return false;
	}
	return true;
}