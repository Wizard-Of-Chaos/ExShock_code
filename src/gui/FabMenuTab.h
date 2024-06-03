#pragma once
#ifndef FABMENUTAB_H
#define FABMENUTAB_H
#include "BaseHeader.h"
#include "LoadoutTab.h"

class GuiFabMenu;

class FabMenuTab : public LoadoutTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which) = 0;
		virtual void show();
		virtual void m_showSupplies();

	protected:
		virtual void m_basebg(IGUIElement* root) override;
		IGUIButton* confirm = nullptr;
		GuiFabMenu* fab = nullptr;
};
#endif 