#pragma once
#ifndef SENSORCOMPONENT_H
#define SENSORCOMPONENT_H
#include "BaseHeader.h"
#include "ShipComponent.h"
#include "FactionComponent.h"
#include "BulletRigidBodyComponent.h"

#include <tuple>
#include <vector>
//Default range of a sensor
const f32 DEFAULT_SENSOR_RANGE = 1000.f;
//default time it updates on - how long in between checks
const f32 DEFAULT_SENSOR_UPDATE_INTERVAL = 1.95f;

//Typedef for the information about a given contact, used by the sensors.
struct ContactInfo
{
	flecs::entity ent=INVALID_ENTITY;
	const BulletRigidBodyComponent* rbc=nullptr;
	const FactionComponent* fac=nullptr;
	f32 dist=0.f;
};
/*
* The sensor component allows a given entity to "see" what's around it. It looks for any Bullet body
* within the radius (default 1000), checks whether or not it's friendly, and then updates its list of contacts
* accordingly (as well as closest hostile and friendly contacts).
* 
* This component is updated in the SensorUpdateSystem.
*/

struct SensorComponent
{
	f32 detectionRadius = 0.f;
	std::vector<ContactInfo> contacts;
	std::vector<ContactInfo> nearbyOrderableContacts;
	flecs::entity closestContact = INVALID_ENTITY;
	flecs::entity closestHostileContact = INVALID_ENTITY;
	flecs::entity closestFriendlyContact = INVALID_ENTITY;
	flecs::entity targetContact = INVALID_ENTITY; //contact for whoever the sensors are actually focused on
	f32 timeSelected = 0.f;

	bool onlyDetectShips = false;

	f32 updateInterval;
	f32 timeSinceLastUpdate;
};

#endif