#pragma once
#ifndef SCENARIO_H
#define SCENARIO_H
#include "BaseHeader.h"
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <memory>
#include "WeaponInfoComponent.h"
#include "Objective.h"
#include "ScenarioEnums.h"

//Max amount of objectives possible (enemies to kill, things to blow up, etc).
const u32 SCENARIO_MAX_OBJECTIVES = 10;

/*
* Scenarios hold the information for a given scenario. They get randomly generated and include the objectives, player start,
* enemy start, environment, type, objective count, and other info. When initialized (i.e. run by the actual game when the player hits start)
* the entities for the game get created at the positions listed here. The scenario will get constantly checked to see whether the objectives
* have been completed, and if so, the bool "m_complete" will be set to true and the scenario will end (either that or the player dies like a dog,
* in which case the scenario ends anyway).
*/

struct WingmanInstance;
struct ShipInstance;

class Scenario
{
	public:
		//Friend! Hello, friend!
		friend class MapRunner;

		Scenario(SECTOR_TYPE env) : m_environment(env) {}
		virtual ~Scenario() {
			m_recoveredShips.clear();
			m_recoveredWeps.clear();
		}
		virtual void setRandom(u32 difficulty);
		virtual void setScramble(u32 difficulty);
		virtual void setType(u32 difficulty, SCENARIO_TYPE type);
		void print();

		u32 numAddedAmmo() { return m_ammoRecovered; }
		f32 numAddedSupplies() { return std::abs(m_suppliesRecovered); } //dunno how this throws negs
		u32 numWepsRecovered() { return m_wepsRecovered; }
		u32 numShipsRecovered() { return m_shipsRecovered; }
		u32 detectionChance() { return m_detectionChance; }

		std::string location() { return m_location; }
		std::string description() { return m_description; }
		SCENARIO_TYPE type() { return m_type; }
		SECTOR_TYPE environment() { return m_environment; }

		void getLoot();
		const std::vector<ShipInstance*>& recoveredShips() { return m_recoveredShips; }
		const std::vector<WeaponInfoComponent>& recoveredWeps() { return m_recoveredWeps; }
		const WingmanInstance* recoveredWingman() { return m_recoveredWingman; }
	protected:
		std::vector<ShipInstance*> m_recoveredShips;
		std::vector<WeaponInfoComponent> m_recoveredWeps;
		WingmanInstance* m_recoveredWingman=nullptr;

		void m_loadScenData(u32 difficulty, SCENARIO_TYPE type=SCENARIO_NOT_LOADED);

		struct _typeWeight {
			SCENARIO_TYPE which;
			u32 weight;
		};
		std::vector<_typeWeight> availableScenarioWeights;

		SCENARIO_TYPE m_type = SCENARIO_NOT_LOADED;
		SECTOR_TYPE m_environment = SECTOR_DEBRIS;
		u32 m_detectionChance = 0;

		std::string m_location;
		std::string m_description;

		u32 m_ammoRecovered=0;
		f32 m_suppliesRecovered=0;
		u32 m_wepsRecovered=0;
		u32 m_shipsRecovered=0;
};

#endif 