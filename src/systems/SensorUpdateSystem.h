#pragma once
#ifndef SENSORUPDATESYSTEM_H
#define SENSORUPDATESYSTEM_H

#include "BaseHeader.h"
struct SensorComponent;
struct SensorCallbackComponent;
/*
* Updates all sensor components, which are used by the AI and by the player to determine what the hell is going on.
* It checks whether or not an entity is in range of the sensor, and updates accordingly.
*/
void sensorSystem(flecs::iter it, SensorComponent* sns);

#endif 