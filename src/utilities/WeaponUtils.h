#pragma once
#ifndef WEAPONUTILS_H
#define WEAPONUTILS_H
#include "BaseHeader.h"
#include "WeaponInfoComponent.h"
/*
* This function gets called when a projectile smacks into something else, and it fires off the appropriate functions.
* For example, the impulse cannon and missiles call an explosion on impact, and the gravity bolas sets a couple of entity flags.
*/
struct IrrlichtComponent;

const extern std::unordered_map<std::string, ProjInfo_hitCb> impactCbStrings;
const extern std::unordered_map<std::string, WepInfo_UpdateCb> updateCbStrings;
const extern std::unordered_map<std::string, WepInfo_FireCb> fireCbStrings;

void handleProjectileHit(flecs::entity proj, flecs::entity impacted);

void wepHit_projDamage(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint);
void wepHit_laserDamage(flecs::entity wep, flecs::entity impacted, btVector3 hitPoint);
//don't actually use this.
void wepHit_instantKill(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint);
void wepHit_projKnockback(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint);
void wepHit_bfgExplosion(flecs::entity projectile, flecs::entity impacted, btVector3 hitPoint);

void wepHit_impulseBlast(flecs::entity proj, flecs::entity impacted, btVector3 hitPoint);
void wepHit_missileExplosion(flecs::entity proj, flecs::entity impacted, btVector3 hitPoint);
void wepHit_bolas(flecs::entity proj, flecs::entity impacted, btVector3 hitPoint);
void wepHit_energyGrapple(flecs::entity proj, flecs::entity impacted, btVector3 hitPoint);
void wepHit_physHook(flecs::entity weapon, flecs::entity impacted, btVector3 hitPoint);
void wepHit_physSlowdownExplosion(flecs::entity projId, flecs::entity impacted, btVector3 hitPoint);

/*
* If weapons have anything they should be doing in their downtime, that'll get called here. For instance, the gravity bolas
* needs to keep track of how long the connection between two entities lasts.
*/
void wepUpdate_basic(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt);
void wepUpdate_bolas(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt);
void wepUpdate_railgun(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt);
void wepUpdate_goron(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepEnt, f32 dt);

void missileDeathCallback(flecs::entity miss);

#endif 