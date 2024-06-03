#pragma once
#ifndef STATUSEFFECTSYSTEM_H
#define STATUSEFFECTSYSTEM_H
#include "BaseHeader.h"

struct StatusEffectComponent;
//The status effect system takes any status effects and applies them to the appropriate entities.
//A status effect in this sense is anything that happens over time; this could be taking damage once a tick or spawning enemies.
void statusEffectSystem(flecs::iter it, StatusEffectComponent* efc);

#endif 