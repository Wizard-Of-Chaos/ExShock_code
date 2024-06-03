#include "AttributeReaders.h"
#include "GameController.h"
#include "GameStateController.h"
#include "ShipUpgrades.h"
#include "LargeShipUtils.h"
#include "ShipUtils.h"
#include "FadeInParticleAffector.h"
#include "StationModuleUtils.h"
#include "Shaders.h"
#include "WeaponUtils.h"
#include "btUtils.h"
#include "Archetypes.h"
#include "Config.h"
#include "GameAssets.h"
#include "Campaign.h"
#include "IrrlichtUtils.h"
#include "GameFunctions.h"
#include "CrashLogger.h"
#include "ShipParticleComponent.h"

static void readTurretData(gvReader& in, TurretHardpointComponent& turr)
{
	turr.turretCount = in.getUint("turretCount");
	turr.turretScale = in.getFloat("turretScale");
	for (u32 i = 0; i < turr.turretCount; ++i) {
		std::string pos = "turretPos" + std::to_string(i);
		std::string rot = "turretRot" + std::to_string(i);
		turr.turretPositions[i] = in.getVec(pos);
		turr.turretRotations[i] = in.getVec(rot);
	}
}

static void readHealthData(gvReader& in, HealthComponent& hp)
{
	hp.maxHealth = in.getFloat("health");
	hp.health = hp.maxHealth;
	hp.healthRegen = in.getFloat("healthRegen");

	std::stringstream split(in.getString("healthResistances"));
	for (u32 i = 1; i < MAX_DAMAGE_TYPES; ++i) {
		std::string resist = "0";
		std::getline(split, resist, ',');
		hp.healthResistances[i] = std::stof(resist);
	}

	split = std::stringstream(in.getString("shieldResistances"));
	for (u32 i = 1; i < MAX_DAMAGE_TYPES; ++i) {
		std::string resist = "0";
		std::getline(split, resist, ',');
		hp.shieldResistances[i] = std::stof(resist);
	}

	hp.maxShields = in.getFloat("shields");
	hp.shields = in.getFloat("shields");
	hp.rechargeDelay = in.getFloat("rechargeDelay");
	hp.rechargeRate = in.getFloat("rechargeRate");
	hp.timeSinceLastHit = hp.rechargeDelay;
}

//loads textures and the mesh - if "dummy" is set to true then it returns nullptr -- useful for pre-loading material and object data
ISceneNode* _loadObjModelData(std::string& modelPath, std::vector<_materialTexLayer>& materials, IrrlichtComponent* component, bool dummy)
{
	bool firstLoad = false;
	//load mesh
	IMesh* mesh = assets->getMesh(modelPath, firstLoad);
	if (firstLoad) {
		//first time loading? give it some tangents
		IMesh* initmesh = mesh;
		mesh = smgr->getMeshManipulator()->createMeshWithTangents(initmesh);
		smgr->getMeshCache()->removeMesh(initmesh);
		assets->setMesh(modelPath, mesh);
	}
	IMeshSceneNode* node = nullptr;
	if (!dummy) {
		node = smgr->addMeshSceneNode(mesh);
	}
	//load individual texture layers
	if (!dummy) {
		for (u32 i = 0; i < materials.size(); ++i) {
			SMaterial& mat = node->getMaterial(i);
			auto& layer = materials[i];
			for (u32 j = 0; j < layer.count; ++j) {
				if (!cfg->vid.toggles[TOG_BUMP] && j == 1) break;
				ITexture* tex = assets->getTexture(layer.tex[j]);
				if (tex) mat.setTexture(j, tex);
				if (component) {
					component->textures[i].push_back(tex);
				}
			}
		}
	}
	else { //if dummy flag is set just load the textures, don't mess with materials
		for (u32 i = 0; i < materials.size(); ++i) {
			auto& layer = materials[i];
			for (u32 j = 0; j < layer.count; ++j) {
				ITexture* tex = assets->getTexture(layer.tex[j]);
			}
		}
	}
	return node;
}

s32 loadShipData(std::string path, gvReader& in)
{
	baedsLogger::log("Reading in ship off path " + path + "... ");
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return -1;
	}
	in.readLinesToValues();
	s32 id = std::stoi(in.values["id"]);
	std::string name = in.values["name"];
	ShipData* data = new ShipData;

	data->canBuild = (in.getString("canBuild") == "yes") ? true : false;
	data->canLoot = (in.getString("canLoot") == "yes") ? true : false;
	data->hasTurrets = (in.getString("hasTurrets") == "yes") ? true : false;

	if (in.hasVal("hasHangar")) data->hasHangar = (in.getString("hasHangar") == "yes") ? true : false;

	if (in.hasVal("artilleryShip")) data->artilleryShip = (in.getString("artilleryShip") == "yes") ? true : false;

	data->id = id;
	data->name = name;
	data->description = in.values["description"];

	std::string folder = in.getString("path");

	std::string meshpath = "assets/models/" + folder + in.getString("model");
	data->shipMesh = meshpath;
	if (in.hasVal("collider")) data->shipCollisionMesh = "assets/models/" + folder + in.getString("collider");

	std::string materialPath = "assets/models/" + folder;

	u32 totalMaterials = 0;
	while (in.hasVal("material_" + std::to_string(totalMaterials))) {
		std::string mat = in.getString("material_" + std::to_string(totalMaterials));
		std::stringstream split(mat);
		_materialTexLayer layer;
		for (u32 i = 0; i < MATERIAL_MAX_TEXTURES; ++i) {
			std::string tex = "";
			std::getline(split, tex, ',');
			if (tex != "") {
				++layer.count;
				std::string texturePath = materialPath + tex;
				layer.tex[i] = texturePath;
			}
			else break;
		}
		data->materials.push_back(layer);
		++totalMaterials;
	}

	std::string jetpath = "assets/effects/" + in.values["jet"];
	std::string enginepath = "assets/effects/" + in.values["engine"];

	data->engineTipCenter = in.getColorSilly("enginetip_center");
	data->engineTipEdge = in.getColorSilly("enginetip_edge");
	data->engineEndCenter = in.getColorSilly("engineend_center");
	data->engineEndEdge = in.getColorSilly("engineend_edge");
	data->shieldColor = in.getColorSilly("shieldColor");

	data->engineTexture = enginepath;
	data->jetTexture = enginepath;

	data->thrust.forward = std::stof(in.values["forwardThrust"]);
	data->thrust.brake = std::stof(in.values["brakeThrust"]);
	data->thrust.strafe = std::stof(in.values["strafeThrust"]);
	data->thrust.pitch = std::stof(in.values["pitchThrust"]);
	data->thrust.yaw = std::stof(in.values["yawThrust"]);
	data->thrust.roll = std::stof(in.values["rollThrust"]);
	data->thrust.velocityTolerance = std::stof(in.values["velocityTolerance"]);
	data->thrust.linearMaxVelocity = std::stof(in.values["linearMaxVelocity"]);
	data->thrust.angularMaxVelocity = std::stof(in.values["angularMaxVelocity"]);
	data->thrust.boost = std::stof(in.values["boostThrust"]);

	data->hards.hardpointCount = std::stoi(in.values["hardpointCount"]);

	std::string val;
	for (unsigned int i = 0; i < data->hards.hardpointCount; ++i) {
		val = "hardpoint" + std::to_string(i);
		data->hards.hardpoints[i] = strToVec(in.values[val]);
		data->hards.hardpointRotations[i] = in.getVec("hardpointRot" + std::to_string(i));

	}
	for (unsigned int i = 0; i < 2; ++i) {
		val = "upJetPos" + std::to_string(i);
		data->ship.upJetPos[i] = strToVec(in.values[val]);
	}
	for (unsigned int i = 0; i < 2; ++i) {
		val = "downJetPos" + std::to_string(i);
		data->ship.downJetPos[i] = strToVec(in.values[val]);
	}
	for (unsigned int i = 0; i < 2; ++i) {
		val = "leftJetPos" + std::to_string(i);
		data->ship.leftJetPos[i] = strToVec(in.values[val]);
	}
	for (unsigned int i = 0; i < 2; ++i) {
		val = "rightJetPos" + std::to_string(i);
		data->ship.rightJetPos[i] = strToVec(in.values[val]);
	}
	for (u32 i = 0; i < 2; ++i) {
		val = "reverseJetPos" + std::to_string(i);
		data->ship.reverseJetPos[i] = strToVec(in.values[val]);
	}
	data->ship.engineCount = in.getUint("engineCount");
	for (u32 i = 0; i < data->ship.engineCount; ++i) {
		val = "engineJetPos" + std::to_string(i);
		data->ship.engineJetPos[i] = strToVec(in.getString(val));
	}
	data->hards.physWeaponHardpoint = strToVec(in.values["physWeaponHardpoint"]);
	data->hards.physHardpointRot = in.getVec("physHardpointRot");

	data->hards.heavyWeaponHardpoint = in.getVec("heavyHardpoint");
	data->hards.heavyWeaponRot = in.getVec("heavyHardpointRot");

	data->power.power = std::stof(in.values["maxEnergy"]);
	data->power.maxPower = data->power.power;
	data->power.powerRegen = std::stof(in.values["energyRegenRate"]);
	data->power.generator = true;
	data->power.isPowered = true;

	data->thrust.safetyOverride = false;
	data->ship.shipDataId = id;
	data->buildCost = in.getFloat("buildCost");

	readHealthData(in, data->hp);

	data->scale = in.getVec("scale");
	data->mass = in.getFloat("mass");

	if (data->hasTurrets) {
		readTurretData(in, data->turr);
	}
	if (data->hasHangar) {
		baedsLogger::log(" with hangar... ");
		data->hangar.reserveShips = in.getUint("reserveShips");
		data->hangar.launchRate = in.getFloat("launchRate");
	}

	shipData[id] = data;
	baedsLogger::log("Done.\n");
	return id;
}

s32 loadTurretData(std::string path, gvReader& in)
{
	baedsLogger::log("Reading turret from " + path + "... ");
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return -1;
	}
	in.readLinesToValues();
	s32 id = in.getUint("id");
	std::string name = in.getString("name");

	TurretData* data = new TurretData;
	data->name = name;

	std::string folder = in.getString("path");
	std::string meshpath = "assets/models/" + folder + in.getString("model");
	data->mesh = meshpath;

	std::string materialPath = "assets/models/" + folder;

	u32 totalMaterials = 0;
	while (in.hasVal("material_" + std::to_string(totalMaterials))) {
		std::string mat = in.getString("material_" + std::to_string(totalMaterials));
		std::stringstream split(mat);
		_materialTexLayer layer;
		for (u32 i = 0; i < MATERIAL_MAX_TEXTURES; ++i) {
			std::string tex = "";
			std::getline(split, tex, ',');
			if (tex != "") {
				++layer.count;
				std::string texturePath = materialPath + tex;
				layer.tex[i] = texturePath;
			}
			else break;
		}
		data->materials.push_back(layer);
		++totalMaterials;
	}

	data->description = in.getString("description");

	data->hards.hardpointCount = in.getUint("hardpointCount");
	for (u32 i = 0; i < data->hards.hardpointCount; ++i) {
		data->hards.hardpoints[i] = in.getVec("hardpoint" + std::to_string(i));
		data->hards.hardpointRotations[i] = in.getVec("hardpointRot" + std::to_string(i));
	}
	data->thrust.pitch = in.getFloat("pitchThrust");
	data->thrust.yaw = in.getFloat("yawThrust");
	data->thrust.roll = 50.f; //added for braking power
	data->thrust.linearMaxVelocity = 4000.f;
	data->thrust.angularMaxVelocity = 4000.f;
	data->thrust.velocityTolerance = 0.f;

	readHealthData(in, data->hp);

	turretData[id] = data;
	baedsLogger::log("Done.\n");
	return id;
}


static void loadAmmoData(WeaponData* data, gvReader& in)
{
	if (in.hasVal("maxAmmunition")) data->wepComp.maxAmmunition = in.getUint("maxAmmunition");
	if (in.hasVal("clip")) data->wepComp.maxClip = in.getUint("clip");
	if (in.hasVal("reloadTime")) data->wepComp.reloadTime = in.getFloat("reloadTime");
}

s32 loadWeaponData(std::string path, gvReader& in)
{
	baedsLogger::log("Reading weapon from " + path + "... ");
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return -1;
	}
	in.readLinesToValues();

	s32 id = std::stoi(in.values["id"]);
	std::string name = in.values["name"];
	WEAPON_TYPE type = weaponStrings.at(in.values["type"]);
	DAMAGE_TYPE dmgtype = damageStrings.at(in.values["dmgtype"]);
	WeaponData* data = new WeaponData;
	if (type == WEP_HEAVY_MISSILE) {
		delete data;
		data = new MissileData;
	}
	else if (type == WEP_PHYS_BOLAS) {
		delete data;
		data = new BolasData;
	}
	data->wepComp.fireSound = in.getString("fireSound");
	data->wepComp.impactSound = in.getString("impactSound");

	data->id = id;
	data->name = name;
	data->description = in.values["description"];

	std::string folder = in.getString("path");

	std::string meshpath = "assets/models/" + folder + in.getString("model");
	data->weaponMesh = meshpath;

	std::string materialPath = "assets/models/" + folder;

	u32 totalMaterials = 0;
	while (in.hasVal("material_" + std::to_string(totalMaterials))) {
		std::string mat = in.getString("material_" + std::to_string(totalMaterials));
		std::stringstream split(mat);
		_materialTexLayer layer;
		for (u32 i = 0; i < MATERIAL_MAX_TEXTURES; ++i) {
			std::string tex = "";
			std::getline(split, tex, ',');
			if (tex != "") {
				++layer.count;
				std::string texturePath = materialPath + tex;
				layer.tex[i] = texturePath;
			}
			else break;
		}
		data->materials.push_back(layer);
		++totalMaterials;
	}


	std::string effectpath = "assets/effects/";
	data->weaponEffect = effectpath + in.getString("particle");
	data->wepComp.usesAmmunition = false;
	data->wepComp.maxAmmunition = 0;
	//data->wepComp.ammunition = 0;
	data->wepComp.maxClip = 0;
	data->wepComp.reloadTime = 0;
	//data->wepComp.clip = 0;
	//data->wepComp.timeReloading = 0;

	if (in.hasVal("usesAmmo")) {
		data->wepComp.usesAmmunition = (in.getString("usesAmmo") == "yes") ? true : false;
		if(data->wepComp.usesAmmunition) loadAmmoData(data, in);
	}

	if (in.hasVal("hitScan")) data->wepComp.hitScan = (in.getString("hitScan") == "yes") ? true : false;

	if (type == WEP_HEAVY_MISSILE) {
		data->wepComp.usesAmmunition = true;
		std::string misspath = "assets/models/" + in.getString("missile_mesh");
		std::string misstexpath = "assets/models/" + in.getString("missile_tex");
		MissileData* miss = (MissileData*)data;

		miss->missileMesh = misspath;
		miss->missileTexture = misstexpath;

		miss->missThrust.forward = in.getFloat("forward");
		f32 rot = in.getFloat("turn");
		miss->missThrust.pitch = rot;
		miss->missThrust.yaw = rot;
		miss->missThrust.roll = rot;
		miss->missThrust.brake = miss->missThrust.forward * 2.4f;
		miss->missThrust.linearMaxVelocity = in.getFloat("maxVel");
		miss->missThrust.strafe = in.getFloat("strafe");

		miss->miss.timeToLock = in.getFloat("timeToLock");
		loadAmmoData(data, in);
	}
	if (type == WEP_KINETIC || type == WEP_HEAVY_RAILGUN || type == WEP_HEAVY_FLAMETHROWER) {
		data->wepComp.usesAmmunition = true;
		loadAmmoData(data, in);
	}

	if (in.hasVal("accuracy")) data->wepComp.accuracy = in.getFloat("accuracy");
	if (in.hasVal("projectilesPerShot")) {
		data->wepComp.projectilesPerShot = in.getInt("projectilesPerShot");
		if (data->wepComp.projectilesPerShot > MAX_PROJECTILES_PER_SHOT) data->wepComp.projectilesPerShot = MAX_PROJECTILES_PER_SHOT;
	}

	if (type == WEP_PHYS_BOLAS) {
		BolasInfoComponent cmp;
		//cmp.constraint = nullptr;
		cmp.currentDuration = 0.f;
		cmp.duration = in.getFloat("duration");
		cmp.timeToHit = in.getFloat("timeToHit");
		cmp.currentTimeToHit = 0.f;
		cmp.force = in.getFloat("force");
		BolasData* bdat = (BolasData*)data;
		cmp.latchSound = in.getString("latchSound");
		bdat->bolas = cmp;
	}

	//data->wepComp.isFiring = false;
	data->wepComp.type = type;
	data->wepComp.dmgtype = dmgtype;
	data->wepComp.firingSpeed = std::stof(in.values["firingSpeed"]);
	data->wepComp.projectileSpeed = std::stof(in.values["projectileSpeed"]);
	data->wepComp.damage = std::stof(in.values["damage"]);
	data->wepComp.lifetime = in.getFloat("lifetime");
	//data->wepComp.timeSinceLastShot = 0.f;
	data->wepComp.scale = in.getFloat("scale");
	data->wepComp.length = in.getFloat("length");
	data->wepComp.radius = in.getFloat("radius");

	vector3df fakeColor = in.getVec("flashColor"); //lol
	data->muzzleFlashColor = SColor(25, (u32)fakeColor.X, (u32)fakeColor.Y, (u32)fakeColor.Z);

	fakeColor = in.getVec("projectileLightColor");
	data->wepComp.projectileLightColor = SColor(150, (u32)fakeColor.X, (u32)fakeColor.Y, (u32)fakeColor.Z);

	data->wepComp.barrelStart = in.getVec("barrelStart");

	HARDPOINT_TYPE hrdtype = hardpointStrings.at(in.getString("hrdtype"));
	data->wepComp.hrdtype = hrdtype;
	data->wepComp.wepDataId = id;

	data->buildCost = in.getFloat("buildCost");
	data->canBuild = (in.getString("canBuild") == "yes") ? true : false;
	data->canLoot = (in.getString("canLoot") == "yes") ? true : false;

	data->wepComp.powerCost = in.getFloat("powerCost");
	data->wepComp.usesPower = (in.getString("usesPower") == "yes") ? true : false;

	if (hrdtype == HRDP_PHYSICS) {
		physWeaponData[id] = data;
	} else if (hrdtype == HRDP_REGULAR) {
		weaponData[id] = data;
	} else if (hrdtype == HRDP_HEAVY) {
		heavyWeaponData[id] = data;
	}

	std::string funcLine = in.getString("effects");
	std::stringstream split(funcLine);
	std::string token = "";
	while (std::getline(split, token, ',')) {
		data->wepComp.hitEffects.push_back(impactCbStrings.at(token));
	}
	funcLine = in.getString("updates");
	token = "";
	split = std::stringstream(funcLine);
	while (std::getline(split, token, ',')) {
		data->wepComp.updates.push_back(updateCbStrings.at(token));
	}

	data->wepComp.fire = fireCbStrings.at(in.getString("fireFunc"));

	baedsLogger::log("Done.\n");
	return id;
}

s32 loadObstacleData(std::string path, gvReader& in)
{
	baedsLogger::log("Reading in obstacle from " + path + "... ");
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return -1;
	}
	in.readLinesToValues();
	s32 id = std::stoi(in.values["id"]);
	std::string name = in.values["name"];

	ObstacleData* data = new ObstacleData;
	OBSTACLE type = obstacleStrings.at(in.values["type"]);

	if (type == SPACE_STATION) {
		baedsLogger::log("with station... ");
		delete data;
		auto sdata = new StationData;
		readTurretData(in, sdata->turretComponent);
		sdata->scale = in.getFloat("scale");
		data = sdata;
	}
	if (type == STATION_MODULE) {
		baedsLogger::log("with station module... ");
		delete data;
		auto sdata = new StationModuleData;
		sdata->hasTurrets = (in.getString("hasTurrets") == "yes") ? true : false;
		//sdata->moduleComp.powerGen = (in.getString("powerGen") == "yes") ? true : false;
		sdata->moduleComp.hasDock = (in.getString("hasDock") == "yes") ? true : false;

		if (in.hasVal("hasHangar")) {
			sdata->hasHangar = (in.getString("hasHangar") == "yes") ? true : false;
			sdata->hangarComp.reserveShips = in.getUint("reserveShips");
			sdata->hangarComp.launchRate = in.getUint("launchRate");
		}
		sdata->power.generator = (in.getString("powerGen") == "yes") ? true : false;
		sdata->power.isPowered = sdata->power.generator;
		sdata->power.maxPower = (sdata->power.generator) ? 10000.f : 0.f;
		sdata->power.power = sdata->power.maxPower;
		sdata->power.powerRegen = sdata->power.maxPower;

		//if (sdata->moduleComp.powerGen) sdata->moduleComp.isPowered = true;
		if(sdata->hasTurrets) readTurretData(in, sdata->turretComp);
		sdata->moduleComp.connections = in.getUint("connections");
		for (s32 i = 0; i < sdata->moduleComp.connections; ++i) {
			std::string pt = "connectionPoint" + std::to_string(i);
			std::string up = "connectionUp" + std::to_string(i);
			std::string dir = "connectionDirection" + std::to_string(i);
			sdata->moduleComp.connectionPoint[i] = in.getVec(pt);
			sdata->moduleComp.connectionUp[i] = in.getVec(up);
			sdata->moduleComp.connectionDirection[i] = in.getVec(dir);
		}
		for (const auto& val : stationPieceIds) {
			if (val.second == id) { 
				sdata->moduleComp.type = val.first; 
				break; 
			}
		}

		data = sdata;
	}
	data->id = id;
	data->name = name;
	data->type = type;

	readHealthData(in, data->hp);

	std::string folder = in.getString("path");

	std::string meshpath = "assets/models/" + folder + in.getString("model");

	data->obstacleMesh = meshpath;

	std::string materialPath = "assets/models/" + folder;

	u32 totalMaterials = 0;
	while (in.hasVal("material_" + std::to_string(totalMaterials))) {
		std::string mat = in.getString("material_" + std::to_string(totalMaterials));
		std::stringstream split(mat);
		_materialTexLayer layer;
		for (u32 i = 0; i < MATERIAL_MAX_TEXTURES; ++i) {
			std::string tex = "";
			std::getline(split, tex, ',');
			if (tex != "") {
				++layer.count;
				std::string texturePath = materialPath + tex;
				layer.tex[i] = texturePath;
			}
			else break;
		}
		data->materials.push_back(layer);
		++totalMaterials;
	}
	if (in.hasVal("texture")) data->flatTexture = "assets/" + in.getString("texture");

	obstacleData[id] = data;
	baedsLogger::log("Done.\n");
	return id;
}

s32 loadShipUpgradeData(std::string path, gvReader& in)
{
	baedsLogger::log("Reading ship upgrade from " + path + "... ");
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return -1;
	}
	in.readLinesToValues();

	ShipUpgradeData* up = new ShipUpgradeData;
	up->id = in.getInt("id");
	up->minSector = in.getInt("minSector");
	up->permanent = (in.getString("permanent") == "yes") ? true : false;
	up->canBuild = (in.getString("canBuild") == "yes") ? true : false;
	up->name = in.getString("name");
	up->description = in.getString("description");
	up->upgradeDescription = in.getString("upgrade_description");
	up->baseValue = in.getFloat("baseValue");
	up->scaleValue = in.getFloat("scaleValue");
	up->maxValue = in.getFloat("maxValue");
	up->baseCost = in.getFloat("baseCost");
	up->scaleCost = in.getFloat("scaleCost");

	std::string str = in.getString("type");
	SHIPUPGRADE_TYPE type = shipUpgradeStrings.at(in.getString("type"));
	up->type = type;

	shipUpgradeData[up->id] = up;
	baedsLogger::log("Done.\n");
	return up->id;
}

s32 loadWeaponUpgradeData(std::string path, gvReader& in)
{
	baedsLogger::log("Reading weapon upgrade from " + path + "... ");
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return -1;
	}
	in.readLinesToValues();

	WeaponUpgradeData* up = new WeaponUpgradeData;
	up->id = in.getInt("id");
	up->minSector = in.getInt("minSector");
	up->permanent = (in.getString("permanent") == "yes") ? true : false;
	up->canBuild = (in.getString("canBuild") == "yes") ? true : false;
	up->requiresAmmo = (in.getString("requiresAmmo") == "yes") ? true : false;
	up->name = in.getString("name");
	up->description = in.getString("description");
	up->upgradeDescription = in.getString("upgrade_description");
	up->baseValue = in.getFloat("baseValue");
	up->scaleValue = in.getFloat("scaleValue");
	up->maxValue = in.getFloat("maxValue");
	up->baseCost = in.getFloat("baseCost");
	up->scaleCost = in.getFloat("scaleCost");

	std::string str = in.getString("type");
	WEAPONUPGRADE_TYPE type = weaponUpgradeStrings.at(in.getString("type"));
	up->type = type;

	weaponUpgradeData[up->id] = up;
	baedsLogger::log("Done.\n");
	return up->id;
}

dataId loadWeaponArchetypeData(std::string path)
{
	baedsLogger::log("Reading weapon archetype from " + path + "... ");
	gvReader in;
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return -1;
	}
	in.readLinesToValues();
	WeaponArchetype* arch = new WeaponArchetype;
	arch->archetypeId = in.getInt("archetypeId");
	arch->wepId = in.getInt("wepId");
	for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
		std::string key = "wepUpgrade_" + std::to_string(i);
		if (in.hasVal(key)) {
			std::string up = in.getString(key);
			std::string type, value;
			std::stringstream split(up);
			std::getline(split, type, ',');
			std::getline(split, value, ',');
			arch->upgradeIds[i] = weaponUpgradeStrings.at(type);
			arch->wepUpValues[i] = std::stof(value);
		}
	}
	weaponArchetypeData[arch->archetypeId] = arch;
	baedsLogger::log("Done.\n");
	return arch->archetypeId;
}

dataId loadShipArchetypeData(std::string path)
{
	baedsLogger::log("Reading ship archetype from " + path + "... ");
	gvReader in;
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return INVALID_DATA_ID;
	}
	in.readLinesToValues();

	ShipArchetype* arch = new ShipArchetype();
	arch->archetypeId = in.getInt("archetypeId");
	arch->name = in.getString("name");
	arch->shipDataId = in.getInt("shipDataId");
	
	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		std::string key = "shipUpgrade_" + std::to_string(i);
		if (in.hasVal(key)) {
			std::string up = in.getString(key);
			std::string type, value;
			std::stringstream split(up);
			std::getline(split, type, ',');
			std::getline(split, value, ',');
			arch->upgradeIds[i] = shipUpgradeStrings.at(type);
			arch->upgradeValues[i] = std::stof(value);
		}
	}

	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		std::string archetypeKey = "wepArchetype_" + std::to_string(i);
		std::string idKey = "wepId_" + std::to_string(i);

		//check if it's using archetypes
		if (in.hasVal(archetypeKey)) {
			arch->usesWepArchetype[i] = true;
			arch->weps[i] = in.getInt(archetypeKey);
		}
		else if (in.hasVal(idKey)) arch->weps[i] = in.getInt(idKey);
	}

	std::string physKey = "physWepArchetype";
	std::string physIdKey = "physWepId";
	//hangar + turrets

	if (in.hasVal("hangarArchetypes")) {
		std::string vals = in.getString("hangarArchetypes");
		std::string archetype;
		std::stringstream split(vals);
		for (u32 i = 0; i < MAX_HANGAR_SHIPTYPES; ++i) {
			std::getline(split, archetype, ',');
			arch->hangarArchetypes[i] = std::stoi(archetype);
		}
	}

	for (u32 i = 0; i < MAX_TURRET_HARDPOINTS; ++i) {
		std::string archkey = "turretArchetype_" + std::to_string(i);
		std::string regkey = "turretId_" + std::to_string(i);
		std::string wepkey = "turretWepId_" + std::to_string(i);
		if (in.hasVal(archkey)) {
			arch->usesTurretArchetype[i] = true;
			arch->turretArchetypes[i] = in.getInt(archkey);
		}
		else if (in.hasVal(regkey)) {
			arch->turretArchetypes[i] = in.getInt(regkey);
			arch->turretWeps[i] = in.getInt(wepkey);
		}
	}
	shipArchetypeData[arch->archetypeId] = arch;
	baedsLogger::log("Done.\n");
	return arch->archetypeId;
}

dataId loadTurretArchetypeData(std::string path)
{
	baedsLogger::log("Reading turret archetype from " + path + "... ");
	gvReader in;
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return INVALID_DATA_ID;
	}

	in.readLinesToValues();
	TurretArchetype* arch = new TurretArchetype();

	arch->archetypeId = in.getInt("archetypeId");
	arch->name = in.getString("name");
	arch->turretDataId = in.getInt("turretDataId");

	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		std::string archetypeKey = "wepArchetype_" + std::to_string(i);
		std::string idKey = "wepId_" + std::to_string(i);

		//check if it's using archetypes
		if (in.hasVal(archetypeKey)) {
			arch->usesWepArchetype[i] = true;
			arch->weps[i] = in.getInt(archetypeKey);
		}
		else if (in.hasVal(idKey)) arch->weps[i] = in.getInt(idKey);
	}

	turretArchetypeData[arch->archetypeId] = arch;
	baedsLogger::log("Done.\n");
	return arch->archetypeId;
}

bool loadShip(u32 id, flecs::entity entity, vector3df pos, vector3df rot, bool initializeParticles)
{
	ShipData* data = shipData[id];
	//if (carrier) data = carrierData[id];

	if (!data) return false;

	IrrlichtComponent irr;
	entity.set_doc_name(data->name.c_str());

	irr.node = _loadObjModelData(data->shipMesh, data->materials, &irr);
	irr.topMesh = data->shipMesh;
	size_t ind = irr.topMesh.find_last_of(".");
	std::string lowmeshname = irr.topMesh.substr(0, ind) + "low.obj";
	if (device->getFileSystem()->existFile(lowmeshname.c_str())) irr.botMesh = lowmeshname;

	if (!initializeParticles) {
		for (u32 i = 0; i < irr.node->getMaterialCount(); ++i) {
			SMaterial& m = irr.node->getMaterial(i);
			m.setTexture(2, nullptr); //remove lightmaps
		}
	}
	//initialize toggles, fog, material type
	E_MATERIAL_TYPE materialType;
	if (!cfg->vid.toggles[TOG_BUMP]) materialType = EMT_SOLID;
	else materialType = shaders->getShaderMaterial(SHADE_8LIGHT_NORM);
	irr.node->setMaterialType(materialType);
	irr.baseMat = shaders->getShaderMaterial(SHADE_8LIGHT_NORM);

	if(!initializeParticles) irr.node->setMaterialType(shaders->getShaderMaterial(SHADE_2LIGHT_NORM));

	irr.node->setMaterialFlag(EMF_FOG_ENABLE, true);
	if (cfg->vid.toggles[TOG_FILTER]) irr.node->setMaterialFlag(EMF_TRILINEAR_FILTER, true);
	if (cfg->vid.toggles[TOG_ANISOTROPIC]) irr.node->setMaterialFlag(EMF_ANISOTROPIC_FILTER, true);
	if (cfg->vid.toggles[TOG_STENCILBUF]) {
		IMeshSceneNode* node = (IMeshSceneNode*)irr.node;
		auto shadow = node->addShadowVolumeSceneNode();
		shadow->setID(ID_IsNotSelectable);
	}

	//set up ECS stuff (the name of the node is the string corresponding to the entity)
	irr.node->setName(idToStr(entity).c_str());
	irr.node->setID(ID_IsSelectable | ID_IsAvoidable);
	irr.node->setPosition(pos);
	irr.node->setRotation(rot);
	irr.node->setScale(data->scale);

	if (data->hasHangar) {
		entity.set<HangarComponent>(data->hangar);
	}
	if (data->hasTurrets) {
		TurretHardpointComponent turrHards = data->turr;
		for (u32 i = 0; i < MAX_TURRET_HARDPOINTS; ++i) {
			//turrHards.turretConstraints[i] = nullptr;
			turrHards.turrets[i] = INVALID_ENTITY;
		}
		entity.set<TurretHardpointComponent>(turrHards);
	}

	HardpointComponent hards = data->hards;
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		hards.weapons[i] = INVALID_ENTITY;
	}

	entity.set<ShipComponent>(data->ship);
	entity.set<ThrustComponent>(data->thrust);
	entity.set<HardpointComponent>(hards);
	entity.set<IrrlichtComponent>(irr);
	entity.set<HealthComponent>(data->hp);
	entity.set<PowerComponent>(data->power);

	btVector3 scale(1.f, 1.f, 1.f);
	btScalar mass = 1.f;

	btConvexHullShape hull = shipData[id]->collisionShape;

	scale = irrVecToBt(data->scale);
	mass = data->mass;

	initializeBtConvexHull(entity, hull, scale, mass);
	if (initializeParticles) initializeShipParticles(entity);
	else entity.set<ShipParticleComponent>(ShipParticleComponent());
	if(data->hasHangar) gameController->registerDeathCallback(entity, carrierDeathExplosionCallback);
	else gameController->registerDeathCallback(entity, fighterDeathExplosionCallback);

	return true;
}

bool loadTurret(u32 id, flecs::entity entity)
{
	TurretData* data = turretData[id];
	if (!data) return false;

	IrrlichtComponent irr;
	entity.set_doc_name(data->name.c_str());

	irr.node = _loadObjModelData(data->mesh, data->materials, &irr);
	irr.topMesh = data->mesh;
	size_t ind = irr.topMesh.find_last_of(".");
	std::string lowmeshname = irr.topMesh.substr(0, ind) + "low.obj";
	if (device->getFileSystem()->existFile(lowmeshname.c_str())) irr.botMesh = lowmeshname;

	//initialize toggles, fog, material type
	E_MATERIAL_TYPE materialType;
	if (!cfg->vid.toggles[TOG_BUMP]) materialType = EMT_SOLID;
	else materialType = shaders->getShaderMaterial(SHADE_2LIGHT_NORM);
	irr.node->setMaterialType(materialType);
	irr.baseMat = shaders->getShaderMaterial(SHADE_2LIGHT_NORM);

	irr.node->setMaterialFlag(EMF_FOG_ENABLE, true);
	if (cfg->vid.toggles[TOG_FILTER]) irr.node->setMaterialFlag(EMF_TRILINEAR_FILTER, true);
	if (cfg->vid.toggles[TOG_ANISOTROPIC]) irr.node->setMaterialFlag(EMF_ANISOTROPIC_FILTER, true);

	//set ECS shit
	irr.node->setName(idToStr(entity).c_str());
	irr.node->setID(ID_IsSelectable | ID_IsAvoidable);

	entity.set<IrrlichtComponent>(irr);
	entity.set<ThrustComponent>(data->thrust);
	entity.set<HardpointComponent>(data->hards);
	entity.set<HealthComponent>(data->hp);
	return true;
}

bool loadWeapon(u32 id, flecs::entity weaponEntity, HARDPOINT_TYPE type)
{
	WeaponData* data = nullptr;

	if (type == HRDP_PHYSICS) data = physWeaponData[id];
	else if (type == HRDP_HEAVY) data = heavyWeaponData[id];
	else data = weaponData[id];

	if (!data) return false;

	IrrlichtComponent irr;
	weaponEntity.set_doc_name(data->name.c_str());

	irr.node = _loadObjModelData(data->weaponMesh, data->materials, &irr);
	irr.topMesh = data->weaponMesh;
	size_t ind = irr.topMesh.find_last_of(".");
	std::string lowmeshname = irr.topMesh.substr(0, ind) + "low.obj";
	if (device->getFileSystem()->existFile(lowmeshname.c_str())) irr.botMesh = lowmeshname;
	//initialize toggles, fog, material type
	E_MATERIAL_TYPE materialType;
	if (!cfg->vid.toggles[TOG_BUMP]) materialType = EMT_SOLID;
	else materialType = shaders->getShaderMaterial(SHADE_2LIGHT_NORM);
	irr.node->setMaterialType(materialType);
	irr.baseMat = shaders->getShaderMaterial(SHADE_2LIGHT_NORM);

	irr.node->setMaterialFlag(EMF_FOG_ENABLE, true);
	if (cfg->vid.toggles[TOG_FILTER]) irr.node->setMaterialFlag(EMF_TRILINEAR_FILTER, true);
	if (cfg->vid.toggles[TOG_ANISOTROPIC]) irr.node->setMaterialFlag(EMF_ANISOTROPIC_FILTER, true);

	irr.node->setName(idToStr(weaponEntity).c_str());
	irr.node->setID(ID_IsNotSelectable);

	WeaponInfoComponent wep = data->wepComp;
	WeaponFiringComponent fireComp;

	if (data->wepComp.type == WEP_HEAVY_MISSILE) {
		bool firstLoad = false;
		MissileData* mdata = (MissileData*)data;

		auto miss = mdata->miss;

		miss.missileMesh = assets->getMesh(mdata->missileMesh, firstLoad);
		miss.missileTexture = assets->getTexture(mdata->missileTexture);
		miss.missThrust = mdata->missThrust;

		weaponEntity.set<MissileInfoComponent>(miss);
	}

	fireComp.clip = wep.maxClip;

	if (data->wepComp.type == WEP_PHYS_BOLAS) {
		BolasData* bdata = (BolasData*)data;
		weaponEntity.set<BolasInfoComponent>(bdata->bolas);
		auto bolas = weaponEntity.get_mut<BolasInfoComponent>();
		BolasInfoComponent cmp = bdata->bolas;
		*bolas = cmp;
	}
	wep.particle = assets->getTexture(data->weaponEffect);

	fireComp.timeSinceLastShot = wep.firingSpeed;
	if (wep.usesAmmunition) {
		fireComp.clip = wep.maxClip;
		fireComp.timeReloading = wep.reloadTime;
		fireComp.ammunition = wep.maxAmmunition;
	}

	//particle FX!
	auto lightcol = data->muzzleFlashColor;
	lightcol.setAlpha(0);
	fireComp.muzzleFlash = createLightWithVolumeMesh(64, 8, 5.f, SColor(16,255,255,255), lightcol, irr.node, ID_IsNotSelectable, wep.barrelStart + vector3df(0, 0, 1.f));
	fireComp.muzzleFlash->setScale(vector3df(2.f, 2.f, 4.f));
	for (auto child : fireComp.muzzleFlash->getChildren()) {
		if (child->getType() == ESNT_LIGHT) fireComp.muzzleFlashLight = (ILightSceneNode*)child;
		break;
	}
	fireComp.muzzleFlashEmitter = smgr->addParticleSystemSceneNode(true, fireComp.muzzleFlash, ID_IsNotSelectable);
	auto em = fireComp.muzzleFlashEmitter->createMeshEmitter(fireComp.muzzleFlash->getMesh(), true, vector3df(0, .0005f, 0));
	em->setMinParticlesPerSecond(15);
	em->setMaxParticlesPerSecond(30);
	em->setMinLifeTime(80);
	em->setMaxLifeTime(320);
	em->setMinStartSize(dimension2df(.15f, .15f));
	em->setMaxStartSize(dimension2df(.35f, .35f));

	auto paf = fireComp.muzzleFlashEmitter->createFadeOutParticleAffector(SColor(0,0,0,0), 50);
	fireComp.muzzleFlashEmitter->addAffector(paf);
	paf->drop();
	fireComp.muzzleFlashEmitter->setEmitter(em);
	fireComp.muzzleFlashEmitter->setMaterialFlag(EMF_LIGHTING, false);
	fireComp.muzzleFlashEmitter->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
	fireComp.muzzleFlashEmitter->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);
	fireComp.muzzleFlashEmitter->setMaterialTexture(0, wep.particle);
	fireComp.muzzleFlash->setMaterialFlag(EMF_LIGHTING, false);
	fireComp.muzzleFlash->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
	em->drop();

	//wep.muzzleFlashEmitter->setVisible(false);
	fireComp.muzzleFlash->setVisible(false);

	weaponEntity.set<WeaponInfoComponent>(wep);
	weaponEntity.set<WeaponFiringComponent>(fireComp);
	weaponEntity.set<IrrlichtComponent>(irr);
	return true; 
}

void preloadShip(s32 id)
{
	ShipData* data = shipData[id];
	_loadObjModelData(data->shipMesh, data->materials, nullptr, true);
}
void preloadWep(s32 id)
{
	WeaponData* data = weaponData[id];
	_loadObjModelData(data->weaponMesh, data->materials, nullptr, true);
}
void preloadTurret(s32 id)
{
	TurretData* data = turretData[id];
	_loadObjModelData(data->mesh, data->materials, nullptr, true);
}

bool loadObstacle(u32 id, flecs::entity entity)
{
	ObstacleData* data = obstacleData[id];
	if (!data) return false;
	ObstacleComponent obst;
	obst.type = data->type;

	IrrlichtComponent irr;
	entity.set_doc_name(data->name.c_str());

	bool nofog = (obst.type == GAS_CLOUD || obst.type == MESH_GASCLOUD || obst.type == FLAT_BILLBOARD || obst.type == FLAT_BILLBOARD_ANIMATED);

	if (obst.type == GAS_CLOUD) { //gas clouds get special treatment!
		auto ps = smgr->addParticleSystemSceneNode();
		irr.node = ps;
		IParticleAffector* paf = ps->createFadeOutParticleAffector(SColor(0,0,0,0));
		ps->addAffector(paf);
		paf->drop();
		paf = new CParticleFadeInAffector(SColor(0, 0, 0, 0), 1000);
		ps->addAffector(paf);
		paf->drop();
		irr.node->setMaterialFlag(EMF_FOG_ENABLE, false);

		ps->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);
		ps->setMaterialFlag(EMF_LIGHTING, false);
		ps->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
		ps->getMaterial(0).BlendOperation = EBO_ADD;
		auto animator = getGasTextureAnim(data->flatTexture);
		ps->addAnimator(animator);
		animator->drop();
		entity.add<ObstacleDoesNotCollide>();
	}
	else if (obst.type == MESH_GASCLOUD) { //so does the other type of weird-assed gas cloud!
		irr.node = createCloudMeshNode(data->obstacleMesh, 64, SColor(20, 150, 150, 150), SColor(1, 100, 94, 64));
		entity.add<ObstacleDoesNotCollide>();
	}
	else if (obst.type == FLAT_BILLBOARD || obst.type == FLAT_BILLBOARD_ANIMATED) {
		irr.node = smgr->addBillboardSceneNode();
		entity.add<ObstacleDoesNotCollide>();
		irr.node->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
		irr.node->setMaterialFlag(EMF_LIGHTING, false);
		irr.node->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
		if (obst.type == FLAT_BILLBOARD) {
			irr.node->setMaterialTexture(0, assets->getTexture(data->flatTexture));
		}
		else {
			auto anim = getTextureAnim(data->flatTexture, 18, true);
			irr.node->addAnimator(anim);
			anim->drop();
		}
	}

	else {
		irr.node = _loadObjModelData(data->obstacleMesh, data->materials, &irr);
		irr.topMesh = data->obstacleMesh;
		size_t ind = irr.topMesh.find_last_of(".");
		std::string lowmeshname = irr.topMesh.substr(0, ind) + "low.obj";
		if (device->getFileSystem()->existFile(lowmeshname.c_str())) irr.botMesh = lowmeshname;
	}

	//initialize toggles, fog, material type
	if (!cfg->vid.toggles[TOG_BUMP] && !nofog) irr.node->setMaterialType(EMT_SOLID);
	else if (!nofog) {
		irr.node->setMaterialType(shaders->getShaderMaterial(SHADE_3LIGHT_NORM));
		irr.baseMat = shaders->getShaderMaterial(SHADE_3LIGHT_NORM);
	}

	irr.node->setName(idToStr(entity).c_str());
	if (cfg->vid.toggles[TOG_FILTER]) irr.node->setMaterialFlag(EMF_TRILINEAR_FILTER, true);
	if (cfg->vid.toggles[TOG_ANISOTROPIC]) irr.node->setMaterialFlag(EMF_ANISOTROPIC_FILTER, true);
	if (cfg->vid.toggles[TOG_STENCILBUF] && !nofog) {
		IMeshSceneNode* node = (IMeshSceneNode*)irr.node;
		node->addShadowVolumeSceneNode();
	}

	if(!nofog) irr.node->setMaterialFlag(EMF_FOG_ENABLE, true); //todo: gas shader that accounts for fog

	if (data->type == SPACE_STATION) {
		auto sdata = (StationData*)data;
		entity.set<TurretHardpointComponent>(sdata->turretComponent);
	}
	if (data->type == STATION_MODULE) {
		auto sdata = (StationModuleData*)data;
		if(sdata->hasTurrets) entity.set<TurretHardpointComponent>(sdata->turretComp);
		if (sdata->hasHangar) entity.set<HangarComponent>(sdata->hangarComp);
		entity.set<StationModuleComponent>(sdata->moduleComp);
		entity.set<PowerComponent>(sdata->power);
	}

	entity.set<ObstacleComponent>(obst);
	entity.set<IrrlichtComponent>(irr);
	entity.set<HealthComponent>(data->hp);
	return true;
}
btConvexHullShape createCollisionShapeFromMesh(IMesh* mesh)
{
	btConvexHullShape shape;

	for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i) {
		IMeshBuffer* buf = mesh->getMeshBuffer(i);
		S3DVertex* bufverts = (S3DVertex*)buf->getVertices();
		for (u32 j = 0; j < buf->getVertexCount(); ++j) {
			vector3df pos = bufverts[j].Pos;
			shape.addPoint(btVector3(pos.X, pos.Y, pos.Z));
		}
	}
	shape.setMargin(0);
	btShapeHull* hull = new btShapeHull(&shape);
	hull->buildHull(0);
	btConvexHullShape ret((const btScalar*)hull->getVertexPointer(), hull->numVertices(), sizeof(btVector3));
	delete hull;
	return ret;
}

btConvexHullShape createCollisionShapeFromColliderMesh(IMesh* mesh)
{
	btConvexHullShape shape;
	for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i) {
		IMeshBuffer* buf = mesh->getMeshBuffer(i);
		S3DVertex* bufverts = (S3DVertex*)buf->getVertices();
		for (u32 j = 0; j < buf->getVertexCount(); ++j) {
			vector3df pos = bufverts[j].Pos;
			shape.addPoint(btVector3(pos.X, pos.Y, pos.Z));
		}
	}
	shape.setMargin(0);
	return shape;
}

bool saveHull(std::string path, btConvexHullShape& shape)
{
	btDefaultSerializer serializer;
	serializer.startSerialization();
	shape.serializeSingleShape(&serializer);
	serializer.finishSerialization();

	FILE* f = fopen(path.c_str(), "wb");
	if (!f) return false;

	fwrite(serializer.getBufferPointer(), serializer.getCurrentBufferSize(), 1, f);
	fclose(f);
	return true;
}
bool loadHull(std::string path, btConvexHullShape& shape)
{
	btBulletWorldImporter importer(0);
	if (!importer.loadFile(path.c_str())) {
		baedsLogger::errLog("Could not load hull file!\n");
		return false;
	}
	if (!importer.getNumCollisionShapes()) {
		baedsLogger::errLog("Hull file not formatted correctly!\n");
		return false;
	}
	btConvexHullShape* coll = (btConvexHullShape*)importer.getCollisionShapeByIndex(0);
	shape = *coll;

	return true;
	
}

WingmanInstance* loadWingman(const WingmanMarker* marker, bool setFlag)
{
	baedsLogger::log("Reading wingman data from " + marker->path + "... ");
	gvReader in;
	in.read(marker->path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return nullptr;
	}
	WingmanInstance* inst = new WingmanInstance();
	in.readLinesToValues();
	inst->id = in.getInt("id");
	inst->description = in.getString("description");
	inst->name = in.getString("name");
	inst->personality = in.getString("personality");

	inst->ai.aggressiveness = in.getFloat("aggressiveness");
	inst->ai.behaviorFlags = in.getUint("behaviors");
	inst->ai.aim = in.getFloat("aim");
	inst->ai.resolve = in.getFloat("resolve");
	inst->ai.reactionSpeed = in.getFloat("reactionSpeed");

	//inst->flag = wstr(in.getString("flag"));

	inst->attackLine = in.getString("attackLine");
	inst->formUpLine = in.getString("formUpLine");
	inst->deathLine = in.getString("deathLine");
	inst->killLine = in.getString("killLine");
	inst->negLine = in.getString("negLine");
	inst->haltLine = in.getString("haltLine");
	inst->dockLine = in.getString("dockLine");
	inst->disengageLine = in.getString("disengageLine");
	inst->helpLine = in.getString("helpLine");

	inst->bustTotalLine = in.getString("bustTotalLine");
	inst->getBackInsideLine = in.getString("getBackInsideLine");
	inst->bustAmbushLine = in.getString("bustAmbushLine");
	inst->bustSalvageLine = in.getString("bustSalvageLine");
	inst->missionAccomplishedLine = in.getString("missionAccomplishedLine");

	if (setFlag) campaign->setFlag(marker->flag);
	baedsLogger::log("Done.\n");
	return inst;
}

bool loadWingmanMarker(std::string path)
{
	baedsLogger::log("Reading wingman marker from " + path + "... ");
	gvReader in;
	in.read(path);
	if (in.lines.empty()) {
		baedsLogger::log("could not read!\n");
		return false;
	}
	in.readLinesToValues();
	s32 id = in.getInt("id");
	auto mark = new WingmanMarker;
	mark->id = id;
	mark->minSector = in.getInt("minSector");
	mark->flag = wstr(in.getString("flag"));
	mark->path = path;
	wingMarkers[id] = mark;
	baedsLogger::log("Done.\n");
	return true;
}