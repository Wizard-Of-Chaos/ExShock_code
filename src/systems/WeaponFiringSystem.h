#pragma once
#ifndef WEAPONFIRINGSYSTEM_H
#define WEAPONFIRINGSYSTEM_H
#include "BaseHeader.h"
struct WeaponInfoComponent;
struct WeaponFiringComponent;
struct IrrlichtComponent;
struct PowerComponent;
/*
* This function handles whether or not a weapon is currently shooting and spawns the appropriate projectile entities
* that are associated with that weapon. Used by both the player and the AI.
*/
void weaponFiringSystem(flecs::iter it, WeaponInfoComponent* wic, WeaponFiringComponent* fireC, IrrlichtComponent* irrC, PowerComponent* pwrc);

#endif
