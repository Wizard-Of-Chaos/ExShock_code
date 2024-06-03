#pragma once
#ifndef GAMEASSETS_H
#define GAMEASSETS_H

#include "BaseHeader.h"

//Holds all the default textures and sounds and meshes for the game that can be used as placeholders if you can't load your custom mesh.
//When you're loading something, you should set it up in here so it can get cleared when the game ends.

class Assets
{

	public:

		//bool setMeshAsset(std::string name, IMesh* mesh);
		//bool setTextureAsset(std::string name, ITexture* texture);

		//IMesh* getMeshAsset(std::string name);
		//ITexture* getTextureAsset(std::string name);
		//ITexture* getHUDAsset(std::string name);
		//btConvexHullShape getHullAsset(std::string name);
		//IGUIFont* getFontAsset(std::string name);
		//void clearLoadedGameAssets();

		void setMesh(std::string name, IMesh* mesh) { meshAssets[name] = mesh; }
		IMesh* getMesh(std::string name, bool& firstLoad);
		ITexture* getTexture(std::string name, bool inGame = true);
		btConvexHullShape getHull(std::string name);
		IGUIFont* getFont(std::string name);
		void clearGameAssets();
	private:
		std::unordered_map<std::string, IMesh*> meshAssets;
		std::unordered_map<std::string, ITexture*> unclearableTextureAssets;
		std::unordered_map<std::string, ITexture*> clearableTextureAssets;
		std::unordered_map<std::string, btConvexHullShape> hullAssets;
		std::unordered_map<std::string, IGUIFont*> fontAssets;
};
#endif