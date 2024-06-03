#include "LargeShipUtils.h"
#include "GameController.h"
#include "GameStateController.h"
#include "GuiController.h"
#include "HardpointComponent.h"
#include "TurretUtils.h"
#include "AttributeReaders.h"
#include "AudioDriver.h"
#include "ShipUtils.h"
#include "GameFunctions.h"
#include "ObstacleAI.h"

flecs::entity createAlienCarrier(dataId id, vector3df pos, vector3df rot, const dataId weaponId, const s32 turretId, const s32 turretWepId)
{
	flecs::entity carrier = game_world->entity();
	loadShip(id, carrier, pos, rot);
	ShipData* carr = shipData[id];
	carrier.set_doc_name(carr->name.c_str());

	if (carr->hasHangar) {
		auto carrcmp = carrier.get_mut<HangarComponent>();
		carrcmp->shipTypeCount = 1;
		carrcmp->spawnShipArchetypes[0] = 1;
	}
	initializeHostileFaction(carrier);
	initializeSensors(carrier, 2800.f, 3.f);
	//initializeDefaultSensors(carrier);
	initializeDefaultAI(carrier, new FrigateAI); //TODO: swap to frigate AI later
	initializeTurretsOnOwner(carrier, turretId, turretWepId);
	initializeRegularWeapons(carrier, weaponId);
	setDamageDifficulty(carrier);
	setArtillery(carrier);
	return carrier;
}

flecs::entity createHumanCarrier(dataId id, vector3df pos, vector3df rot, const dataId weaponId, const s32 turretId, const s32 turretWepId)
{
	flecs::entity carrier = game_world->entity();
	loadShip(id, carrier, pos, rot);
	ShipData* carr = shipData[id];
	carrier.set_doc_name(carr->name.c_str());

	if (carr->hasHangar) {
		auto carrcmp = carrier.get_mut<HangarComponent>();
		carrcmp->shipTypeCount = 1;
		carrcmp->spawnShipArchetypes[0] = 2;
	}
	initializeFaction(carrier, FACTION_PLAYER);
	initializeSensors(carrier, 2800.f, 3.f);
	//initializeDefaultSensors(carrier);
	initializeDefaultAI(carrier, new FrigateAI); //TODO: swap to frigate AI later
	initializeTurretsOnOwner(carrier, turretId, turretWepId);
	initializeRegularWeapons(carrier, weaponId);
	setDamageDifficulty(carrier);
	setArtillery(carrier);
	return carrier;
}

flecs::entity createChaosTheory(vector3df pos, vector3df rot, bool holdPos)
{
	const ChaosTheoryStats& stats = campaign->getCarrierStats();
	flecs::entity carrier = game_world->entity();
	loadShip(CHAOS_THEORY_ID, carrier, pos, rot);
	carrier.set<HealthComponent>(stats.hp);
	carrier.set<ThrustComponent>(stats.thrst);
	initializePlayerFaction(carrier);
	initializeSensors(carrier, 3500.f, 3.f);
	//initializeDefaultAI(carrier);

	initializeTurretsOnOwner(carrier, stats.turretId, stats.turretWepId);
	initializeRegularWeapons(carrier, 16);

	auto instance = loadWingman(wingMarkers[7], false);
	initWingmanAI(gameController->getPlayer(), carrier, instance);
	delete instance;
	if (!cfg->game.toggles[GTOG_IMPACT]) {
		auto hp = carrier.get_mut<HealthComponent>();
		hp->healthResistances[IMPACT] = 1.f;
	}

	auto ai = carrier.get_mut<AIComponent>();
	ai->aiControls = std::shared_ptr<AIType>(new FrigateAI);

	if (holdPos) {
		auto irr = carrier.get<IrrlichtComponent>();
		ai->orderOverride = true;
		ai->removeBehavior(AIBHVR_CAN_BE_ORDERED);
		ai->fixedPatrolRoute = true;
		ai->state = AI_STATE_PATROL;
		ai->hasPatrolRoute = true;
		ai->route.push_back(irrVecToBt(pos));
		ai->route.push_back(irrVecToBt(pos + (getNodeLeft(irr->node) * 1200.f)));
		ai->route.push_back(irrVecToBt(pos + (getNodeRight(irr->node) * 1200.f)));
	}
	setArtillery(carrier);
	setDamageDifficulty(carrier);

	return carrier;
}

flecs::entity createTroopTransport(vector3df pos)
{
	//WingmanInstance inst;
	auto inst = loadWingman(wingMarkers[8], false);
	flecs::entity transport = createFriendlyShip(TRANSPORT_ID, 3, pos, vector3df(0, 0, 0));
	initWingmanAI(gameController->getPlayer(), transport, inst);
	return transport;
}

void carrierDeathExplosionCallback(flecs::entity id)
{
	auto irr = id.get<IrrlichtComponent>();
	auto turr = id.get<TurretHardpointComponent>();

	vector3df pos = irr->node->getAbsolutePosition();
	vector3df scale = irr->node->getScale();
	f32 avgscale = (scale.X + scale.Y + scale.Z)/3.f;
	f32 rad = irr->node->getBoundingBox().getExtent().getLength() * avgscale;
	explode(pos, 3.f, avgscale, rad, 30.f, 450.f, EXTYPE_REGULAR);
	audioDriver->playGameSound(pos, "death_explosion_carrier.ogg", 1.f, 100.f);
}