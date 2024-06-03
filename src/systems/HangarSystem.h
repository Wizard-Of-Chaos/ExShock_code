#pragma once
#ifndef HANGARSYSTEM_H
#define HANGARSYSTEM_H
#include "BaseHeader.h"

struct HangarComponent;
struct IrrlichtComponent;
struct FactionComponent;

//Updates all carriers in a given scene, including things like whether or not they're spawning a ship and how many ships they have in reserve.
void hangarSystem(flecs::iter it, HangarComponent* carr, IrrlichtComponent* irr, FactionComponent* fac);

#endif 