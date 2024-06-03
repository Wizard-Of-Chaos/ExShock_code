#pragma once

#ifndef HUDELEMENT_H
#define HUDELEMENT_H
#include "BaseHeader.h"
/*
* The base class for all HUD elements. HUD elements should inherit from this class.
* Includes the root node for the HUD element as part of the constructor, but doesn't actually HOLD
* the root HUD element. The root HUD element is part of the player class, and should be used to show or hide
* the entire HUD at once.
*/
class HUDElement
{
public:
	HUDElement(IGUIElement* root) : root(root) {}
	virtual ~HUDElement() {}
	virtual void updateElement(flecs::entity playerId) = 0;
	IGUIElement* root;
};

#endif