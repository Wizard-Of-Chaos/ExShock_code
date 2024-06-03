#pragma once
#ifndef IRRLICHTCOMPONENT_H
#define IRRLICHTCOMPONENT_H
#include "BaseHeader.h"

/*
* The Irrlicht component is a wrapper for the base Scene Node that an entity might have (for ships, the actual ship mesh).
* It also stores the name of whatever it's attached to - and that's an actual string, not the entity ID or the ID itself.
* 
* The Irrlicht node's actual name (called with node->getName(), returning a char*) is the EntityId associated with that Irrlicht node,
* which is used for things like selection. The Irrlicht node's ID determines whether or not it can be avoided, selected, etc.
*/
struct IrrlichtComponent {
	ISceneNode* node = nullptr;
	bool downscaled = false;
	std::string topMesh = "";
	std::string botMesh = "";
	IAttributes* serializedMats[8];
	std::vector<ITexture*> textures[8];
	E_MATERIAL_TYPE baseMat = EMT_SOLID;
	IrrlichtComponent() : node(nullptr) {
		for (u32 i = 0; i < 8; ++i) {
			serializedMats[i] = nullptr;
		}
	}
};

#endif