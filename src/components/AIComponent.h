#pragma once

#ifndef AICOMPONENT_H
#define AICOMPONENT_H
#include "BaseHeader.h"
#include <unordered_map>
/*
* The different states an AI might be in. Implemented using steering behaviors in
* the AI_Systems folder.
*/
enum AI_STATE
{
	AI_STATE_IDLE,
	AI_STATE_PURSUIT,
	AI_STATE_FLEE,
	AI_STATE_SHATTERED, //thanks, total war
	AI_STATE_SEARCH,
	AI_STATE_PATROL,
	AI_STATE_DOCKING,
	AI_STATE_DOCKED,
	AI_STATE_HALT,
	AI_STATE_PLAYER //?
};

enum AI_BEHAVIOR_FLAG
{
	AIBHVR_VELOCITY_TRACKING = 1,
	AIBHVR_SELF_VELOCITY_TRACKING = 2,
	AIBHVR_DEFENSIVE_BOOST = 4,
	AIBHVR_AGGRESSIVE_BOOST = 8,
	AIBHVR_STAY_AT_MAX_RANGE = 16,
	AIBHVR_FLEES_WHEN_TARGETED = 32,
	AIBHVR_RANDOM_WALK_ON_FLEE = 64,
	AIBHVR_ATTEMPT_TO_RAM = 128,
	AIBHVR_MORON = 256,//uses drunk go-to instead of normal go-to
	AIBHVR_SUICIDAL_FLEE = 512, //if yes, runs away and dies, if no, closes in on its wing commander for safety
	AIBHVR_CAN_DOCK = 1024, //this thing can attempt to dock with othe crap
	AIBHVR_CAN_ISSUE_ORDERS = 2048,
	AIBHVR_CAN_BE_ORDERED = 4096,
	AIBHVR_SCREAMS_FOR_HELP = 8192,
	AIBHVR_CARES_ABOUT_WINGMEN = 16384,
	AIBHVR_AVOIDS_OBSTACLES = 32768,
};

enum ORDER_TYPE
{
	ORD_ATTACK,
	ORD_FORMUP,
	ORD_HALT,
	ORD_DOCK,
	ORD_HONK_HELP,
	ORD_DISENGAGE,
	ORD_MAX
};

struct Order
{
	ORDER_TYPE type = ORDER_TYPE::ORD_MAX;
	flecs::entity target = INVALID_ENTITY;
	flecs::entity from = INVALID_ENTITY;
};

const u32 AI_DEFAULT_BEHAVIORS = (AIBHVR_SUICIDAL_FLEE | AIBHVR_RANDOM_WALK_ON_FLEE);

//Default reaction time. Default is to check state once per second.
const f32 AI_DEFAULT_REACTION_TIME = 1.f;
//The AI will run when it is below 25% health, by default. Represented as a float.
const f32 AI_DEFAULT_RESOLVE = .15f;
//When issued a form-on-wing order the AI will try to get 25 units away.
const f32 AI_DISTANCE_FROM_WING = 200.f;
//If the AI gets further away than the max distance it will regroup with its buddies first.
const f32 AI_MAX_DISTANCE_FROM_WING = 800.f;
//The multiplier on how far the AI will go outside of their wing chasing an enemy.
const f32 AI_DEFAULT_AGGRESSIVENESS = 1.f;
//How well the AI aims, where higher is better and lower is less accurate.
const f32 AI_DEFAULT_AIM = .5f;

class AIType;

//The component that controls the AI for a given AI ship.
//Includes the type of AI, the current state of the AI, how quickly it reacts
//(changes state), the damage point at which it will flip out, and time since
//the last time it changed states (internal use).
struct AIComponent
{
	u32 behaviorFlags = AI_DEFAULT_BEHAVIORS;
	void setBehavior(u32 flags) { behaviorFlags |= flags; }
	void removeBehavior(u32 flags) { behaviorFlags &= ~flags; }
	const bool hasBehavior(AI_BEHAVIOR_FLAG flag) const { return !!(behaviorFlags & flag); }

	std::shared_ptr<AIType> aiControls; //The control setup for the AI, deciding which functions it uses.
	AI_STATE state = AI_STATE_IDLE; //The state the AI is currently in.
	f32 reactionSpeed = AI_DEFAULT_REACTION_TIME; //how long it takes to change states
	f32 timeSinceLastStateCheck = 0; //this is going to be the time since the last state check
	f32 timeInCurrentState = 0;

	f32 obstacleAvoidTimer = 0.f;
	bool avoiding = false;
	btVector3 avoidPoint;

	f32 resolve = AI_DEFAULT_RESOLVE; // %hp that the AI will run at
	f32 aggressiveness = AI_DEFAULT_AGGRESSIVENESS; //multiplier on how far the AI will go on a chase
	f32 aim = AI_DEFAULT_AIM; //how well the AI aims

	f32 wingDistance = AI_DISTANCE_FROM_WING; //The distance that is considered to be "on wing".
	f32 maxWingDistance = AI_MAX_DISTANCE_FROM_WING; //The distance that is considered to be "on wing" when in combat.
	bool onWing = false;

	flecs::entity wingCommander = INVALID_ENTITY; //if the AI isn't an ace
	bool orderOverride = false;

	bool orderHandled = true;
	Order mostRecentOrder;
	void registerOrder(ORDER_TYPE type, flecs::entity from, flecs::entity target = INVALID_ENTITY) {
		mostRecentOrder = { type, target, from }; 
		orderHandled = false; 
	}

	std::list<flecs::entity> unitsOnWing;
	flecs::entity beingTargetedBy = INVALID_ENTITY;

	btVector3 startPos; //used for patrolling
	std::vector<btVector3> route;

	u32 movingToPoint = 0; //Which point on the route the AI is currently moving toward.
	bool hasPatrolRoute = false;
	bool fixedPatrolRoute = false;
	bool onPatrol = false;

	std::string orderLines[ORD_MAX];

	std::string targetingAudio = "targeted_minor.ogg";
	std::string AIName = "AI";
	std::string deathLine = "Dead";
	std::string killLine = "Killed target";
	std::string negLine = "Negative";
};
#endif
