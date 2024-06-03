#pragma once

#ifndef GAMEFUNCTIONS_H
#define GAMEFUNCTIONS_H
#include "BaseHeader.h"
#include "StatusEffectComponent.h"
#include "IrrlichtUtils.h"
/*
* A file of convenience functions that are used with the game.
*/

#define MARKED_FOR_DEATH 1

class AIType;

//Translates a bullet vector to an irrlicht vector.
vector3df btVecToIrr(btVector3 vec);
//Translates an irrlicht vector to a bullet vector.
btVector3 irrVecToBt(vector3df vec);
vector3df randomRotationVector();
vector3df randomDirectionalVector();
vector3df randomPositionalVector(f32 max = 15000.f);

//Gets a point in the defined sphere.
vector3df getPointInSphere(vector3df center, f32 radius);
//Checks if a point is in the defined sphere.
bool isPointInSphere(vector3df& point, vector3df center, f32 radius);
//Checks if a given point is within a sphere around any of the points contained in the given vector.
bool isPointCloseToList(vector3df& point, std::vector<vector3df>& points, f32 radius);

//Gets a point in the defined torus. The torus will have "up" as perpendicular to the torus.
vector3df getPointInTorus(vector3df center, f32 innerRadius, f32 outerRadius, vector3df up=vector3df(0,1,0));
//Gets a point in a shell around an area. The inner sphere is defined by innerRadius, while the total radius is defined by outerRadius.
vector3df getPointInShell(vector3df center, f32 innerRadius, f32 outerRadius);
//Gets a point in an arbitrary capsule defined by the start, end, and radius of said capsule.
vector3df getPointInCapsule(vector3df start, vector3df end, f32 radius);
//Gets a point in a flat disc with the given center, radius, and up vector.
vector3df getPointInDisc(vector3df center, f32 radius, vector3df up = vector3df(0, 1, 0));
//Gets a point in a plane defined by the given points.
//TODO: Make this take in just topLeft and botRight.
vector3df getPointInBoundedPlane(vector3df topLeft, vector3df topRight, vector3df botLeft);

//Removes the given object from the scene.
void destroyObject(flecs::entity id);
void destroyObject_real(flecs::entity id, bool finalDelete=false);

//Adds on a player component and an input component to a given ship.
//Requires: Irrlicht component. Returns false without that.
bool initializeDefaultPlayer(flecs::entity shipId);
//Adds a health component to the given entity. 
void initializeHealth(flecs::entity id, f32 healthpool);
//Adds default health component.
void initializeDefaultHealth(flecs::entity objectId);

//Creates AI component with the assigned values.
void initializeAI(flecs::entity id, f32 react, f32 coward, f32 aggro, f32 aim, u32 behaviors, AIType* type=nullptr);
//Creates an AI component with default values.
void initializeDefaultAI(flecs::entity id, AIType* type=nullptr);
//Creates an AI component with ace values.
void initializeAceAI(flecs::entity id, AIType* type=nullptr);

//Creates a dummy entity at the given position so the player has a radio signal to follow.
flecs::entity createRadioMarker(vector3df pos, std::string name);

f32 getDamageDifficulty(f32 dmg, bool player = true);
void setDamageDifficulty(flecs::entity ship);
//Get the current adjusted difficulty value of the AI's aim. 
//The "adjust" parameter is up/down from the current difficulty (aces are usually bumped up by one).
f32 getCurAiAim(s32 adjust=0);
//Get the current adjusted difficulty value of the AI's wing count. 
//The "adjust" parameter is up/down from the current difficulty (aces are usually bumped up by one).
u32 getCurAiNum(s32 adjust = 0);
//Get the current adjusted difficulty value of the AI's behaviors. 
//The "adjust" parameter is up/down from the current difficulty (aces are usually bumped up by one).
u32 getCurAiBehaviors(s32 adjust = 0);
//Get the current adjusted difficulty value of the AI's resolve. 
//The "adjust" parameter is up/down from the current difficulty (aces are usually bumped up by one).
f32 getCurAiResolve(s32 adjust = 0);

//Creates an explosion at the point that lasts for the duration.
void explosiveForce(vector3df position, f32 radius, f32 damage, f32 force);
void implosiveForce(vector3df position, f32 radius, f32 damage, f32 force);
//Explodes. I really don't think this one needs a comment. If damage and force are set, those apply in the radius. Type determines the particle effect.
void explode(vector3df position, f32 duration, f32 scale, f32 radius, f32 damage, f32 force, EXPLOSION_TYPE type=EXTYPE_REGULAR);
void implode(vector3df position, f32 duration, f32 scale, f32 radius, f32 damage, f32 force, EXPLOSION_TYPE type = EXTYPE_REGULAR);
//Gets a particle effect billboard (as in like an explosion animation) and makes it an entity.
void particleEffectBill(std::string which, vector3df position, f32 duration = 1.f, f32 scale = 5.f, bool light=false, SColorf lightCol=SColorf(0,0,0,0));
//Creates a de-cloaking effect at the given position.
void decloakEffect(vector3df position, f32 scale);

struct StatusEffect;

//Adds the given status effect to the entity. Creates a StatusEffectComponent if one doesn't exist.
void addStatusEffectToEntity(StatusEffect* eff, flecs::entity ent);

//Refreshes the status effect on the entity (if applicable).
void refreshStatusEffectOnEntity(UNIQUE_STATUS_EFFECT effect, flecs::entity ent);
#endif