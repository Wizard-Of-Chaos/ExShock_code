#include "GameAssets.h"
#include "AttributeReaders.h"
#include "CrashLogger.h"

IMesh* Assets::getMesh(std::string name, bool& firstLoad)
{
	if (meshAssets[name]) {
		firstLoad = false;
		return meshAssets[name];
	}
	IMesh* mesh = smgr->getMesh(name.c_str());
	if (mesh) meshAssets[name] = mesh;
	firstLoad = true;
	return meshAssets[name];
}

ITexture* Assets::getTexture(std::string name, bool inGame)
{
	if (clearableTextureAssets[name]) {
		return clearableTextureAssets[name];
	}
	else if (unclearableTextureAssets[name]) {
		return unclearableTextureAssets[name];
	}
	ITexture* tex = driver->getTexture(name.c_str());
	if (inGame) clearableTextureAssets[name] = tex;
	else unclearableTextureAssets[name] = tex;

	return tex;
}

btConvexHullShape Assets::getHull(std::string name)
{
	if (hullAssets.find(name) != hullAssets.end()) {
		return hullAssets[name];
	}

	btConvexHullShape hull;
	std::string shapeLoc = "assets/attributes/hulls/" + name + ".bullet";
	if (!loadHull(shapeLoc, hull)) {
		baedsLogger::log("Hull not found for " + name + ". Attempting to build from existing mesh... ");
		if (meshAssets[name]) {
			hull = createCollisionShapeFromMesh(meshAssets[name]);
			saveHull(shapeLoc, hull);
			baedsLogger::log("Done. Hull created and saved to " + shapeLoc + ".\n");
		}
		else {
			baedsLogger::log("No mesh found for " + name + ".\n");
		}
	}
	hullAssets[name] = hull;

	return hull;
}

IGUIFont* Assets::getFont(std::string name)
{
	if (fontAssets[name]) {
		return fontAssets[name];
	}

	fontAssets[name] = guienv->getFont(name.c_str());
	return fontAssets[name];
}

void Assets::clearGameAssets()
{
	auto meshes = smgr->getMeshCache();

	for (auto [name, mesh] : meshAssets) {
		if (mesh)meshes->removeMesh(mesh);
	}
	for (auto [name, tex] : clearableTextureAssets) {
		if (tex)driver->removeTexture(tex);
	}
	meshes->clear();
	meshAssets.clear();
	clearableTextureAssets.clear();
	hullAssets.clear();
}