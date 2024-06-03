#pragma once
#ifndef SHIPBUILDTAB_H
#define SHIPBUILDTAB_H
#include "BaseHeader.h"
#include "FabMenuTab.h"

struct ShipData;

class ShipBuildTab : public FabMenuTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
		bool onScrapShipSelect(const SEvent& event);
		bool onBuildShipSelect(const SEvent& event);
		bool onConfirm(const SEvent& event);
		virtual void show();
		virtual void hide();
	private:
		s32 currentSelection = -1;

		IGUIStaticText* name = nullptr;
		IGUIStaticText* statsNames = nullptr;
		IGUIStaticText* statsNumbers = nullptr;
		IGUIStaticText* description = nullptr;
		IGUIImage* shipImage = nullptr;
		IGUIStaticText* lineBase = nullptr;
		IMeshSceneNode* rotatingShip = nullptr;
		IMesh* shipMesh = nullptr;
		ICameraSceneNode* camera = nullptr;

		vector3df shipPos;
		void m_buildHangarShipList();
		void m_buildBuildableShipList();
		void m_setText(ShipData* dat);
		void m_cleanMeshes();
};

#endif 