#pragma once
#ifndef AITYPES_H
#define AITYPES_H
#include "BaseHeader.h"

struct ShipComponent;
struct BulletRigidBodyComponent;
struct SensorComponent;
struct IrrlichtComponent;
struct HealthComponent;
struct ThrustComponent;
struct HardpointComponent;
struct AIComponent;

struct aiSystemInfo
{
	flecs::entity ent;
	AIComponent* ai;
	ThrustComponent* thrst;
	HardpointComponent* hrd;
	BulletRigidBodyComponent* rbc;
	IrrlichtComponent* irr;
	SensorComponent* sns;
	HealthComponent* hp;
	f32 dt;
};


class AIType
{
	public:
		//Basic state check for an AI to determine what state it should be in for "free range" AI.
		virtual void stateCheck(aiSystemInfo& inf);
		//Idle behavior for an AI.
		virtual void idle(aiSystemInfo& inf);
		//Flee behavior for an AI.
		virtual void flee(aiSystemInfo& inf);
		//Pursuit behavior for an AI.
		virtual void pursue(aiSystemInfo& inf);
		//Patrol behavior for an AI.
		virtual void patrol(aiSystemInfo& inf);
		//Docking behavior with a station module for an AI. Requires the AIBHVR_CAN_DOCK flag. Only accessible via order.
		virtual void docking(aiSystemInfo& inf);
		//Docked behavior. Requires the AIBHVR_CAN_DOCK flag. Only accessible via the prior docking behavior.
		virtual void docked(aiSystemInfo& inf);
		//Complete shutdown behavior for an AI. Only accessible via order.
		virtual void halt(aiSystemInfo& inf);
		//Shatter behavior where it sprints away screaming and dies.
		virtual void shatter(aiSystemInfo& inf);

	protected:
		f32 firingDistance = 0.f;
		vector3df previousVec = vector3df(0.f, 0.f, 0.f);

		virtual void m_toggleAllWeps(HardpointComponent* hards, bool firing = true);
		virtual void m_setAIWeapon(flecs::entity wep, bool firing);

		virtual bool m_isOnWingCheck(aiSystemInfo& inf);
		virtual bool m_distanceToWingCheck(aiSystemInfo& inf);
		virtual bool m_combatDistanceToWingCheck(aiSystemInfo& inf);
		virtual bool m_basicWingDistanceCheck(AIComponent* aiComp, BulletRigidBodyComponent* rbc, f32 m_dist);

		virtual void m_fireAtTarget(const BulletRigidBodyComponent* rbc, const BulletRigidBodyComponent* targRBC, const AIComponent* ai, HardpointComponent* hards);

		virtual void m_formOnWing(aiSystemInfo& inf);

		virtual void m_moveToPoint(btVector3 position, aiSystemInfo& inf, bool precise=false, bool accountForVelocity=true);
};

class FrigateAI : public AIType
{
	public:
		//Basic state check for an AI to determine what state it should be in for "free range" AI.
		virtual void stateCheck(aiSystemInfo& inf);
		//Idle behavior for an AI.
		virtual void idle(aiSystemInfo& inf);
		//Flee behavior for an AI.
		virtual void flee(aiSystemInfo& inf);
		//Pursuit behavior for an AI.
		virtual void pursue(aiSystemInfo& inf);
		//Patrol behavior for an AI.
		virtual void patrol(aiSystemInfo& inf);
	protected:
};
#endif 