#pragma once
#ifndef HUDHEALTHBAR_H
#define HUDHEALTHBAR_H
#include "BaseHeader.h"
#include "HUDElement.h"
#include "HardpointComponent.h"
#include "ShipComponent.h"
#include "HUDFillBar.h"
/*
* The health bar element holds the data for the health bar, as well as some text to show the actual
* current health number. The health is adjusted in accordance with the % of the player's current health,
* containing logic to show the appropriate percentage of the actual health bar itself.
*/

class HUDComms;

class HUDResources : public HUDElement
{
public:
	HUDResources(IGUIElement* root, flecs::entity id);
	~HUDResources() override;
	virtual void updateElement(flecs::entity playerId);

	IGUIImage* lBg;
	//IGUIImage* rBg;
	IGUIImage* centBg;
	//IGUIStaticText* keys;
	HUDComms* comms;

	f32 sideWidthRatio;
	f32 sideHeightRatio;
	f32 centerWidthRatio;
	f32 centerHeightRatio;
	dimension2du sideSize;
	dimension2du centerSize;
	FillBar* hp;
	FillBar* shields;
	FillBar* energy;
	FillBar* velocity;
	IGUIImage* weps[MAX_HARDPOINTS];
	FillBar* ammoBars[MAX_HARDPOINTS];
	IGUIImage* hvyAmmoBar;
	FillBar* hvyBar;

	IGUIImage* hpHit[4];
};

#endif