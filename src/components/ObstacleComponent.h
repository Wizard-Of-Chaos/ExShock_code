#pragma once
#ifndef OBSTACLE_COMPONENT_H
#define OBSTACLE_COMPONENT_H
#include "BaseHeader.h"

//Different types of obstacle.
enum OBSTACLE
{
	ASTEROID,
	ICE_ASTEROID,
	RADIOACTIVE_ASTEROID,
	HUGE_ASTEROID,
	GAS_CLOUD,
	SPACE_STATION,
	DEBRIS,
	JET_DEBRIS,
	EXPLOSIVE_ASTEROID,
	STASIS_POD,
	STATION_MODULE,
	DERELICT_STATION_MODULE,
	MESH_GASCLOUD,
	DEAD_SHIP,
	DEAD_WEAPON,
	FLAT_BILLBOARD,
	FLAT_BILLBOARD_ANIMATED
};
//Convenience map for types of obstacles that can be read from file.
const std::unordered_map<std::string, OBSTACLE> obstacleStrings = {
	{"asteroid", ASTEROID},
	{"gascloud", GAS_CLOUD},
	{"space_station", SPACE_STATION},
	{"debris", DEBRIS},
	{"jetdebris", JET_DEBRIS},
	{"explosiveasteroid", EXPLOSIVE_ASTEROID},
	{"stasis_pod", STASIS_POD},
	{"station_module", STATION_MODULE},
	{"mesh_gascloud", MESH_GASCLOUD},
	{"billboard", FLAT_BILLBOARD},
	{"billboard_animated", FLAT_BILLBOARD_ANIMATED}
};

/*
* The obstacle component keeps track of what obstacle type a given obstacle would be. Might do other things later, but for now it's just the one part.
* The major thing this is useful for is just determining what an obstacle is in the scene.
*/
struct ObstacleComponent
{
	ObstacleComponent() {};
	ObstacleComponent(OBSTACLE type) : type(type) {};
	OBSTACLE type;
};

#endif 