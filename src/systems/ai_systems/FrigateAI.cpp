#include "AITypes.h"
#include "btUtils.h"
#include "ShipMovementUtils.h"
#include "SensorComponent.h"
#include "AIComponent.h"
#include "TurretHardpointComponent.h"
#include "ThrustComponent.h"
void FrigateAI::stateCheck(aiSystemInfo& inf)
{
	AIType::stateCheck(inf);
}

void FrigateAI::idle(aiSystemInfo& inf)
{
	//sit down and think about what you've done
	if (m_distanceToWingCheck(inf)) {
		inf.ai->orderOverride = false;
		inf.thrst->moves[STOP_ROTATION] = true;
		inf.thrst->moves[STOP_VELOCITY] = true;
	}
	else m_formOnWing(inf);
	//frigates should not rotate if off their wing
	m_toggleAllWeps(inf.hrd, false);
}

void FrigateAI::flee(aiSystemInfo& inf)
{
	//frigates absolutely should not flee in the same way that fighters do
	halt(inf);
}

void FrigateAI::pursue(aiSystemInfo& inf)
{
	flecs::entity targ = inf.sns->targetContact;
	if (!targ.is_alive()) {
		inf.ai->state = AI_STATE_IDLE;
		return;
	}
	//first thing to do is check if the frigate has turrets
	const bool hasTurrets = inf.ent.has<TurretHardpointComponent>();
	if (hasTurrets) {
		auto turrets = inf.ent.get_mut<TurretHardpointComponent>();
		for (u32 i = 0; i < turrets->turretCount; ++i) {
			if (!turrets->turrets[i].is_alive()) continue;
			if (!turrets->turrets[i].has<SensorComponent>()) continue; //sometimes you add checks that seem like nonsense.
			if (turrets->turrets[i].get<SensorComponent>()->targetContact == targ) continue;
			//turrets->turrets[i].get_mut<SensorComponent>()->targetContact = targ; //trigger all the turrets to have the pursuit as their target contact
		}
	}
	if (!targ.has<BulletRigidBodyComponent>()) {
		stateCheck(inf);
		return;
	}
	auto targetRBC = targ.get<BulletRigidBodyComponent>();

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
	//we're ignoring form on 
	m_fireAtTarget(inf.rbc, targetRBC, inf.ai, inf.hrd); //start shooting if it's in line of sight

	btVector3 facing = targetPos - pos;
	facing = facing.normalize();

	//If it's not behind the ship, get behind it
	f32 goToVal = 40.f;
	if (inf.ai->hasBehavior(AIBHVR_ATTEMPT_TO_RAM)) goToVal = 1.f; //ramming prioritizes over max behavior
	else if (inf.ai->hasBehavior(AIBHVR_STAY_AT_MAX_RANGE)) goToVal = firingDistance;
	if (distance > goToVal) {
		m_moveToPoint(tailPos, inf, true);
		if (inf.ai->hasBehavior(AIBHVR_AGGRESSIVE_BOOST) && distance > 1500.f) {
			inf.thrst->moves[THRUST_BOOST] = true;
		}
	}
	else { //if we have turrets, stay still and let them do the work.
		if (!hasTurrets) {
			smoothTurnToDirection(inf.rbc->rigidBody, inf.thrst, facing);
		}
		else if (hasTurrets && inf.ent.get<TurretHardpointComponent>()->turretsDead()) {
			smoothTurnToDirection(inf.rbc->rigidBody, inf.thrst, facing);
		}
		else { //in this case we want to stop rotating too; it has turrets and they're alive
			inf.thrst->moves[STOP_ROTATION] = true;
		}
		inf.thrst->moves[STOP_VELOCITY] = true;
	}

}

void FrigateAI::patrol(aiSystemInfo& inf)
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

	auto stop = inf.ai->route[inf.ai->movingToPoint];

	m_moveToPoint(stop, inf, true);

	if ((stop - curPos).length() <= 40.f) {
		++inf.ai->movingToPoint;
		if (inf.ai->movingToPoint == inf.ai->route.size()) inf.ai->movingToPoint = 0;
	}
	m_toggleAllWeps(inf.hrd, false);
}