#pragma once
#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H
#include "BaseHeader.h"

struct HealthComponent;
//The damage system applies damage from all damage instances to their given entities.
void damageSystem(flecs::iter it, HealthComponent* hc);

#endif 