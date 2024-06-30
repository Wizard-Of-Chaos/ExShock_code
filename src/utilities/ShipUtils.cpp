#include "ShipUtils.h"
#include "GameController.h"
#include "GameStateController.h"
#include "SensorComponent.h"
#include "StatTrackerComponent.h"
#include "ShipParticleComponent.h"
#include "AITypes.h"
#include "NodeAnimators.h"
#include "TurretUtils.h"
#include "Archetypes.h"
#include "AttributeReaders.h"
#include "Config.h"
#include "GameAssets.h"
#include "Campaign.h"
#include "AudioDriver.h"
#include "Shaders.h"
#include "GameFunctions.h"
#include "CrashLogger.h"
#include "MapRunner.h"
#include "NetworkingComponent.h"
#include "StatusEffects.h"

bool initializeWeaponFromId(u32 id, flecs::entity shipId, int hardpoint, HARDPOINT_TYPE type, NetworkId networkId)
{
	if (id <= 0) {
		return false;
	}
	if (!shipId.has<HardpointComponent>() || !shipId.has<IrrlichtComponent>()) return false;
	if (!shipId.is_alive()) return false;

	auto shipIrr = shipId.get<IrrlichtComponent>();
	auto hards = shipId.get_mut<HardpointComponent>();

	flecs::entity wepEntity = INVALID_ENTITY;
	wepEntity = game_world->entity();

	wepEntity.add(flecs::ChildOf, shipId);
	PowerComponent dummy;
	dummy.isPowered = true;
	dummy.receivingPowerFrom = shipId;
	wepEntity.set<PowerComponent>(dummy);

	vector3df scale = shipIrr->node->getScale();
	loadWeapon(id, wepEntity, type);
	auto irr = wepEntity.get_mut<IrrlichtComponent>();
	irr->node->setParent(shipIrr->node);
	if (type == HRDP_HEAVY) {
		irr->node->setPosition(hards->heavyWeaponHardpoint);
		hards->heavyWeapon = wepEntity;
		irr->node->setRotation(hards->heavyWeaponRot);
	}
	else if (type == HRDP_PHYSICS) {
		irr->node->setPosition(hards->physWeaponHardpoint);
		hards->physWeapon = wepEntity;
		irr->node->setRotation(hards->physHardpointRot);

	}
	else {
		irr->node->setPosition(hards->hardpoints[hardpoint]);
		hards->weapons[hardpoint] = wepEntity;
		irr->node->setRotation(hards->hardpointRotations[hardpoint]);

	}
	irr->node->setScale(vector3df(.5f, .5f, .5f));
	irr->node->updateAbsolutePosition();
	initializeNetworkingComponent(wepEntity, 2U, networkId);
	return true;
}

bool initializeWeaponfromArchetype(dataId archetype, flecs::entity shipId, int hardpoint, HARDPOINT_TYPE type)
{
	auto arch = weaponArchetypeData[archetype];
	auto wep = initializeWeaponFromId(arch->archetypeId, shipId, hardpoint, type);
	//TODO: upgrade the weapon pending rewrites
	return wep;
}


flecs::entity createShip(u32 shipDataId, u32 wepDataId, vector3df pos, vector3df rot, bool initializeParticles, bool isPlayer)
{
	flecs::entity ship = game_world->entity();
	if (isPlayer) gameController->setPlayer(ship);
	loadShip(shipDataId, ship, pos, rot, initializeParticles);
	if (ship.has<HardpointComponent>()) {

		auto hards = ship.get<HardpointComponent>();
		for (unsigned int i = 0; i < hards->hardpointCount; ++i) {
			initializeWeaponFromId(wepDataId, ship, i);
		}
	}
#ifdef _DEBUG
	else {
		baedsLogger::errLog("Ship " + entDebugStr(ship) + " spawned without hardpoint component.\n");
	}
#endif
	return ship;
}

void initializeRegularWeapons(flecs::entity id, dataId wepDataId)
{
	if (!id.has<HardpointComponent>()) return;

	auto hards = id.get<HardpointComponent>();
	for (u32 i = 0; i < hards->hardpointCount; ++i) {
		initializeWeaponFromId(wepDataId, id, i);
	}
}

void setGunship(flecs::entity ship, s32 turretId, s32 turretWepId)
{
	auto shipcmp = ship.get<ShipComponent>();
	if (!shipData[shipcmp->shipDataId]->hasTurrets) return;
	initializeTurretsOnOwner(ship, turretId, turretWepId);
}

void setArtillery(flecs::entity ship)
{
	auto ai = ship.get_mut<AIComponent>();
	ai->setBehavior(AIBHVR_STAY_AT_MAX_RANGE);
	ai->targetingAudio = "targeted_sniper.ogg";
	ai->aim += .15f;
}

flecs::entity createFriendlyShip(u32 shipDataId, u32 wepDataId, vector3df pos, vector3df rot, bool decloaking)
{
	flecs::entity ship = createShip(shipDataId, wepDataId, pos, rot);
	if (!cfg->game.toggles[GTOG_IMPACT]) {
		auto hp = ship.get_mut<HealthComponent>();
		hp->healthResistances[IMPACT] = 1.f;
	}
	initializePlayerFaction(ship);
	initializeDefaultSensors(ship);
	setDamageDifficulty(ship);
	if (decloaking) {
		auto irr = ship.get_mut<IrrlichtComponent>();
		auto anim = new CSceneNodeFadeInAnimator(device->getTimer()->getTime(), 3500, smgr);
		irr->node->addAnimator(anim);
		anim->drop();
		decloakEffect(pos, 150.f);
	}
	return ship;
}

flecs::entity createHostileShip(u32 shipDataId, u32 wepDataId, vector3df pos, vector3df rot, bool decloaking)
{
	flecs::entity ship = createShip(shipDataId, wepDataId, pos, rot);
	initializeHostileFaction(ship);
	initializeDefaultSensors(ship);
	setDamageDifficulty(ship);
	if (decloaking) {
		auto irr = ship.get_mut<IrrlichtComponent>();
		auto anim = new CSceneNodeFadeInAnimator(device->getTimer()->getTime(), 3500, smgr);
		irr->node->addAnimator(anim);
		anim->drop();
		decloakEffect(pos, 150.f);
	}
	return ship;
}

flecs::entity loadShipFromArchetype(dataId archetypeId, vector3df pos, vector3df rot, FACTION_TYPE fac, bool decloaking)
{
	auto arch = shipArchetypeData[archetypeId];
	auto sdata = shipData[arch->shipDataId];
	flecs::entity ship = game_world->entity();
	loadShip(arch->shipDataId, ship, pos, rot);

	ship.set_doc_name(arch->name.c_str());
	initializeDefaultSensors(ship);
	initializeFaction(ship, fac);

	for (u32 i = 0; i < sdata->hards.hardpointCount; ++i) {
		if (arch->weps[i] == INVALID_DATA_ID) continue;
		if (arch->usesWepArchetype[i] == false) initializeWeaponFromId(arch->weps[i], ship, i);
		else initializeWeaponfromArchetype(arch->weps[i], ship, i);
	}

	if (arch->heavyWep != INVALID_DATA_ID) {
		if (arch->usesHeavyArchetype == false) initializeWeaponFromId(arch->heavyWep, ship, HEAVY_HARDPOINT, HRDP_HEAVY);
		else initializeWeaponfromArchetype(arch->heavyWep, ship, HEAVY_HARDPOINT, HRDP_HEAVY);
	}
	if (arch->physWep != INVALID_DATA_ID) {
		if (arch->usesPhysArchetype == false) initializeWeaponFromId(arch->physWep, ship, PHYS_HARDPOINT, HRDP_PHYSICS);
		else initializeWeaponfromArchetype(arch->physWep, ship, PHYS_HARDPOINT, HRDP_PHYSICS);
	}

	if (decloaking) {
		auto irr = ship.get_mut<IrrlichtComponent>();
		auto anim = new CSceneNodeFadeInAnimator(device->getTimer()->getTime(), 3500, smgr);
		irr->node->addAnimator(anim);
		anim->drop();
		decloakEffect(pos, 150.f);
	}
	setDamageDifficulty(ship);
	if (sdata->artilleryShip) setArtillery(ship);
	if (sdata->hasTurrets) initializeTurretsOnOwnerFromArchetypes(ship, arch);

	//TODO: hangars
	//TODO: ship / wep ugrades, pending a rewrite of that system

	return ship;
}

void initializeFaction(flecs::entity id, FACTION_TYPE type, u32 hostiles, u32 friendlies, bool important)
{
	FactionComponent fac;
	setFaction(&fac, type, hostiles, friendlies, important);
	id.set<FactionComponent>(fac);
}

void initializeFaction(flecs::entity id, FACTION_TYPE type, bool important) 
{
	switch (type) {
	case FACTION_HOSTILE:
		initializeHostileFaction(id, important);
		break;
	case FACTION_NEUTRAL:
		initializeNeutralFaction(id, important);
		break;
	case FACTION_PLAYER:
		initializePlayerFaction(id, important);
		break;
	case FACTION_UNCONTROLLED:
		initializeFaction(id, FACTION_UNCONTROLLED, (FACTION_HOSTILE | FACTION_PLAYER), FACTION_NEUTRAL, important);
		break;
	default:
		initializeNeutralFaction(id, false);
		break;
	}
}

void setFaction(FactionComponent* fac, FACTION_TYPE type, unsigned int hostilities, unsigned int friendlies, bool important)
{
	fac->type = type;
	fac->hostileTo = hostilities;
	fac->friendlyTo = friendlies;
	fac->isImportant = important;
}

void initializeNeutralFaction(flecs::entity id, bool important)
{
	initializeFaction(id, FACTION_NEUTRAL, 0, 0, important);
}
void initializeHostileFaction(flecs::entity id, bool important)
{
	initializeFaction(id, FACTION_HOSTILE, FACTION_PLAYER | FACTION_UNCONTROLLED, FACTION_HOSTILE, important);
}

void initializePlayerFaction(flecs::entity id, bool important)
{
	initializeFaction(id, FACTION_PLAYER, FACTION_HOSTILE | FACTION_UNCONTROLLED, FACTION_PLAYER, important);
}

bool initializeSensors(flecs::entity id, f32 range, f32 updateInterval, bool onlyShips)
{
	//Faction component is REQUIRED for sensors (the one for hostiles, the other for positioning).
	//if (!id.has<FactionComponent>()) return false;
	SensorComponent sensors;
	sensors.detectionRadius = range;

	sensors.closestContact = INVALID_ENTITY; 
	sensors.closestFriendlyContact = INVALID_ENTITY;
	sensors.closestHostileContact = INVALID_ENTITY;
	sensors.targetContact = INVALID_ENTITY;
	sensors.timeSelected = 0;

	sensors.onlyDetectShips = onlyShips;

	sensors.updateInterval = updateInterval;
	f32 start = random.frange(0.f, updateInterval);
	sensors.timeSinceLastUpdate = start; //stagger sensor updates so it doesn't all happen at once

	id.set<SensorComponent>(sensors);
	return true;
}
bool initializeDefaultSensors(flecs::entity id)
{
	return initializeSensors(id, DEFAULT_SENSOR_RANGE, DEFAULT_SENSOR_UPDATE_INTERVAL);
}

void initializeShields(flecs::entity id, f32 amount, f32 delay, f32 recharge)
{
	auto hp = id.get_mut<HealthComponent>();
	hp->shields = amount;
	hp->maxShields = amount;
	hp->rechargeDelay = delay;
	hp->rechargeRate = recharge;
	hp->timeSinceLastHit = hp->rechargeDelay;
}
void initializeDefaultShields(flecs::entity objectId)
{
	initializeShields(objectId, DEFAULT_MAX_SHIELDS, DEFAULT_RECHARGE_DELAY, DEFAULT_RECHARGE_RATE);
}

IParticleSystemSceneNode* _createShipJet(ISceneNode* node, vector3df pos, vector3df dir)
{

	IParticleSystemSceneNode* ps = smgr->addParticleSystemSceneNode(true, node);
	ps->setID(ID_IsNotSelectable);
	IParticleEmitter* em = ps->createSphereEmitter(
		vector3df(0,0,0), .1f, dir * .02f,
		0, 0, SColor(255, 180, 180, 180), SColor(255, 210, 210, 210),
		50, 200, 2, dimension2df(.2f, .2f), dimension2df(.35f, .35f)
	);
	ps->setEmitter(em);
	em->drop();

	IParticleAffector* paf = ps->createFadeOutParticleAffector();
	ps->addAffector(paf);
	paf->drop();
	ps->setMaterialFlag(EMF_LIGHTING, false);
	ps->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
	ps->setMaterialTexture(0, assets->getTexture("assets/effects/smokejet.png"));
	ps->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);

	return ps;
}
IParticleSystemSceneNode* _createFlamePoint(IMeshSceneNode* node, f32 scaleMin, f32 scaleMax) 
{
	auto par = smgr->addParticleSystemSceneNode();
	par->setParent(node);

	auto buf = node->getMesh()->getMeshBuffer(random.unumEx(node->getMesh()->getMeshBufferCount()));
	S3DVertexTangents* verts = (S3DVertexTangents*)node->getMesh()->getMeshBuffer(random.unumEx(node->getMesh()->getMeshBufferCount()))->getVertices();
	if (buf->getVertexCount() <= 0) {
		S3DVertexTangents vert = verts[random.unumEx(buf->getVertexCount())];
		vector3df pos = vert.Pos;
		par->setPosition(pos);
	}

	f32 scale = 1.f;
	vector3df scaleVec = par->getScale();
	scale = (scaleVec.X + scaleVec.Y + scaleVec.Z) / 3.f;
	u32 lvl = (u32)cfg->vid.particleLevel + 1;
	auto emit = par->createPointEmitter(vector3df(0.f, 0.0005f, 0.f), 12 * lvl, 30 * lvl,
		SColor(255, 255, 255, 255), SColor(255, 255, 255, 255), 600, 1200, 360, dimension2df(scaleMin * scale, scaleMin * scale), dimension2df(scaleMax * scale, scaleMax * scale));
	par->setEmitter(emit);
	emit->drop();
	auto paf = par->createFadeOutParticleAffector();
	par->addAffector(paf);
	paf->drop();
	par->setMaterialFlag(EMF_LIGHTING, false);
	par->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
	par->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);
	return par;
}
void initializeShipParticles(flecs::entity id)
{
	ShipParticleComponent& parc = *id.get_mut<ShipParticleComponent>();
	auto ship = id.get<ShipComponent>();
	auto irr = id.get<IrrlichtComponent>();
	if (!irr) return; //huh?
	for (u32 i = 0; i < 2; ++i) {
		parc.upJetEmit[i] = _createShipJet(irr->node, ship->upJetPos[i], getNodeUp(irr->node));
		parc.downJetEmit[i] = _createShipJet(irr->node, ship->downJetPos[i], getNodeDown(irr->node));
		parc.leftJetEmit[i] = _createShipJet(irr->node, ship->leftJetPos[i], getNodeLeft(irr->node));
		parc.rightJetEmit[i] = _createShipJet(irr->node, ship->rightJetPos[i], getNodeRight(irr->node));
		parc.reverseJetEmit[i] = _createShipJet(irr->node, ship->reverseJetPos[i], getNodeForward(irr->node));
	}
	
	for (u32 i = 0; i < 4; ++i) {
		if (!irr->node) continue;
		IParticleSystemSceneNode* par = _createFlamePoint((IMeshSceneNode*)irr->node, 5.f, 9.f);
		parc.flamePoints[i] = par;
		ISceneNodeAnimator* texanim = getFireTextureAnim();
		par->addAnimator(texanim);
		texanim->drop();
		par->setVisible(false);

		par = _createFlamePoint((IMeshSceneNode*)irr->node, 1.8f, 2.8f);
		parc.smokePoints[i] = par;
		texanim = getSmokeTextureAnim();
		par->addAnimator(texanim);
		texanim->drop();
		par->setVisible(false);

	}
	ShipData* dat = shipData.at(ship->shipDataId);

	bool firstload = false;
	if (!shipData.at(ship->shipDataId)->hasTurrets) {
		parc.shield = smgr->addMeshSceneNode(assets->getMesh(dat->shipMesh, firstload), irr->node, ID_IsNotSelectable, vector3df(0), vector3df(0), vector3df(1.175f));
		parc.shield->setMaterialType(EMT_TRANSPARENT_VERTEX_ALPHA);
		parc.shield->setMaterialFlag(EMF_LIGHTING, false);

		for (u32 i = 0; i < parc.shield->getMesh()->getMeshBufferCount(); ++i) {
			auto manip = smgr->getMeshManipulator();
			manip->setVertexColors(parc.shield->getMesh()->getMeshBuffer(i), dat->shieldColor);
			manip->setVertexColorAlpha(parc.shield->getMesh()->getMeshBuffer(i), 0);
		}
	}
	for (u32 i = 0; i < dat->materials.size(); ++i) {

		SMaterial& mat = irr->node->getMaterial(i);
		auto& layer = dat->materials[i];
		ITexture* tex = assets->getTexture(layer.tex[2]);
		if (tex) mat.setTexture(2, tex);

	}

	for (u32 i = 0; i < ship->engineCount; ++i) {
		parc.engineJetEmit[i] = smgr->addVolumeLightSceneNode(irr->node,
			ID_IsNotSelectable, 256, 256, SColor(255, 255, 255, 255), SColor(0, 0, 0, 0), ship->engineJetPos[i],
			vector3df(-90, 0, 0), vector3df(2, 1, 2));

		ISceneNodeAnimator* glowie = getTextureAnim(dat->engineTexture, 30, true);
		parc.engineJetEmit[i]->addAnimator(glowie);
		glowie->drop(); //Terry would be proud

		parc.engineJetEmit[i]->setMaterialFlag(EMF_LIGHTING, false);
		parc.engineJetEmit[i]->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
		parc.engineJetEmit[i]->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);

		auto engine = smgr->addLightSceneNode(irr->node, ship->engineJetPos[i], SColorf(0.f, 1.f, 0.f), 1.3f);
		parc.engineLight[i] = engine;
		engine->setID(ID_IsNotSelectable);

		if (id != gameController->getPlayer()) {

			auto trail = addMotionTrailTimeBased(parc.engineJetEmit[i], 32, 500, true);
			trail->getMaterial(0).Lighting = false;
			trail->getMaterial(0).MaterialType = EMT_TRANSPARENT_ALPHA_CHANNEL;
			trail->setVertexColors(
				dat->engineTipCenter, //tip center
				dat->engineTipEdge, //tip edge
				dat->engineEndCenter, //end center
				dat->engineEndEdge); //end edge
			trail->setWidth(8.f);
			trail->setShrinkDirection(false, true);
		}
	}
	//id.set<ShipParticleComponent>(parc);
}

flecs::entity createShipFromInstance(ShipInstance& inst, vector3df pos, vector3df rot, bool carrier, bool player)
{
	flecs::entity id = game_world->entity();
	if (player) gameController->setPlayer(id);
	loadShip(inst.ship.shipDataId, id, pos, rot);
	if (id == INVALID_ENTITY) return id;

	HardpointComponent* hards = id.get_mut<HardpointComponent>();
	for (u32 i = 0; i < hards->hardpointCount; ++i) {
		if (!carrier) {
			if (inst.weps[i] <= -1) continue;
			WeaponInstance* wepInst = campaign->getWeapon(inst.weps[i]);
			WeaponInfoComponent wepReplace = wepInst->wep;
			WeaponFiringComponent wepFire = wepInst->fire;
			initializeWeaponFromId(wepReplace.wepDataId, id, i);
			const WeaponInfoComponent* wep = hards->weapons[i].get<WeaponInfoComponent>();
			WeaponFiringComponent* fire = hards->weapons[i].get_mut<WeaponFiringComponent>();
			if (wepReplace.type != WEP_NONE && wep) {
				fire->ammunition = wepFire.ammunition;
			}
			for (u32 j = 0; j < MAX_WEP_UPGRADES; ++j) {
				if (wepInst->upgrades[j] > -1) campaign->getWeaponUpgrade(wepInst->upgrades[j])->upgrade(hards->weapons[i]);
			}
		}
		else {
			initializeWeaponFromId(inst.weps[i], id, i);
		}
	}
	HealthComponent* hp = id.get_mut<HealthComponent>();
	*hp = inst.hp;
	if (inst.physWep > -1) {
		if (!carrier) initializeWeaponFromId(campaign->getPhysWeapon(inst.physWep)->wep.wepDataId, id, 0, HRDP_PHYSICS);
		else initializeWeaponFromId(inst.physWep, id, 0, HRDP_PHYSICS);
	}
	if (inst.heavyWep > -1) {
		if(!carrier) initializeWeaponFromId(campaign->getHeavyWeapon(inst.heavyWep)->wep.wepDataId, id, 0, HRDP_HEAVY);
		else initializeWeaponFromId(inst.physWep, id, 0, HRDP_HEAVY);

	}

	return id;
}

flecs::entity hangarLaunchShip(ShipInstance& inst, vector3df spawnPos, vector3df spawnRot, FactionComponent* carrFac)
{
	auto id = createShipFromInstance(inst, spawnPos, spawnPos, true);
	if (id == INVALID_ENTITY) return id;
	id.set<FactionComponent>(*carrFac);
	initializeDefaultAI(id);
	initializeDefaultSensors(id);
	if (carrFac->type == FACTION_PLAYER && !cfg->game.toggles[GTOG_IMPACT]) {
		auto hp = id.get_mut<HealthComponent>();
		hp->healthResistances[IMPACT] = 1.f;
	}
	return id;
}

flecs::entity createPlayerShip(vector3df pos, vector3df rot)
{
	flecs::entity id = INVALID_ENTITY;
	if (stateController->inCampaign) {
		ShipInstance* inst = campaign->getPlayerShip();
		id = createShipFromInstance(*inst, pos, rot, false, true);
	}
	else {
		id = createShip(0, 3, pos, rot, true, true);
	}
	gameController->setPlayer(id);

	if (!cfg->game.toggles[GTOG_IMPACT]) {
		auto hp = id.get_mut<HealthComponent>();
		hp->healthResistances[IMPACT] = 1.f;
	}

	initializeDefaultPlayer(id);
	initializePlayerFaction(id);
	initializeSensors(id, 1500.f, .4f);
	id.set<StatTrackerComponent>(StatTrackerComponent());
	setDamageDifficulty(id);
	IMeshSceneNode* node = (IMeshSceneNode*)id.get_mut<IrrlichtComponent>()->node;
	node->setMaterialType(shaders->getShaderMaterial(SHADE_8LIGHT_NORM));
	gameController->registerDeathCallback(id, fighterDeathExplosionCallback);
	if (stateController->inCampaign) {
		for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
			ShipInstance* inst = campaign->getPlayerShip();
			if (inst->upgrades[i] > -1) campaign->getShipUpgrade(inst->upgrades[i])->upgrade(id);
		}
	}
	return id;
}

void initWingmanAI(flecs::entity player, flecs::entity id, WingmanInstance* wingData)
{
	auto ai = wingData->ai;
	ai.aiControls = std::shared_ptr<AIType>(new AIType);
	ai.wingCommander = player;
	ai.onWing = true;
	ai.setBehavior(AIBHVR_CAN_BE_ORDERED | AIBHVR_AVOIDS_OBSTACLES);

	ai.orderLines[ORD_ATTACK] = wingData->attackLine;
	ai.orderLines[ORD_DOCK] = wingData->dockLine;
	ai.orderLines[ORD_FORMUP] = wingData->formUpLine;
	ai.orderLines[ORD_HALT] = wingData->haltLine;
	ai.orderLines[ORD_HONK_HELP] = wingData->helpLine;
	ai.orderLines[ORD_DISENGAGE] = wingData->disengageLine;

	ai.deathLine = wingData->deathLine;
	ai.killLine = wingData->killLine;
	ai.negLine = wingData->negLine;

	ai.AIName = wingData->name;

	id.set<AIComponent>(ai);
}

flecs::entity createWingman(u32 num, flecs::entity player, vector3df pos, vector3df rot)
{
	if (!campaign->getAssignedWingman(num) || !campaign->getAssignedShip(num)) return INVALID_ENTITY;
	auto wingData = campaign->getAssignedWingman(num);
	auto id = createShipFromInstance(*campaign->getAssignedShip(num), pos, rot);
	if (!cfg->game.toggles[GTOG_IMPACT]) {
		auto hp = id.get_mut<HealthComponent>();
		hp->healthResistances[IMPACT] = 1.f;
	}
	if (id == INVALID_ENTITY) return id;
	id.set_doc_name(wingData->name.c_str());

	initializePlayerFaction(id);
	initializeDefaultSensors(id);
	initWingmanAI(player, id, wingData);
	setDamageDifficulty(id);

	id.set<StatTrackerComponent>(StatTrackerComponent());
	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		if (campaign->getShip(wingData->assignedShip)->upgrades[i] > -1) campaign->getShipUpgrade(campaign->getShip(wingData->assignedShip)->upgrades[i])->upgrade(id);
	}

	return id;
}

std::list<flecs::entity> spawnShipWing(vector3df pos, vector3df rot, dataId wingSize, dataId shipId, dataId wepId, dataId aceShipId, dataId aceWepId, dataId turretId, dataId turretWepId, bool friendly, bool decloaking)
{
	std::list<flecs::entity> ret;
	flecs::entity ace;
	if (!friendly) ace = createHostileShip(aceShipId, aceWepId, pos, rot, decloaking);
	else ace = createFriendlyShip(aceShipId, aceWepId, pos, rot, decloaking);
	initializeAceAI(ace);
	initializeDefaultSensors(ace);
	ret.push_back(ace);

	if (shipData[aceShipId]->artilleryShip) setArtillery(ace);
	if (shipData[aceShipId]->hasTurrets) setGunship(ace, turretId, turretWepId);

	std::string name = ace.doc_name();
	name += " Ace";
	ace.set_doc_name(name.c_str());
	for (u32 i = 0; i < wingSize; ++i) {
		vector3df wingpos = getPointInSphere(pos, 325.f * ace.get<IrrlichtComponent>()->node->getScale().X);
		flecs::entity ship;
		if (!friendly) ship = createHostileShip(shipId, wepId, wingpos, rot, decloaking);
		else ship = createFriendlyShip(shipId, wepId, wingpos, rot, decloaking);
		initializeDefaultAI(ship);
		initializeDefaultSensors(ship);

		if (shipData[shipId]->artilleryShip) setArtillery(ship);
		if (shipData[shipId]->hasTurrets) setGunship(ship, turretId, turretWepId);

		auto ai = ship.get_mut<AIComponent>();
		ai->wingCommander = ace;
		ai->onWing = true;
		ret.push_back(ship);
	}
	return ret;
}

std::list<flecs::entity> spawnScenarioWing(vector3df pos, vector3df rot, u32 shipId, u32 wepId, u32 aceShipId, u32 aceWepId, u32 turretId, u32 turretWepId)
{
	std::list<flecs::entity> ret;
	flecs::entity ace = createHostileShip(aceShipId, aceWepId, pos, rot, true);

	initializeAceAI(ace);
	initializeDefaultSensors(ace);
	auto acedat = shipData[aceShipId];
	auto shipdat = shipData[shipId];
	if (acedat->hasTurrets) initializeTurretsOnOwner(ace, turretId, turretWepId);
	if (acedat->artilleryShip) setArtillery(ace);
	ret.push_back(ace);

	std::string name = ace.doc_name();
	name += " Ace";
	ace.set_doc_name(name.c_str());

	for (u32 i = 0; i < getCurAiNum(); ++i) {
		vector3df wingpos = getPointInSphere(pos, 325.f);
		flecs::entity ship = createHostileShip(shipId, wepId, wingpos, rot, true);
		initializeDefaultAI(ship);
		initializeDefaultSensors(ship);
		if (shipdat->hasTurrets) initializeTurretsOnOwner(ship, turretId, turretWepId);
		if (shipdat->artilleryShip) setArtillery(ship);
		auto ai = ship.get_mut<AIComponent>();
		ai->wingCommander = ace;
		ai->onWing = true;
		ret.push_back(ship);
	}
	return ret;
}

MapGenShip::MapGenShip(flecs::entity ent, vector3df position, vector3df rotation)
{
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		hardpoints[i] = INVALID_DATA_ID;
		wepArchetype[i] = false;
		hardpointNetworkIds[i] = INVALID_NETWORK_ID;
	}
	this->position = position;
	this->rotation = rotation;
	auto ship = ent.get<ShipComponent>();
	if (ship) id = ship->shipDataId;

	auto fac = ent.get<FactionComponent>();
	if (fac) faction = fac->type;

	auto hards = ent.get<HardpointComponent>();
	if (hards) {
		for (u32 i = 0; i < hards->hardpointCount; ++i) {
			if (hards->weapons[i].is_alive()) {
				auto wep = hards->weapons[i].get<WeaponInfoComponent>();
				this->hardpoints[i] = wep->wepDataId;
				this->hardpointNetworkIds[i] = hards->weapons[i].get<NetworkingComponent>()->networkedId;
				baedsLogger::log("gun network id: " + std::to_string(hardpointNetworkIds[i]) + "\n");
			}
		}
		if (hards->physWeapon) {
			auto wep = hards->physWeapon.get<WeaponInfoComponent>();
			this->phys = wep->wepDataId;
			this->physNetworkId = hards->physWeapon.get<NetworkingComponent>()->networkedId;
		}
		if (hards->heavyWeapon) {
			auto wep = hards->heavyWeapon.get<WeaponInfoComponent>();
			this->heavy = wep->wepDataId;
			this->heavyNetworkId = hards->heavyWeapon.get<NetworkingComponent>()->networkedId;
		}
	}
	networkId = ent.get<NetworkingComponent>()->networkedId;

	baedsLogger::log("SHIP NETWORK ID CONSTRUCTED: " + std::to_string(networkId) + "\n");
}

flecs::entity createShipFromMapGen(MapGenShip ship)
{
	flecs::entity id = game_world->entity();
	auto data = shipData.at(ship.id);
	loadShip(ship.id, id, ship.position, ship.rotation, true, ship.networkId);
	if (id.has<HardpointComponent>()) {
		auto hards = id.get<HardpointComponent>();
		baedsLogger::log("Gun network ids: ");
		for (u32 i = 0; i < hards->hardpointCount; ++i) {
			baedsLogger::log(std::to_string(ship.hardpointNetworkIds[i]) + ", ");
			if (ship.hardpoints[i] != INVALID_DATA_ID) 
				initializeWeaponFromId(ship.hardpoints[i], id, i, HRDP_REGULAR, ship.hardpointNetworkIds[i]);

		}
		baedsLogger::log("\n");

		if (ship.phys != INVALID_DATA_ID) 
			initializeWeaponFromId(ship.phys, id, PHYS_HARDPOINT, HRDP_PHYSICS, ship.physNetworkId);

		if (ship.heavy != INVALID_DATA_ID) 
			initializeWeaponFromId(ship.heavy, id, HEAVY_HARDPOINT, HRDP_HEAVY, ship.heavyNetworkId);
	}
	if (data->hasTurrets) {
		auto turr = id.get_mut<TurretHardpointComponent>();
		auto irr = id.get<IrrlichtComponent>();
		for (u32 i = 0; i < MAX_TURRET_HARDPOINTS; ++i) {
			turr->turrets[i] = createTurret(ship.turretIds[i], ship.turretWepIds[i], getTurretPosition(data->turr.turretPositions[i], irr->node),
				data->turr.turretRotations[i], ship.faction, vector3df(turr->turretScale), i, id, ship.turretNetworkIds[i]);
		}
	}
	initializeFaction(id, ship.faction);
	return id;
}

void fighterDeathSpiralCallback(flecs::entity id)
{
	if (!id.is_alive()) return;
	if (!id.has<IrrlichtComponent>() || !id.has<BulletRigidBodyComponent>() || !id.has<FactionComponent>()) return;
	auto irr = id.get<IrrlichtComponent>();
	auto node = irr->node;
	node->setID(ID_IsNotSelectable);
	vector3df start = node->getAbsolutePosition();
	vector3df pt1 = start + (getNodeForward(node) * random.frange(30.f, 100.f)) + (getNodeUp(node) * random.frange(3.f, 10.f));
	vector3df pt2 = pt1 + (getNodeDown(node) * random.frange(-15.f, 15.f)) + (getNodeForward(node) * random.frange(40.f, 60.f));
	vector3df pt3 = pt2 + (getNodeForward(node) * random.frange(40.f, 60.f)) + (getNodeRight(node) * random.frange(10.f, 20.f));

	irr::core::array<vector3df> points;
	points.push_back(start); points.push_back(pt1); points.push_back(pt2); points.push_back(pt3);

	auto anim = smgr->createFollowSplineAnimator(device->getTimer()->getTime(), points, 1.f, .5f, false, false, false);
	node->addAnimator(anim);
	anim->drop();

	anim = smgr->createRotationAnimator(vector3df(1.f, 0, 1.5f));
	node->addAnimator(anim);
	anim->drop();

	auto ent = game_world->entity();
	ent.set<IrrlichtComponent>(*irr);
	ent.set<FactionComponent>(*id.get<FactionComponent>());
	std::shared_ptr<StatusEffect> effect(new DelayedKillEffect);
	effect->duration = 1.4f;
	ent.get_mut<StatusEffectComponent>()->effects.push_back(effect);


	gameController->registerDeathCallback(ent, fighterDeathExplosionCallback);
}

void fighterDeathExplosionCallback(flecs::entity id)
{
	if (!id.is_alive()) return;
	if (!id.has<IrrlichtComponent>()) return; //we just can't do anything
	auto irr = id.get<IrrlichtComponent>();
	vector3df pos = irr->node->getAbsolutePosition();
	vector3df scale = irr->node->getScale();
	f32 avgscale = (scale.X + scale.Y + scale.Z);
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale;
	auto extype = EXTYPE_REGULAR;
	std::string explodenoise = "death_explosion_fighter.ogg";
	if (id.has<FactionComponent>()) {
		auto fac = id.get<FactionComponent>();
		if (fac->type == FACTION_HOSTILE) {
			avgscale *= 2.5f;
			extype = EXTYPE_ALIEN;
			explodenoise = "death_organic_fighter.ogg";
		}
		else if (fac->type == FACTION_UNCONTROLLED) extype = EXTYPE_TECH;
	}
	explode(pos, 2.f, avgscale, rad, 40.f, 300.f, extype);
	audioDriver->playGameSound(pos, explodenoise, 1.f, 250.f);
}