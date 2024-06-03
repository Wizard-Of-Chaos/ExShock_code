#pragma once
#ifndef CARRIERTAB_H
#define CARRIERTAB_H
#include "BaseHeader.h"
#include "LoadoutTab.h"

class CarrierTab : public LoadoutTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
		virtual void show();
		bool onHover(const SEvent& event);
	private:
		void m_buildUpgradeList();
};
#endif 