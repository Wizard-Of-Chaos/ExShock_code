#pragma once
#ifndef THRUSTCOMPONENT_H
#define THRUSTCOMPONENT_H
#include "BaseHeader.h"

/*
* The different types of movement a thrustable object can make.
* Thrust forward, strafe left/right/up/down/back, and then pitch/yaw/roll.
* It also has STOP_VELOCITY and STOP_ROTATION to get the opposing force to rotation and velocity.
*
* The MAX_MOVEMENTS enum is used to create arrays for movement.
*/
enum MOVEMENT {
	THRUST_FORWARD,
	THRUST_BOOST,
	THRUST_BACKWARD,
	STRAFE_LEFT,
	STRAFE_RIGHT,
	STRAFE_UP,
	STRAFE_DOWN,
	PITCH_UP,
	PITCH_DOWN,
	ROLL_LEFT,
	ROLL_RIGHT,
	YAW_LEFT,
	YAW_RIGHT,
	STOP_VELOCITY,
	STOP_ROTATION,
	MAX_MOVEMENTS
};

/*
* The thrust component handles how fast a given entity is able to move in various directions, such as pitch/yaw/roll speed and forward acceleration.
* It also keeps track of the speed cap for a given entity (the max speed it can move before it starts taking damage) as well as how tolerant it is to
* over-speed damage and whether or not the safety is currently overridden.
* 
* For turrets, forward and strafe should be set to zero, since a turret can't, y'know, move.
*/
class AudioSource;

struct ThrustComponent
{
	f32 pitch=0.f;
	f32 yaw=0.f;
	f32 roll=0.f;
	f32 forward=0.f;
	f32 strafe=0.f;
	f32 brake=0.f;
	f32 boost=0.f; //if the thing has a boost capability (afterburners).
	f32 boostCd = 0.4f;
	f32 curBoostCd=0.f;
	bool nonstopThrust = false;
	bool moves[MAX_MOVEMENTS];
	f32 velocityTolerance=0.00001f; //How tolerant the entity is of going over the max speed it has
	f32 linearMaxVelocity=4000.f; //Max linear velocity
	f32 angularMaxVelocity=4000.f; //Max angular velocity

	f32 multiplier = 1.f;
	const f32 getPitch() const { return pitch * multiplier; }
	const f32 getYaw() const { return yaw * multiplier; }
	const f32 getRoll() const { return roll * multiplier; }
	const f32 getForward() const { return forward * multiplier; }
	const f32 getStrafe() const { return strafe * multiplier; }
	const f32 getBrake() const { return brake * multiplier; }
	const f32 getBoost() const { return boost * multiplier; }
	const f32 getLinMax() const { return linearMaxVelocity * multiplier; }
	const f32 getAngMax() const { return angularMaxVelocity * multiplier; }

	bool safetyOverride = false; //whether or not this thing is allowing itself to move faster than max
	AudioSource* ambient = nullptr; //audio source, if it has one

	ThrustComponent() {
		for (u32 i = 0; i < MAX_MOVEMENTS; ++i) {
			moves[i] = false;
		}
	}
};

#endif 