#pragma once
#ifndef STATIONMODULESYSTEM_H
#define STATIONMODULESYSTEM_H
#include "BaseHeader.h"

struct StationModuleComponent;
struct PowerComponent;
//should just be a quick cleanup check
void stationModuleSystem(flecs::iter it, StationModuleComponent* smod);

void powerSystem(flecs::iter it, PowerComponent* pwrc);
#endif 