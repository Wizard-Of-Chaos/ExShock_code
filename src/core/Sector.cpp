#include "Sector.h"
#include "ShipInstance.h"
#include "StatTrackerComponent.h"
#include "Scenario.h"
#include "GvReader.h"
#include "IrrlichtUtils.h"
#include "Campaign.h"
#include "GameController.h"
#include "GuiController.h"
#include "CrashLogger.h"

Sector::Sector(SECTOR_TYPE stype) : type(stype), moved(false), encounterNum(0) 
{
	gvReader in;
	in.read("assets/attributes/scenarios/environments/" + environmentTypeStrings.at(type) + ".gdat");
	in.readLinesToValues();

	combatMusic = in.getString("combatMusic");
	ambientMusic = in.getString("ambientMusic");
	menuMusic = in.getString("menuMusic");
	introDialogue = wstr(in.getString("introDialogue"));

	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		scenarioOptions[i] = nullptr;
	}
}

bool Sector::advance(bool fromSettings)
{
	if (moved && !fromSettings) return false;
	++encounterNum;
	buildScenarios();
	if (!fromSettings) {
		for (auto wingman : campaign->wingmen()) {
			if (!wingman->injured) continue;
			++wingman->turnsInjured;
			if (wingman->turnsInjured == 2) {
				wingman->injured = false;
				wingman->turnsInjured = 0;
			}
		}
	}
	moved = true;
	if (encounterNum >= MAX_ENCOUNTERS) {
		buildBossScenario();
		m_sectorComplete = true;
		if (m_sectorComplete && campaign->getFlag(L"MARTIN_BUILDING_CLOAK")) {
			campaign->setFlag(L"MARTIN_BUILDING_CLOAK", false);
			campaign->setFlag(L"MARTIN_CLOAK_COMPLETED");
		}
		return true;
	}
	return false;
}

void Sector::clearScenarios()
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
}

void Sector::initSectorFromSettings(bool hasMoved, u32 encounters)
{
	encounterNum = encounters;
	moved = hasMoved;
	if (hasMoved) {
		if (encounterNum >= MAX_ENCOUNTERS) {
			m_sectorComplete = true;
			buildBossScenario();
		}
		else buildScenarios();
	}
}

void Sector::m_setShipInstanceFromEntity(ShipInstance* inst, flecs::entity ent, bool wingman)
{
	if (!inst) return; //what?
	if (!ent && !gameController->isWingmanDisengaged(ent)) {
		baedsLogger::log("This entity is dead -- cannot set ship instance.\n");
		if (!wingman) {
			baedsLogger::log("PLAYER DEAD BUT STILL CALLING SHIP INSTANCE SET - WHATEVER YOU'RE DOING YOU HAVE DONE IT ***WRONG***\n");
			return; //something horrible has happened if this triggered
		}
		WingmanInstance* mans = campaign->getWingman(inst->inUseBy);
		u32 slot;
		bool found = false;
		for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
			if (campaign->getAssignedWingman(i) == mans) {
				slot = i;
				found = true;
				break;
			}
		}
		if (!found) return; //again this should never happen
		campaign->removeAssignedWingman(slot);
		mans->injured = true;
		++mans->totalInjuries;

		u32 roll = random.d6();
		if (roll == 1 && inst->ship.shipDataId != 5) { //arthur's ship cannot be destroyed
			campaign->destroyShip(inst);
			return;
		}
		//doesn't reload if the ship was destroyed, probably fix that later
		//honestly, just the HP loss is punishment enough
		return;
	}
	auto hp = ent.get<HealthComponent>();
	auto hards = ent.get<HardpointComponent>();

	inst->hp = *hp;

	inst->hp.health = std::roundf(inst->hp.health); //sick of micro-repairs
	if (inst->hp.health <= 0.f) inst->hp.health = 1.f;
	if (inst->hp.health > inst->hp.maxHealth) inst->hp.health = inst->hp.maxHealth;
	inst->hp.shields = inst->hp.maxShields;

	for (u32 i = 0; i < hards->hardpointCount; ++i) {
		if (inst->weps[i] > -1) {
			auto wep = hards->weapons[i].get<WeaponInfoComponent>();
			auto wepFire = hards->weapons[i].get_mut<WeaponFiringComponent>();
			if (wepFire->clip < wep->maxClip) { //if the clip is partially spent just reload the damn thing
				wepFire->clip = wep->maxClip;
				if (wepFire->ammunition >= wep->maxClip) {
					wepFire->ammunition -= wep->maxClip;
				}
				else {
					wepFire->ammunition = 0;
				}
			}
			campaign->getWeapon(inst->weps[i])->fire = *wepFire;
		}
	}
	if (inst->heavyWep > -1) {
		auto wep = hards->heavyWeapon.get<WeaponInfoComponent>();
		auto wepFire = hards->heavyWeapon.get_mut<WeaponFiringComponent>();

		if (wepFire->clip < wep->maxClip) {
			wepFire->clip = wep->maxClip;
			if (wepFire->ammunition >= wep->maxClip) {
				wepFire->ammunition -= wep->maxClip;
			}
			else {
				wepFire->ammunition = 0;
			}
		}
		campaign->getWeapon(inst->heavyWep)->fire = *wepFire;
	}

	auto stats = ent.get<StatTrackerComponent>();

	auto mans = campaign->getWingman(inst->inUseBy);
	if (!mans) return;
	mans->totalKills += stats->kills;
}

void Sector::finishScenario()
{
	moved = false;
	baedsLogger::log("Retrieving player data from scenario.\n");
	m_setShipInstanceFromEntity(campaign->getPlayerShip(), gameController->getPlayer(), false);
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		if (!campaign->getAssignedWingman(i)) continue;
		baedsLogger::log("Retrieving wingman data from scenario...\n");
		m_setShipInstanceFromEntity(campaign->getAssignedShip(i), gameController->getWingman(i));
	}
	currentScenario->getLoot();
	if (encounterNum == 1 || encounterNum == 4) {
		campaign->setFlag(L"EVENT_AVAILABLE");
	}
}

void DebrisSector::buildScenarios()
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = new Scenario(SECTOR_DEBRIS);
		scenarioOptions[i]->setRandom(campaign->getDifficulty());
	}
}
void DebrisSector::buildBossScenario()
{
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"debris_boss_event"));
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	scenarioOptions[1] = new Scenario(SECTOR_DEBRIS);
	scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BOSSFIGHT);
}

void AsteroidSector::buildScenarios()
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = new Scenario(SECTOR_ASTEROID);
		scenarioOptions[i]->setRandom(campaign->getDifficulty());
	}
}

void AsteroidSector::buildBossScenario()
{
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"asteroid_boss_event"));
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	scenarioOptions[1] = new Scenario(SECTOR_ASTEROID);
	scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BOSSFIGHT);
}

void GasSector::buildScenarios()
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = new Scenario(SECTOR_GAS);
		scenarioOptions[i]->setRandom(campaign->getDifficulty());
	}
}
void GasSector::buildBossScenario()
{
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"gas_boss_event"));
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	scenarioOptions[1] = new Scenario(SECTOR_GAS);
	scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BOSSFIGHT);

}

void SupplyDepotSector::buildScenarios()
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = new Scenario(SECTOR_SUPPLY_DEPOT);
		scenarioOptions[i]->setRandom(campaign->getDifficulty());
	}
}
void SupplyDepotSector::buildBossScenario()
{
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"supply_depot_boss_event"));
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	scenarioOptions[1] = new Scenario(SECTOR_SUPPLY_DEPOT);
	scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BOSSFIGHT);
}

void RingSector::buildScenarios()
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = new Scenario(SECTOR_RINGS);
		scenarioOptions[i]->setRandom(campaign->getDifficulty());
	}
}
void RingSector::buildBossScenario()
{
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"rings_boss_event"));
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	scenarioOptions[1] = new Scenario(SECTOR_RINGS);
	scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BOSSFIGHT);
}
void FleetSector::buildScenarios()
{
	campaign->setFlag(L"FLEETSECTOR_FREE_CHOICE_MISSION", false);
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	u32 roll = random.urange(0, 5);
	///*
	switch (roll) {
	case 0:
		for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
			scenarioOptions[i] = new Scenario(SECTOR_FLEET_GROUP);
			scenarioOptions[i]->setRandom(campaign->getDifficulty());
		}
		guiController->setDialogueTree(campaign->getCharacterDialogue(L"davis_mission_generic_0"));
		campaign->setFlag(L"FLEETSECTOR_FREE_CHOICE_MISSION");
		break;
	case 1:
		scenarioOptions[1] = new Scenario(SECTOR_FLEET_GROUP);
		scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_SEARCH_AND_DESTROY);
		guiController->setDialogueTree(campaign->getCharacterDialogue(L"davis_mission_generic_1"));
		break;
	case 2:
		scenarioOptions[1] = new Scenario(SECTOR_FLEET_GROUP);
		scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_FRIGATE_RESCUE);
		guiController->setDialogueTree(campaign->getCharacterDialogue(L"davis_mission_generic_2"));
		break;
	case 3:
		scenarioOptions[1] = new Scenario(SECTOR_FLEET_GROUP);
		scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_DESTROY_OBJECT);
		guiController->setDialogueTree(campaign->getCharacterDialogue(L"davis_mission_generic_3"));
		break;
	case 4:
		scenarioOptions[1] = new Scenario(SECTOR_FLEET_GROUP);
		scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_DISTRESS_BEACON);
		guiController->setDialogueTree(campaign->getCharacterDialogue(L"davis_mission_generic_4"));
		break;
	case 5:
		scenarioOptions[1] = new Scenario(SECTOR_FLEET_GROUP);
		scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BRAWL);
		guiController->setDialogueTree(campaign->getCharacterDialogue(L"davis_mission_generic_5"));
		break;
	default:
		break;
	}
	
	//scenarioOptions[1] = new Scenario(SECTOR_FLEET_GROUP);
	//scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_FRIGATE_RESCUE);
	//guiController->setDialogueTree(campaign->getCharacterDialogue(L"davis_mission_generic_2"));

}
void FleetSector::buildBossScenario()
{
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"fleet_boss_event"));
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	scenarioOptions[1] = new Scenario(SECTOR_FLEET_GROUP);
	scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BOSSFIGHT);
}
void CometSector::buildScenarios()
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = new Scenario(SECTOR_FINALE);
		scenarioOptions[i]->setRandom(campaign->getDifficulty());
	}
}
void CometSector::buildBossScenario()
{
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"comet_boss_event"));
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		if (scenarioOptions[i]) delete scenarioOptions[i];
		scenarioOptions[i] = nullptr;
	}
	scenarioOptions[1] = new Scenario(SECTOR_FINALE);
	scenarioOptions[1]->setType(campaign->getDifficulty(), SCENARIO_BOSSFIGHT);
}