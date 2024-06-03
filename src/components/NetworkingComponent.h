#ifndef NETWORKINGCOMPONENT_H
#define NETWORKINGCOMPONENT_H
#include "BaseHeader.h"

struct AccumulatorEntry;

struct NetworkingComponent
{
	uint16_t priority = 1;
	std::shared_ptr<AccumulatorEntry> accumulatorEntry;
};

#endif