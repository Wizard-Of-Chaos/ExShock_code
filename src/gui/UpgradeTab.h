#pragma once
#ifndef UPGRADETAB_H
#define UPGRADETAB_H
#include "BaseHeader.h"
#include "FabMenuTab.h"

class UpgradeTab : public FabMenuTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
		bool onConfirm(const SEvent& event);
		bool onUpgradeScrapSelect(const SEvent& event);
		bool onUpgradeBuildSelect(const SEvent& event);
		bool onIncrement(const SEvent& event);
		bool onDecrement(const SEvent& event);
		bool onShip(const SEvent& event);
		bool onWeapon(const SEvent& event);
		virtual void show();
	private:
		bool showingShips = true;
		s32 currentSelection = -1;
		IGUIStaticText* costDisplay = nullptr;
		IGUIStaticText* valDisplay = nullptr;
		IGUIStaticText* flavorText = nullptr;
		IGUIButton* shipList = nullptr;
		IGUIButton* wepList = nullptr;
		IGUIButton* increase = nullptr;
		IGUIButton* decrease = nullptr;
		IGUIStaticText* upgradeBlocksBg = nullptr;
		std::vector<IGUIImage*> blocks;
		f32 curVal = 0.f;
		f32 curCost = 0.f;
		u32 availableIncrements = 0; // just for blocks
		u32 currentIncrements = 0;

		void m_buildableShipUpgradesList();
		void m_buildableWepUpgradesList();
		void m_buildUpgradeList();
		void m_clear();
		void m_buildBlocks();
};

#endif 