#pragma once

#ifndef PLAYERCOMPONENT_H
#define PLAYERCOMPONENT_H

#include "BaseHeader.h"
#include <memory>

class HUDElement;
class HUDContact;
class HUDResources;
class HUDActiveSelection;
class HUDPopup;

/*
* The player component stores things that are exclusive to the player and get adjusted by the player.
* It stores the camera scene node, which allows the player to actually view the scene, the target scene node,
* which is tied to the IrrlichtComponent of the player ship and updates to allow the camera to rotate.
* 
* TODO: Why the hell is the HUD in a component?
*/
struct PlayerComponent
{
	ICameraSceneNode* camera = nullptr;
	ICameraSceneNode* reverseCamera = nullptr;
	ISceneNode* target = nullptr;
	//values for how much the camera swings around
	f32 slerpFactor = .009f;
	f32 velocityFactor = .02f;
	f32 timeSinceLastOrder = 0.f;
	f32 inputTimeDelay = 0.f;
	//identical to the one for AI
	flecs::entity beingTargetedBy = INVALID_ENTITY;
};

#endif