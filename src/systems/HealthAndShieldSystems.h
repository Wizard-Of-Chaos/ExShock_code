#pragma once
#ifndef HEALTHANDSHIELDSYSTEMS_H
#define HEALTHANDSHIELDSYSTEMS_H
#include "BaseHeader.h"

struct HealthComponent;

//Updates health for all entities in the scene. If health is 0, removes it from the scene.
void healthSystem(flecs::iter it, HealthComponent* hc);
#endif 

