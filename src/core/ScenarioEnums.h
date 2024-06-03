#pragma once
#ifndef SCENARIOENUMS_H
#define SCENARIOENUMS_H

//Different types of scenario available.
enum SCENARIO_TYPE
{
	SCENARIO_KILL_HOSTILES,
	SCENARIO_DESTROY_OBJECT,
	SCENARIO_SALVAGE,
	SCENARIO_RETRIEVE_POD,
	SCENARIO_BRAWL,
	SCENARIO_CAPTURE,
	SCENARIO_FRIGATE_RESCUE,
	SCENARIO_SCUTTLE,
	SCENARIO_ROCK_IMPACT,
	SCENARIO_EXTRACT_FUEL,
	SCENARIO_DISTRESS_BEACON,
	SCENARIO_ESCORT,
	SCENARIO_HARVEST,
	SCENARIO_SEARCH_AND_DESTROY,
	//these ones shouldn't get rolled
	SCENARIO_SCRAMBLE,
	SCENARIO_BOSSFIGHT,
	SCENARIO_MAX_TYPES,

	SCENARIO_SEAN,
	SCENARIO_CAT,
	SCENARIO_TAURAN,
	SCENARIO_JAMES,
	SCENARIO_ARNOLD,
	SCENARIO_ARTHUR,
	SCENARIO_THEOD,
	SCENARIO_MI_CHA,

	SCENARIO_TEAM_DEATHMATCH,
	SCENARIO_CTF,
	SCENARIO_POINT_CONTROL,

	SCENARIO_DUMMY, //expressly for testing
	SCENARIO_CUSTOM,
	SCENARIO_NOT_LOADED
};

enum SECTOR_TYPE
{
	SECTOR_DEBRIS = 0,
	SECTOR_ASTEROID = 1,
	SECTOR_GAS = 2,
	SECTOR_SUPPLY_DEPOT = 3,
	SECTOR_RINGS = 4,
	SECTOR_FLEET_GROUP = 5,
	SECTOR_FINALE = 6
};
//Strings for loading a scenario from file.
const extern std::unordered_map<SCENARIO_TYPE, std::string> objectiveTypeStrings;
//Strings for loading and displaying a scenario environment.
const extern std::unordered_map<SECTOR_TYPE, std::string> environmentTypeStrings;

#endif // !SCENARIOENUMS_H