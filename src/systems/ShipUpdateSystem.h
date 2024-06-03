#pragma once
#ifndef SHIPUPDATESYSTEM_H
#define SHIPUPDATESYSTEM_H
#include "BaseHeader.h"

struct ThrustComponent;
struct ShipComponent;
struct BulletRigidBodyComponent;
struct IrrlichtComponent;
struct ShipParticleComponent;
struct PowerComponent;
/*
* Actually updates the ship component in a given scene. This function plays the audio associated with each movement,
* updates the particle effects while moving (with the jet functions above), but DOES NOT apply torques and forces - that's 
* handled by the thrust system. Instead this system will adjust the values on thrust components if they do things like, say, go over
* the safety limits.
*/

void shipUpdateSystem(flecs::iter it, ThrustComponent* thrc, ShipComponent* shpc, BulletRigidBodyComponent* rbcs, IrrlichtComponent* irrc, ShipParticleComponent* prtc, PowerComponent* pwrc);

#endif 