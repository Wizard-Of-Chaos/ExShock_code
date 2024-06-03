#pragma once
#ifndef NETWORKINGSYSTEM_H
#define NETWORKINGSYSTEM_H
#include "BaseHeader.h"

struct NetworkingComponent;

void networkingSystem(flecs::iter it, NetworkingComponent* nc);

#endif