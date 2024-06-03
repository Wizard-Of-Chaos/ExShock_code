#pragma once

#ifndef SHIPMOVEMENTUTILS_H
#define SHIPMOVEMENTUTILS_H
#include "BaseHeader.h"
/*
* Holds a whole bunch of functions that are mostly used with AI to make the ship move around.
* Also includes bullet normalized vectors and the forces to move in specific directions.
*/

struct ThrustComponent;
/*FORCE VECTORS*/

//Gets a force in the forward direction, modified by the ship's forward thrust.
btVector3 getForceForward(btRigidBody* body, ThrustComponent* thrust);

//Gets an afterburner force in the ship's forward direction.
btVector3 getForceBoost(btRigidBody* body, ThrustComponent* thrust);

//Gets a force in the backward direction, modified by the ship's braking thrust.
btVector3 getForceBackward(btRigidBody* body, ThrustComponent* thrust);

//Gets a force in the right direction, modified by the ship's strafing thrust.
btVector3 getForceRight(btRigidBody* body, ThrustComponent* thrust);

//Gets a force in the left direction, modified by the ship's strafing thrust.
btVector3 getForceLeft(btRigidBody* body, ThrustComponent* thrust);

//Gets a force in the up direction, modified by the ship's strafing thrust.
btVector3 getForceUp(btRigidBody* body, ThrustComponent* thrust);

//Gets a force in the down direction, modifed by the ship's strafing thrust.
btVector3 getForceDown(btRigidBody* body, ThrustComponent* thrust);

/* ROTATIONAL VECTORS */

//Gets a torque to pitch up, modified by the ship's pitch thrust.
btVector3 getTorquePitchUp(btRigidBody* body, ThrustComponent* thrust);

//Gets a torque to pitch down, modified by the ship's pitch thrust.
btVector3 getTorquePitchDown(btRigidBody* body, ThrustComponent* thrust);

//Gets a torque to yaw right, modified by the ship's yaw thrust.
btVector3 getTorqueYawRight(btRigidBody* body, ThrustComponent* thrust);

//Gets a torque to yaw left, modified by the ship's yaw thrust.
btVector3 getTorqueYawLeft(btRigidBody* body, ThrustComponent* thrust);

//Gets a torque to roll right, modified by the ship's roll thrust.
btVector3 getTorqueRollRight(btRigidBody* body, ThrustComponent* thrust);

//Gets a torque to roll left, modified by the ship's roll thrust.
btVector3 getTorqueRollLeft(btRigidBody* body, ThrustComponent* thrust);

/* MOVEMENT VECTORS */

//Returns the torque in the opposite direction of the current angular velocity, modified by the ship's rotational speed.
btVector3 getTorqueToStopAngularVelocity(btRigidBody* body, ThrustComponent* thrust);

//Returns the force in the opposite direction of the current linear velocity, modified by the ship's speed.
btVector3 getForceToStopLinearVelocity(btRigidBody* body, ThrustComponent* thrust);

//Updates the ship's moves array to pitch and yaw towards the given direction. No brakes. Basic turn movement.
void turnToDirection(btRigidBody* body, ThrustComponent* thrust, btVector3 dir);

//Combines torque and opposing torque to try and smoothly turn towards the direction.
void smoothTurnToDirection(btRigidBody* body, ThrustComponent* thrust, btVector3 dir, bool print=false);

//Wildly blasts as quickly as it can towards the given direction with no regards for brakes. Stops when it damn well feels like it. Drifty.
void drunkTurnToDirection(btRigidBody* body, ThrustComponent* thrust, btVector3 dir);

//Applies force and torque to move towards the given point, arriving as precisely as possible. Lots of stops and starts for precision. No strafes.
void goToPointPrecise(btRigidBody* body, ThrustComponent* thrust, btVector3 dest);

//Drunkenly careens in the given direction while mashing the gas. Hits the brakes a fixed distance away with no accounting for velocity. No strafes.
void goToPointVague(btRigidBody* body, ThrustComponent* thrust, btVector3 dest);

//Flies toward the given point with some margin for error. Applies strafe thrusts to cushion itself on overshoots. Smooth movements. 
void goToPointPilot(btRigidBody* body, ThrustComponent* thrust, btVector3 dest, f32 linPrecision=45.f, f32 fastAngle=70.f, f32 slowAngle=10.f, bool accountForVelocity =true);

//Checks to see whether or not there is an obstacle in the path of the current velocity of the rigid body. Checks 5 seconds in the future.
const btCollisionObject* obstacleOnVelocityVector(btRigidBody* rb);

//Checks to see whether or not there is an obstacle in the path from the rigid body's current position to the target.
const btCollisionObject* obstacleOnPath(btRigidBody* rb, const btVector3& target);

//Given the rigid body in question and the collision object it's about to smack into alongside the ultimate target, find a position to target that leads away from the obstacle.
//Note: First use either obstacleOnVelocityVector or obstacleOnPath to determine whether or not there is an obstacle.
const btVector3 pathAroundObstacle(const btRigidBody* body, const btCollisionObject* obstacle, const btVector3& target);
#endif 