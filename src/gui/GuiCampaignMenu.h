#pragma once
#ifndef CAMPAIGNMENU_H
#define CAMPAIGNMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"
#include "Campaign.h"
/*
* The GuiCampaignMenu has the data and the functions to handle the campaign interface. It allows you to select a scenario from a list,
* shows you some basic info about the scenario, and it will allow you to mess with your current "inventory" as well as loadouts for yourself
* and your wingmen.
*/
class GuiCampaignMenu : public GuiDialog
{
	public:
		GuiCampaignMenu() : GuiDialog() {}
		virtual void init();
		bool onShowScenarioInfo(const SEvent& event);
		bool advanceConfirm(const SEvent& event);

		bool onSteven(const SEvent& event);
		bool onKate(const SEvent& event);
		virtual void show();
		void deselect();
		u32 showing = -1;
	private:
		bool sectorInfoShowing = false;
		void m_showShipInfo();
		//IGUIButton* launchButton;
		//IGUIStaticText* name;
		IGUIStaticText* desc;
		IGUIButton* advance;
		IGUIButton* scenarioSelects[NUM_SCENARIO_OPTIONS];
		IGUIImage* carrierSprite;
		IGUIImage* barTracker[8];
		IGUIStaticText* ammo;
		IGUIStaticText* supplies;
		IGUIStaticText* detection;
		IGUIImage* bg;
		IGUIButton* steven;
		IGUIButton* kate;
		NavMenu nav;
};
#endif 