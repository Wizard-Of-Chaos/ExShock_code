#pragma once
#ifndef GUILOOTMENU_H
#define GUILOOTMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"

/*
* The loot menu displays at the end of a scenario and acts as a buffer between the campaign and the scenario. It does nothing except show you
* the loot that you got from that scenario; weapons, ships, supplies, etc.
*/
class GuiLootMenu : public GuiDialog
{
	public:
		GuiLootMenu() : GuiDialog() {}
		virtual void init();
		bool onReturnToCampaign(const SEvent& event);
		bool onShow(f32 dt);
		bool onHide(f32 dt);
		virtual void show();
	private:
		f32 timer;
		IGUIStaticText* loot;
		IGUIImage* screen;
		IGUIButton* returnToCampaign;
};

#endif 