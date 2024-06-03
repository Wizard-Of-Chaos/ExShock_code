#include "BaedsLightManager.h"
#include "IrrlichtUtils.h"
#include "IrrlichtComponent.h"
#include <iostream>

//one day I will do something here

void BaedsLights::OnNodePreRender(ISceneNode* node)
{
	if (m_currentPass != ESNRP_SOLID && m_currentPass != ESNRP_TRANSPARENT) return;
	current = node;
	const vector3df& nodepos = node->getAbsolutePosition();

	//utility struct for sorting by distance
	struct _lDist {
		_lDist() {};
		_lDist(ILightSceneNode* n, f64 d) : node(n), dist(d) {}
		ILightSceneNode* node = 0;
		f64 dist = 0;
		bool operator < (const _lDist& other) const {
			return (dist < other.dist);
		}
	};
	array<_lDist> sortingArr;
	sortingArr.reallocate(m_list->size(), false);

	u32 i;
	for (i = 0; i < m_list->size(); ++i) {
		ILightSceneNode* lightNode = (ILightSceneNode*)(*m_list)[i];
		if (!lightNode->isVisible()) continue;
		const f64 dist = lightNode->getAbsolutePosition().getDistanceFromSQ(nodepos);
		sortingArr.push_back(_lDist(lightNode, dist));
	}
	sortingArr.sort();

	driver->deleteAllDynamicLights();
	s32 index;
	if (m_global) {
		s32 global = driver->addDynamicLight(m_global->getLightData());
		driver->turnLightOn(global, true);
	}
	u32 numLights = std::min(sortingArr.size(), driver->getMaximalDynamicLightAmount() - 1);

	for (i = 0; i < numLights; ++i) {
		if (!sortingArr[i].node) continue;
		ILightSceneNode* node = sortingArr[i].node;

		index = driver->addDynamicLight(node->getLightData());
		driver->turnLightOn(index, true);
	}
}