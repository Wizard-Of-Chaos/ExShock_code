#pragma once
#ifndef SECTOR_H
#define SECTOR_H
#include "BaseHeader.h"
#include "Scenario.h"
class Scenario;
struct ShipInstance;

const u32 NUM_SCENARIO_OPTIONS = 4;
extern const u32 MAX_ENCOUNTERS;

/*
* Sectors contain the data for whatever is currently going on in the campaign board. This includes what encounter the sector is currently at
* as well as options for scenarios that are available to fly into. A given sector will also include its own music and functions for building
* its scenarios. The sector class gets extended into classes like DebrisSector, AsteroidSector, and so on.
*/
class Sector
{
	public:
		Sector(SECTOR_TYPE stype);
		SECTOR_TYPE getType() { return type; }
		virtual bool advance(bool fromSettings=false); //returns true if we're at the end of the sector
		bool sectorComplete() { return m_sectorComplete; }
		Scenario* getCurrentScenario() { return currentScenario; }
		Scenario* getScenario(u32 pos) { return scenarioOptions[pos]; }
		void finishScenario();

		void selectCurrentScenario(u32 pos) { currentScenario = scenarioOptions[pos]; }
		void deselectCurrentScenario() { currentScenario = nullptr; }
		u32 getEncounterNum() { return encounterNum; }

		bool hasMoved() { return moved; }

		void initSectorFromSettings(bool moved, u32 encounterNum);

		virtual void buildScenarios()=0;
		void clearScenarios();
		virtual void buildBossScenario()=0;

		std::string combatMusic = "combat_default.ogg";
		std::string ambientMusic = "ambient_default.ogg";
		std::string menuMusic = "campaign_default.ogg";
		std::wstring introDialogue = L"steven_newgame";

		u32 wingmenRecruitedThisSector = 0;
	protected:
		void m_setShipInstanceFromEntity(ShipInstance* inst, flecs::entity ent, bool wingman = true);
		bool moved;
		bool m_sectorComplete;
		u32 encounterNum;
		Scenario* scenarioOptions[NUM_SCENARIO_OPTIONS];
		Scenario* currentScenario;
		SECTOR_TYPE type;
};

class DebrisSector : public Sector
{
	public:
		DebrisSector() : Sector(SECTOR_DEBRIS) { }
		virtual void buildScenarios();
		virtual void buildBossScenario();
	protected:
};

class AsteroidSector : public Sector
{
	public:
		AsteroidSector() : Sector(SECTOR_ASTEROID) { }
		virtual void buildScenarios();
		virtual void buildBossScenario();
	protected:
};

class GasSector : public Sector
{
	public:
		GasSector() : Sector(SECTOR_GAS) { }
		virtual void buildScenarios();
		virtual void buildBossScenario();
	protected:
};

class SupplyDepotSector : public Sector
{
	public:
		SupplyDepotSector() : Sector(SECTOR_SUPPLY_DEPOT) { }
		virtual void buildScenarios();
		virtual void buildBossScenario();
	protected:
};

class RingSector : public Sector
{
	public:
		RingSector() : Sector(SECTOR_RINGS) { }
		virtual void buildScenarios();
		virtual void buildBossScenario();
	protected:
};

class FleetSector : public Sector
{
	public:
		FleetSector() : Sector(SECTOR_FLEET_GROUP) { }
		virtual void buildScenarios();
		virtual void buildBossScenario();
	protected:
};

class CometSector : public Sector
{
	public:
		CometSector() : Sector(SECTOR_FINALE) { }
		virtual void buildScenarios();
		virtual void buildBossScenario();
	protected:
};

#endif