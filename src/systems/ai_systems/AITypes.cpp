#include "AITypes.h"
#include "AIComponent.h"
#include "WeaponInfoComponent.h"
#include "BulletRigidBodyComponent.h"
#include "SensorComponent.h"
#include "IrrlichtComponent.h"
#include "ThrustComponent.h"
#include "IrrlichtUtils.h"
#include "HardpointComponent.h"
#include "ShipMovementUtils.h"
#include "WeaponUtils.h"
#include "AudioDriver.h"
#include "GameController.h"
#include "StationModuleComponent.h"
#include "PlayerComponent.h"
#include "GameFunctions.h"
#include "CrashLogger.h"

void AIType::m_setAIWeapon(flecs::entity wep, bool firing)
{
	if (!wep.is_alive()) return;
	if (!wep.has<WeaponInfoComponent>()) return;
	if (wep.get<WeaponFiringComponent>()->isFiring == firing) return;
	auto wepFire = wep.get_mut<WeaponFiringComponent>();
	wepFire->isFiring = firing;
}

void AIType::m_toggleAllWeps(HardpointComponent* hards, bool firing)
{
	for (unsigned int i = 0; i < hards->hardpointCount; ++i) {
		m_setAIWeapon(hards->weapons[i], false);
	}
}

void AIType::m_fireAtTarget(const BulletRigidBodyComponent* rbc, const BulletRigidBodyComponent* targRBC, const AIComponent* ai, HardpointComponent* hards)
{
	if (!targRBC || !rbc) return;
	flecs::entity firstGun = hards->weapons[0];

	const WeaponInfoComponent* firstWepInfo = nullptr;
	if (firstGun.is_alive()) {
		if (firstGun.has<WeaponInfoComponent>() && firstGun.has<IrrlichtComponent>()) {
			firstWepInfo = firstGun.get<WeaponInfoComponent>();
			auto firing = firstGun.get<WeaponFiringComponent>();
		}
	}

	btVector3 selfPos = rbc->rigidBody->getCenterOfMassPosition();
	btVector3 targetPosition = targRBC->rigidBody->getCenterOfMassPosition();
	btVector3 facing = targetPosition - selfPos;
	f32 dist = facing.length();
	facing = facing.normalize();
	btVector3 forward = getRigidBodyForward(rbc->rigidBody);
	btScalar angle = forward.angle(facing);

	//now we mess with their targeting data
	f32 distAimBonus = std::clamp(dist / 1000.f, 0.f, 1.f);
	distAimBonus = (1.f - distAimBonus) * 30.f;
	f32 variance = (200.f / ai->aim) / std::max(distAimBonus, 1.f);
	//in this case our variance is how crappy their aim is versus how close they are
	//aim bonus conferred based on distance
	vector3df rand = randomPositionalVector(variance);
	targetPosition += irrVecToBt(rand);

	btVector3 aheadTarg = targRBC->rigidBody->getCenterOfMassPosition();
	f32 travelTime = .4f;
	if (firstWepInfo) {
		travelTime = (aheadTarg - selfPos).length() / (firstWepInfo->projectileSpeed / .1f);
		firingDistance = (firstWepInfo->projectileSpeed / .1f);

		if (travelTime > firstWepInfo->lifetime) {
			m_toggleAllWeps(hards, false);
			return;
		}
	}

	//if it's facing the ship, start shooting
	if ((angle * RADTODEG) < 30.f) { //converted to degrees so my overworked meat brain can better comprehend it
		for (unsigned int i = 0; i < hards->hardpointCount; ++i) {
			flecs::entity wep = hards->weapons[i];
			if (!wep.is_alive()) continue;
			if (!wep.has<WeaponFiringComponent>() || !wep.has<IrrlichtComponent>()) continue;
			auto wepFire = wep.get_mut<WeaponFiringComponent>();
			wepFire->isFiring = true;

			//target position here is the futzed target pos
			btVector3 wepFacing = targetPosition - irrVecToBt(wepFire->spawnPosition);
			wepFacing = wepFacing.normalize();

			if (ai->hasBehavior(AIBHVR_VELOCITY_TRACKING) && firstWepInfo) {
				btVector3 velTarg = targetPosition + (targRBC->rigidBody->getLinearVelocity() * travelTime);
				if (ai->hasBehavior(AIBHVR_SELF_VELOCITY_TRACKING)) velTarg -= (rbc->rigidBody->getLinearVelocity() * travelTime);
				wepFacing = velTarg - irrVecToBt(wepFire->spawnPosition);
				wepFacing = wepFacing.normalize();
			}
			wepFire->firingDirection = btVecToIrr(wepFacing);
		}
	}
	else {
		m_toggleAllWeps(hards, false);
	}
}

void AIType::stateCheck(aiSystemInfo& inf)
{
	if (inf.ai->orderOverride || inf.ai->state == AI_STATE_SHATTERED) { //override states -- if you're shattered you're done, if you're overridden you're done
		return;
	}
	if (inf.sns->closestHostileContact == INVALID_ENTITY) {
		if (inf.ai->hasPatrolRoute) inf.ai->state = AI_STATE_PATROL;
		else inf.ai->state = AI_STATE_IDLE;
		return;
	}
	else if (inf.hp->health <= (inf.hp->maxHealth * (1.f - inf.ai->resolve)) && inf.ai->hasBehavior(AIBHVR_SUICIDAL_FLEE)) {
		//There's a hostile and my health is lower than my resolve -- SHATTER
		inf.ai->state = AI_STATE_SHATTERED;
		return;
	}
	auto pursuitTarget = inf.sns->closestHostileContact;
	if (inf.sns->targetContact != pursuitTarget) { //emit "target" sound when targeting the player
		if (pursuitTarget == gameController->getPlayer() && pursuitTarget.is_alive()) {
			//massively increased ref distance to be sure the player hears it
			audioDriver->playGameSound(inf.ent, inf.ai->targetingAudio, 1.f, 650.f);
			gameController->triggerCombatMusic();
			pursuitTarget.get_mut<PlayerComponent>()->beingTargetedBy = inf.ent;
		}
		if (pursuitTarget.is_alive()) {
			if (pursuitTarget.has<AIComponent>()) {
				pursuitTarget.get_mut<AIComponent>()->beingTargetedBy = inf.ent;
			}
		}
	}
	inf.sns->targetContact = pursuitTarget;
	inf.ai->state = AI_STATE_PURSUIT;
	//whoop its ass!
}

bool AIType::m_basicWingDistanceCheck(AIComponent* aiComp, BulletRigidBodyComponent* rbc, f32 m_dist)
{
	if (aiComp->wingCommander == INVALID_ENTITY || !aiComp->wingCommander.is_alive() || !aiComp->onWing) return true;
	auto cdrRBC = aiComp->wingCommander.get<BulletRigidBodyComponent>();
	if (!cdrRBC) return true;

	btVector3 dist = cdrRBC->rigidBody->getCenterOfMassPosition() - rbc->rigidBody->getCenterOfMassPosition();
	if (dist.length() > m_dist) return false;

	return true;
}

bool AIType::m_distanceToWingCheck(aiSystemInfo& inf)
{
	f32 scale = inf.irr->node->getScale().X / 4.f;
	if (scale < 1.f) scale = 1.f;
	return m_basicWingDistanceCheck(inf.ai, inf.rbc, inf.ai->wingDistance * scale);
}
bool AIType::m_combatDistanceToWingCheck(aiSystemInfo& inf)
{
	f32 scale = inf.irr->node->getScale().X / 4.f;
	if (scale < 1.f) scale = 1.f;
	return m_basicWingDistanceCheck(inf.ai, inf.rbc, inf.ai->maxWingDistance * inf.ai->aggressiveness * scale);
}

void AIType::idle(aiSystemInfo& inf)
{
	//sit down and think about what you've done
	if (m_distanceToWingCheck(inf)) {
		inf.ai->orderOverride = false;

		if (m_isOnWingCheck(inf)) {
			smoothTurnToDirection(inf.rbc->rigidBody, inf.thrst, getRigidBodyForward(inf.ai->wingCommander.get<BulletRigidBodyComponent>()->rigidBody));
		}
		else inf.thrst->moves[STOP_ROTATION] = true;
		inf.thrst->moves[STOP_VELOCITY] = true;
	}
	else m_formOnWing(inf);

	m_toggleAllWeps(inf.hrd, false);
}

void AIType::flee(aiSystemInfo& inf)
{
	auto fleeTarget = inf.sns->closestHostileContact;
	if (fleeTarget == INVALID_ENTITY) return;
	if (!fleeTarget.is_alive()) {
		stateCheck(inf);
		return;
	}
	if (!fleeTarget.has<IrrlichtComponent>()) return;
	auto fleeIrr = fleeTarget.get<IrrlichtComponent>();

	vector3df distance = fleeIrr->node->getPosition() - inf.irr->node->getPosition(); //vector between the two things
	distance.normalize();
	vector3df targetVector = -distance; //runs away in the opposite direction

	btScalar angle = getRigidBodyForward(inf.rbc->rigidBody).angle(irrVecToBt(targetVector));
	angle *= RADTODEG;

	inf.thrst->moves[THRUST_FORWARD] = true;
	//if(inf.ai->state==AI_STATE_FLEE) inf.thrst->safetyOverride = true;

	if (inf.ai->onWing && inf.ai->wingCommander.is_alive()) {
		targetVector = (inf.irr->node->getPosition() - inf.ai->wingCommander.get<IrrlichtComponent>()->node->getPosition()).normalize();
	}

	if (angle > 55.f) previousVec = targetVector;
	//turn away and hit the gas as fast as possible
	if (inf.ai->hasBehavior(AIBHVR_RANDOM_WALK_ON_FLEE)) {
		targetVector = makeVectorInaccurate(previousVec, .2f);
		previousVec = targetVector;
	}
	smoothTurnToDirection(inf.rbc->rigidBody, inf.thrst, irrVecToBt(targetVector));

	if (inf.ai->hasBehavior(AIBHVR_DEFENSIVE_BOOST) && angle <=70.f) { //if it's close to the flee vector, boost
		inf.thrst->moves[THRUST_BOOST] = true;
	}

	m_toggleAllWeps(inf.hrd, false);
}

void AIType::shatter(aiSystemInfo& inf)
{
	m_toggleAllWeps(inf.hrd, false);
	inf.thrst->safetyOverride = true;
	inf.thrst->moves[THRUST_FORWARD] = true;
	if (inf.ai->hasBehavior(AIBHVR_RANDOM_WALK_ON_FLEE)) {
		auto targetVector = makeVectorInaccurate(previousVec, .2f);
		previousVec = targetVector;
		smoothTurnToDirection(inf.rbc->rigidBody, inf.thrst, irrVecToBt(targetVector));
	}
}

void AIType::pursue(aiSystemInfo& inf)
{
	auto pursuitTarget = inf.sns->targetContact;
	if (pursuitTarget == INVALID_ENTITY || !pursuitTarget.is_alive()) {
		inf.sns->targetContact = INVALID_ENTITY;
		inf.ai->state = AI_STATE_PATROL;
		inf.ai->orderOverride = false;
		return;
	}
	if (!pursuitTarget.has<BulletRigidBodyComponent>()) return;
	auto targetRBC = pursuitTarget.get<BulletRigidBodyComponent>();

	btVector3 targetPos = targetRBC->rigidBody->getCenterOfMassPosition();
	btVector3 pos = inf.rbc->rigidBody->getCenterOfMassPosition();

	btVector3 tailPos = targetPos + (getRigidBodyBackward(inf.rbc->rigidBody) * 20.f);
	if (inf.ai->hasBehavior(AIBHVR_ATTEMPT_TO_RAM)) tailPos = targetPos;
	btVector3 distVec = tailPos - pos;

	btVector3 targFacing = pos - targetPos;
	targFacing.normalize();
	btVector3 targForward = getRigidBodyForward(targetRBC->rigidBody);
	btScalar targAngle = targForward.angle(targFacing);

	btScalar distance = distVec.length();

	if (inf.ai->hasBehavior(AIBHVR_FLEES_WHEN_TARGETED)) {
		if ((targAngle * RADTODEG) <= 15.f && distance <= (700.f * (1.f - inf.ai->resolve))) { //if the target is pointed at IT, turn tail and run
			flee(inf);
			return;
		}
	}
	btVector3 facing = targetPos - pos;
	facing = facing.normalize();

	m_fireAtTarget(inf.rbc, targetRBC, inf.ai, inf.hrd); //start shooting if it's in line of sight

	if (m_isOnWingCheck(inf) && !m_combatDistanceToWingCheck(inf)) {
		m_formOnWing(inf);
		return; //form up FIRST if not already
	}
	//If it's not behind the ship, get behind it
	f32 goToVal = 40.f;
	if (inf.ai->hasBehavior(AIBHVR_ATTEMPT_TO_RAM)) goToVal = 1.f; //ramming prioritizes over max behavior
	else if (inf.ai->hasBehavior(AIBHVR_STAY_AT_MAX_RANGE)) goToVal = firingDistance * .7f;
	if (distance > goToVal) {
		m_moveToPoint(tailPos, inf);
		if (inf.ai->hasBehavior(AIBHVR_AGGRESSIVE_BOOST) && distance > 450.f) {
			inf.thrst->moves[THRUST_BOOST] = true;
		}
	}
	else {
		//if it is behind it, start turning towards it
		inf.thrst->moves[STOP_VELOCITY] = true;
		smoothTurnToDirection(inf.rbc->rigidBody, inf.thrst, facing);
	}
}

void AIType::m_formOnWing(aiSystemInfo& inf)
{
	if (m_distanceToWingCheck(inf)) return;
	m_moveToPoint(inf.ai->wingCommander.get<BulletRigidBodyComponent>()->rigidBody->getCenterOfMassPosition(), inf);
}

bool AIType::m_isOnWingCheck(aiSystemInfo& inf)
{
	if (inf.ai->onWing && !inf.ai->wingCommander.is_alive()) {
		inf.ai->onWing = false;
	}
	return inf.ai->onWing;
}

void AIType::patrol(aiSystemInfo& inf)
{
	if (!inf.ai->hasPatrolRoute) {
		inf.ai->state = AI_STATE_IDLE;
		idle(inf);
		return;
	}
	btVector3 start = inf.ai->startPos;
	btVector3 curPos = inf.rbc->rigidBody->getCenterOfMassPosition();

	if (!inf.ai->onPatrol) {
		m_moveToPoint(start, inf);
		if ((start - curPos).length() <= 40.f) {
			inf.ai->onPatrol = true;
			if (!inf.ai->fixedPatrolRoute) {
				inf.ai->route.clear();
				inf.ai->route.push_back(start + btVector3(400, 0, 0));
				inf.ai->route.push_back(start + btVector3(0, 0, 400));
				inf.ai->route.push_back(start + btVector3(-400, 0, 0));
				inf.ai->route.push_back(start + btVector3(0, 0, -400));
				inf.ai->movingToPoint = 0;
			}
		}
	}

	btVector3 stop(0, 0, 0);
	if (!inf.ai->route.empty()) stop = inf.ai->route[inf.ai->movingToPoint];
	else stop == inf.rbc->rigidBody->getCenterOfMassPosition(); //whar? shoulda generated a patrol order
	m_moveToPoint(stop, inf);

	if ((stop - curPos).length() <= 40.f) {
		++inf.ai->movingToPoint;
		if (inf.ai->movingToPoint == inf.ai->route.size()) inf.ai->movingToPoint = 0;
	}
	m_toggleAllWeps(inf.hrd, false);
}

void AIType::docking(aiSystemInfo& inf)
{
	const auto targ = inf.sns->targetContact;	
	if (!targ.has<StationModuleComponent>()) {
		inf.ai->orderOverride = false;
		return;
	}
	auto smod = targ.get<StationModuleComponent>();
	if (!smod->hasDock) {
		const StationModuleOwnerComponent* stationCenter = nullptr;
		if (targ.has<StationModuleOwnerComponent>()) stationCenter = targ.get<StationModuleOwnerComponent>();
		else stationCenter = smod->ownedBy.get<StationModuleOwnerComponent>();

		if (!stationCenter) {
			inf.ai->orderOverride = false;
			return;
		}
		//find a dock
		for (u32 i = 0; i < stationCenter->dockCount; ++i) {
			if (stationCenter->docks[i].is_alive()) {
				inf.sns->targetContact = stationCenter->docks[i];
				return; //start again next tick
			}
		}
		//if we haven't returned by now there are literally no docks available
		inf.ai->orderOverride = false;
		return;
	}
	auto rbc = targ.get<BulletRigidBodyComponent>()->rigidBody;
	auto irr = targ.get<IrrlichtComponent>()->node;
	//rotate the dock's connection dir as the "out"
	auto dockdir = -irr->getAbsoluteTransformation().getRotationDegrees().rotationToDirection(smod->connectionDirection[0]);
	const btVector3 dockPoint = rbc->getCenterOfMassPosition() + irrVecToBt(dockdir * 5.f * irr->getScale().X);
	m_moveToPoint(dockPoint, inf, true);

	if ((dockPoint - inf.rbc->rigidBody->getCenterOfMassPosition()).length() <= 4.f) {
		inf.ai->state = AI_STATE_DOCKED;
		smod->ownedBy.get_mut<StationModuleOwnerComponent>()->currentlyDockedShips.push_back(inf.ent);
		halt(inf);
	}
	m_toggleAllWeps(inf.hrd, false);
}

void AIType::m_moveToPoint(btVector3 position, aiSystemInfo& inf, bool precise, bool accountForVelocity)
{
	btVector3 pos = position;

	if (!inf.ai->hasBehavior(AIBHVR_MORON) && inf.ai->hasBehavior(AIBHVR_AVOIDS_OBSTACLES) && inf.ai->obstacleAvoidTimer >= .5f) {//if the ai isn't an idiot, check for obstacles
		auto obstacle = obstacleOnVelocityVector(inf.rbc->rigidBody);
		if (obstacle) {
			btVector3 newPosTarget = pathAroundObstacle(inf.rbc->rigidBody, obstacle, pos);
			inf.ai->avoiding = true;
			inf.ai->avoidPoint = newPosTarget;
			pos = newPosTarget;
		}
		else {
			inf.ai->avoiding = false;
		}
		inf.ai->obstacleAvoidTimer = 0.f;
	}
	if (inf.ai->avoiding) {
		pos = inf.ai->avoidPoint;
	}
	//if there was an obstacle (and the AI isn't stupid) we now have a new target to move towards
	if (precise) goToPointPrecise(inf.rbc->rigidBody, inf.thrst, pos);
	else if (inf.ai->hasBehavior(AIBHVR_MORON)) goToPointVague(inf.rbc->rigidBody, inf.thrst, pos);
	else goToPointPilot(inf.rbc->rigidBody, inf.thrst, pos, 45.f, 70.f, 10.f, accountForVelocity);
}

void AIType::docked(aiSystemInfo& inf)
{
	halt(inf);
}

void AIType::halt(aiSystemInfo& inf)
{
	inf.thrst->moves[STOP_ROTATION] = true;
	inf.thrst->moves[STOP_VELOCITY] = true;
	m_toggleAllWeps(inf.hrd, false);

}