#pragma once
#ifndef BAEDSLIGHTMANAGER_H
#define BAEDSLIGHTMANAGER_H
#include <irrlicht.h>
#include <iostream>
#include "BaseHeader.h"

class BaedsLights : public ILightManager
{
public:
	virtual void OnRenderPassPreRender(E_SCENE_NODE_RENDER_PASS renderPass) { m_currentPass = renderPass; };
	virtual void OnPreRender(core::array<ISceneNode*>& lightList) { m_list = &lightList; }
	virtual void OnPostRender() {
		for (u32 i = 0; i < m_list->size(); ++i) {
			(*m_list)[i]->setVisible(true);
		}
	}
	virtual void OnNodePreRender(ISceneNode* node) override;
	virtual void OnNodePostRender(ISceneNode* node) {
		current = nullptr;
	}
	ISceneNode* currentNode() { return current; }
	ILightSceneNode* global() { return m_global; }
	void setGlobal(ILightSceneNode* global) { m_global = global; }
private:
	E_SCENE_NODE_RENDER_PASS m_currentPass;
	core::array<ISceneNode*>* m_list;
	ISceneNode* current = nullptr;
	ILightSceneNode* m_global = nullptr;
};

#endif 