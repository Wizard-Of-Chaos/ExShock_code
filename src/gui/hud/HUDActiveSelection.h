#pragma once
#ifndef HUDACTIVESELECTION_H
#define HUDACTIVESELECTION_H
#include "BaseHeader.h"
#include "HUDElement.h"
#include "HUDFillBar.h"
/*
* This class holds the currently active selection for the player. It updates where it is on the screen
* and holds information to display itself and the name of the given entity selected. When the entity is removed
* (i.e. you kill it) it hides itself and stops updating.
*/
class HUDActiveSelection : public HUDElement
{
public:
	HUDActiveSelection(IGUIElement* root);
	~HUDActiveSelection() override;
	f32 selectTimer = 0.f;
	f32 timeBetweenSelects = .15f;
	virtual void updateElement(flecs::entity playerId);
	flecs::entity selected=INVALID_ENTITY;

	void setVisible(bool vis = true);
	void setType();

	IGUIStaticText* name;
	IGUIStaticText* dist;
	IGUIStaticText* objType;
	IGUIImage* selectGUI;
	IGUIImage* crosshair;
	FillBar* hp;
	FillBar* sp;
	IGUIImage* selectHP;
	IGUIImage* selectSP;

	btVector3 crosshairTarget;
};

#endif 
