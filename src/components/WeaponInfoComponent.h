#pragma once

#ifndef WEAPONINFOCOMPONENT_H
#define WEAPONINFOCOMPONENT_H
#include "BaseHeader.h"
#include "HealthComponent.h"
#include <map>

/*
* This enum holds the different types of weapon that a gun can be.
* The PHYS tag means its a goofy weapon.
*/
enum HARDPOINT_TYPE
{
	HRDP_REGULAR = 0,
	HRDP_PHYSICS = 1,
	HRDP_HEAVY = 2,
};

const std::map<std::string, HARDPOINT_TYPE> hardpointStrings{
	{"regular", HRDP_REGULAR},
	{"physics", HRDP_PHYSICS},
	{"heavy", HRDP_HEAVY}
};

enum WEAPON_TYPE {
	WEP_NONE = -1,
	WEP_ENERGY = 0,
	WEP_KINETIC = 1,
	WEP_PHYS_IMPULSE = 2,
	WEP_PHYS_BOLAS = 3,
	WEP_PHYS_GRAPPLE = 4,
	WEP_HEAVY_MISSILE = 5,
	WEP_HEAVY_LASER = 6,
	WEP_HEAVY_FLAMETHROWER =7,
	WEP_HEAVY_RAILGUN=8,
	WEP_ROCK = 9,
	WEP_PHYS_SHIELD = 10
};

//A map for convenience when pulling values out of files.
const std::map<std::string, WEAPON_TYPE> weaponStrings{
	{"none", WEP_NONE},
	{"energy", WEP_ENERGY},
	{"missile", WEP_HEAVY_MISSILE},
	{"kinetic", WEP_KINETIC},
	{"phys_impulse", WEP_PHYS_IMPULSE},
	{"phys_bolas", WEP_PHYS_BOLAS},
	{"phys_grapple", WEP_PHYS_GRAPPLE},
	{"heavy_laser", WEP_HEAVY_LASER},
	{"heavy_missile", WEP_HEAVY_MISSILE},
	{"heavy_flamethrower", WEP_HEAVY_FLAMETHROWER},
	{"heavy_railgun", WEP_HEAVY_RAILGUN},
	{"rock", WEP_ROCK},
	{"phys_shield", WEP_PHYS_SHIELD}
};

struct WeaponInfoComponent;
struct WeaponFiringComponent;
struct PowerComponent;
struct Network_ShotFired;
//The entity here is the projectile and the victim, in that order.
typedef std::function<void(flecs::entity, flecs::entity, btVector3)> ProjInfo_hitCb;
//Updates the weapon in question. In a list of effects.
typedef std::function<void(WeaponInfoComponent*, WeaponFiringComponent*, PowerComponent*, flecs::entity, f32)> WepInfo_UpdateCb;
//Fires the weapon in question (if possible).
typedef std::function<bool(WeaponInfoComponent*, WeaponFiringComponent*, PowerComponent*, flecs::entity, f32, Network_ShotFired*)> WepInfo_FireCb;

const u32 MAX_PROJECTILES_PER_SHOT = 10;

//WEAPON ENTITIES:
//Irrlicht component, weapon info component, weapon firing component
struct WeaponFiringComponent
{
	bool isFiring = false;
	bool hasFired = false;
	vector3df firingDirection = vector3df(0, 0, 1);
	vector3df spawnPosition = vector3df(0);

	u32 ammunition = 0;
	u32 clip = 0;
	f32 timeSinceLastShot = 0.f;
	f32 timeReloading = 0.f;

	IParticleSystemSceneNode* muzzleFlashEmitter=nullptr;
	IMeshSceneNode* muzzleFlash=nullptr;
	ILightSceneNode* muzzleFlashLight = nullptr;
	f32 flashTimer = 0.f;
};

struct Network_ShotFired
{
	Network_ShotFired() {
		for (u32 i = 0; i < MAX_PROJECTILES_PER_SHOT; ++i) {
			projIds[i] = INVALID_ENTITY_ID;
			directions[i] = vector3df(0);
		}
	}
	uint32_t firingWeapon = INVALID_ENTITY_ID;
	vector3df directions[MAX_PROJECTILES_PER_SHOT];
	vector3df spawn = vector3df(0);
	uint32_t projIds[MAX_PROJECTILES_PER_SHOT];
};

/*
* The weapon info component holds some basic information about the weapon. It holds
* the projectile speed, firing speed, range, damage, and some information to determine when to fire the gun.
* It also holds the type of weapon, which is loaded in AttributeLoaders as an integer.
*
* In order to load a weapon, first the data is pulled from the appropriate .gdat file into the stateController. After that, the weapon
* is loaded onto the given ship and the textures / model / projectiles are loaded. When fired from the weaponFiringSystem function, a projectile entity
* gets created and added to the scene, and it eventually gets despawned when it hits something or goes out of range. In order to add a new weapon, you'll
* need to add your new attributes file where it defines the basic information and texture/model locations, then any additional behaviors you might want depending
* on the weapon type (i.e., you implemented a new type and need to define how the hell a teapot launcher works) and finally if you've added extra data (or extra components) you'll need
* to add in some extra loading information to the relevant functions in AttributeReaders.cpp (loadWeaponData and loadWeapon).
*/
struct WeaponInfoComponent
{
	u32 wepDataId;
	HARDPOINT_TYPE hrdtype = HRDP_REGULAR;
	WEAPON_TYPE type = WEP_ENERGY;
	DAMAGE_TYPE dmgtype = ENERGY;

	//default to perfect accuracy
	f32 accuracy = 20.f;
	u32 projectilesPerShot = 1;

	f32 firingSpeed = 1.f; //how long it should take in seconds between shots
	f32 projectileSpeed = 500.f; //how fast the projectile goes
	f32 lifetime = 2.f; //how long the projectile lasts in the world
	f32 damage = 10.f;
	f32 multiplier = 1.f;

	bool usesAmmunition=false;
	u32 maxAmmunition=0;
	u32 maxClip=0;
	f32 reloadTime=0;

	bool hasFired=false; //used for the railgun
	bool hitScan = false;
	bool usesPower = false;
	f32 powerCost = 0.f;

	//the size of the shot visually
	f32 scale = 1.f;
	//the length of the shot visually
	f32 length = 1.f;
	//the radius of the projectile sphere
	f32 radius = 1.f;

	ITexture* particle;
	std::string fireSound;
	std::string impactSound;
	vector3df barrelStart;

	SColor projectileLightColor;

	WepInfo_FireCb fire;
	std::list<WepInfo_UpdateCb> updates;
	std::list<ProjInfo_hitCb> hitEffects;
};

//PROJECTILE ENTITIES:
//Irrlicht component, projectile info component, rigid body component

/*
* The projectile info component duplicates the information from the weapon info component that it spawned from.
* It also holds its own start position and its range, as well as a bullet component to be able to collide with ships properly.
*/
struct ProjectileInfoComponent
{
	WEAPON_TYPE type;
	DAMAGE_TYPE dmgtype;
	f32 damage;
	f32 multiplier;
	f32 speed;
	f32 lifetime;
	f32 currentLifetime;
	f32 length = 1.f;
	vector3df startPos;
	vector3df fireDir;
	std::string impactSound;
	ITexture* particle;

	std::list<ProjInfo_hitCb> hitEffects;
	bool hit = false;
};

#endif