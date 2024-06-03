#pragma once
#ifndef SHIPPARTICLECOMPONENT_H
#define SHIPPARTICLECOMPONENT_H
#include "BaseHeader.h"
#include "ShipComponent.h"

/*
* Ship particle components handle the various particle systems for a given ship, i.e., the thrust jets and the engine effects. They should not be
* included in any actual gameplay-related calculation.
* 
* TODO: Merge these back into the actual ship component, since that thing is almost entirely cosmetic by this point.
*/
struct ShipParticleComponent
{
	//Positions on the ship for where the thrust emissions come from.
	IParticleSystemSceneNode* upJetEmit[2] = { nullptr, nullptr };
	IParticleSystemSceneNode* downJetEmit[2] = { nullptr, nullptr };
	IParticleSystemSceneNode* leftJetEmit[2] = { nullptr, nullptr };
	IParticleSystemSceneNode* rightJetEmit[2] = { nullptr, nullptr };
	IParticleSystemSceneNode* reverseJetEmit[2] = { nullptr, nullptr };
	IVolumeLightSceneNode* engineJetEmit[MAX_ENGINES] = { nullptr, nullptr, nullptr, nullptr };
	ILightSceneNode* engineLight[MAX_ENGINES] = { nullptr, nullptr, nullptr, nullptr }; //The engine light is on! Check your oil.
	IParticleSystemSceneNode* flamePoints[4] = { nullptr, nullptr, nullptr, nullptr };
	IParticleSystemSceneNode* smokePoints[4] = { nullptr, nullptr, nullptr, nullptr };
	IMeshSceneNode* shield = nullptr;

};


#endif 