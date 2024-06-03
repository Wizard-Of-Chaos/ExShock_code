#pragma once
#ifndef PROJECTILEUTILS_H
#define PROJECTILEUTILS_H
#include "BaseHeader.h"

struct WeaponInfoComponent;
struct WeaponFiringComponent;
struct PowerComponent;
struct Network_ShotFired;

bool wepFire_kinetic(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_kineticKick(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_plasma(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_rock(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_missile(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_railgun(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_laser(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_thickLaser(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);
bool wepFire_goron(WeaponInfoComponent* wep, WeaponFiringComponent* fire, PowerComponent* power, flecs::entity wepId, f32 dt, Network_ShotFired* chaser = nullptr);

//Creates an explosion at the impact point of a projectile. Way smaller than the other one.
void projectileImpactCallback(flecs::entity id);
//Callback for when a missile blows up.
void missileDeathCallback(flecs::entity miss);
#endif 