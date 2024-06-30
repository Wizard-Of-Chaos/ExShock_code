#pragma once
#ifndef MAPBUILDER_H
#define MAPBUILDER_H
#include "BaseHeader.h"
#include "Objective.h"
#include "ScenarioEnums.h"
#include "GvReader.h"
#include "Dialogue.h"
#include "FactionComponent.h"
#include "LoadoutData.h"

typedef std::shared_ptr<Objective> ObjElem;

class MapRunner
{
	public:
		void build(SCENARIO_TYPE objective, SECTOR_TYPE sector, bool isCampaignMatch = false, std::string scenarioWhich="");
		void startMap();
		bool run(f32 dt);
		void endMap();

		std::shared_ptr<Objective> objective() { return m_curObjective; }

		//sets a list of spawners (for a jumpscare beacon for example) to the current scenario's loadout
		void setSpawns(s32& ace, s32& reg, s32& aceWep, s32& regWep, s32& turret, s32& turretWep) const
		{ ace = m_regAce; reg = m_regShip; aceWep = m_regAceGun; regWep = m_regGun; turret = m_regTurret; turretWep = m_regTurretGun; }
		const vector3df& playerStart() const { return m_playerStartPos; }

		std::vector<MapGenShip>& mapShips() { return m_mapShips; }
		std::vector<MapGenObstacle>& mapObstacles() { return m_mapObstacles; }
		void addShipToMapShips(MapGenShip ship) { m_mapShips.push_back(ship); }
		void addObstacleToMapObstacles(MapGenObstacle obstacle) { m_mapObstacles.push_back(obstacle); }
		const std::vector<vector3df>& const objectiveStartPos() { return m_objectivesStartPos; }
		flecs::entity radioSignal;

	private:
		struct _corridor {
			vector3df start;
			vector3df end;
		};
		std::vector<_corridor> m_corridors;
		std::vector<vector3df> m_obstaclePositions;

		u32 m_obstaclesAroundObjective = 0;
		u32 m_looseObstacles = 0;
		u32 m_obstDensityPerKm = 0;
		f32 m_maxObjGenRadius = 20000.f;
		f32 m_maxObjTorusRadius = 2500.f;
		f32 m_maxTorusDistance = 10000.f;

		f32 m_objectiveDeadZone = 0;
		f32 m_corridorRadius = 0;
		f32 m_objectiveFieldRadius = 0;

		SCENARIO_TYPE m_type;
		SECTOR_TYPE m_sector;

		bool m_boss = false;
		bool m_cleanup = true;
		bool m_bustSignals = true;
		bool m_campaignMatch = true;
		bool m_customMap = false;
		bool m_customObjective = false;

		IrrXMLReader* xml = nullptr;

		bool m_timeForBanter();
		std::vector<Banter> m_bantz;
		f32 m_timeBetweenBanters;
		f32 m_minTimeSinceBeingShot;
		f32 m_banterTimer = 0.f;

		void m_preload();
		void m_setGlobals();
		void m_setPlayerObjPositions();
		void m_setPlayer();
		void m_setObstacles();
		void m_setObjectives();

		//environments and their bossfights
		void m_debris();
		void m_debrisRoadblocks();
		void m_debrisBoss();

		void m_asteroid();
		void m_asteroidRoadblocks();
		void m_asteroidBoss();

		void m_gas();
		void m_gasRoadblocks();
		void m_gasBoss();

		void m_supplyDepot();
		void m_supplyDepotRoadblocks();
		void m_supplyDepotBoss();

		void m_rings();
		void m_ringsRoadblocks();
		void m_ringsBoss();

		void m_fleet();
		void m_fleetRoadblocks();
		void m_fleetBoss();

		void m_finale();
		void m_finaleRoadblocks();
		void m_finaleBoss();

		//objectives
		void m_dogfight();
		void m_salvage();
		void m_pod();
		void m_brawl();
		void m_destroy_object();
		void m_capture();
		void m_frigate_rescue();
		void m_scuttle();
		void m_rock_impact();
		void m_extract();
		void m_distress();
		void m_escort();
		void m_scramble();
		void m_harvest();
		void m_search_and_destroy();

		void m_arnold();
		void m_sean();
		void m_cat();
		void m_tauran();
		void m_lee();
		void m_theod();
		void m_arthur();
		void m_james();

		void m_tutorial();

		std::shared_ptr<Objective> m_getCleanup();
		void m_setRadioSignals(ObjElem& obj);

		bool m_buildScenarioFromXml(std::string xml);
		std::vector<MapGenShip> m_mapShips;
		std::vector<MapGenObstacle> m_mapObstacles;
		//Scenario m_builtScenario;

		s32 m_regAce = 4;
		s32 m_regShip = 1;
		s32 m_regAceGun = 9;
		s32 m_regGun = 1;
		s32 m_scoutAce = 1;
		s32 m_scoutShip = 8;
		s32 m_scoutAceGun = 3;
		s32 m_scoutGun = 8;

		s32 m_regTurret = 1;
		s32 m_regTurretGun = 1;
		s32 m_scoutTurret = 8;
		s32 m_scoutTurretGun = 8;

		//spawners
		std::list<flecs::entity> m_spawnRegWing(vector3df pos, vector3df rot);
		std::list<flecs::entity> m_spawnScoutWing(vector3df pos, vector3df rot);
		std::list<flecs::entity> m_spawnWing(vector3df pos, vector3df rot, s32 aceShip, s32 regShip, s32 aceGun, s32 regGun, s32 turret, s32 turretGun, s32 numAdjust=0);
		std::list<flecs::entity> m_fleetBattleSpawn(vector3df pos, vector3df rot);

		gvReader m_mapData;
		gvReader m_objectiveData;

		vector3df m_playerStartPos;
		std::vector<vector3df> m_objectivesStartPos;
		
		std::list<std::shared_ptr<Objective>> m_objectives;
		std::shared_ptr<Objective> m_curObjective;

};

#endif
