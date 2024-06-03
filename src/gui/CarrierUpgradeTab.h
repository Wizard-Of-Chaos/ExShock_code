#pragma once
#ifndef CARRIERUPGRADETAB_H
#define CARRIERUPGRADETAB_H
#include "BaseHeader.h"
#include "FabMenuTab.h"

class CarrierUpgradeTab : public FabMenuTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
		bool onConfirm(const SEvent& event);
		bool onUpgradeSelect(const SEvent& event);
		virtual void show();
	private:
		std::string currentSelection = "";

		IGUIStaticText* upgradeBlocksBg = nullptr;
		std::vector<IGUIImage*> blocks;

		void m_buildUpgradeList();
		std::string m_carrUpgradeStr(std::string which);
		void m_buildBlocks();
};

#endif 