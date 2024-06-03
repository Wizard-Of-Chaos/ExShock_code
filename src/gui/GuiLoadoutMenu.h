#pragma once
#ifndef LOADOUTMENU_H
#define LOADOUTMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"
#include "WingTab.h"
#include "ShipTab.h"
#include "WeaponTab.h"
#include "CarrierTab.h"

class GuiLoadoutMenu : public GuiDialog
{
	public:
		virtual void init();
		bool onWing(const SEvent& event);
		bool onShips(const SEvent& event);
		bool onWeps(const SEvent& event);
		bool onCarrier(const SEvent& event);

		bool onMartin(const SEvent& event);
		void showSupplies();
		virtual void hide();
		virtual void show();
		LoadoutTab* curTab;

		WingTab wingTab;
		ShipTab shipTab;
		WeaponTab wepTab;
		CarrierTab carrierTab;
	private:
		IGUIButton* wing;
		IGUIButton* ships;
		IGUIButton* weps;
		IGUIButton* carrier;
		IGUIButton* martin;

		IGUIStaticText* supplyBg = nullptr;
		IGUIStaticText* supplyCount = nullptr;
		IGUIStaticText* ammoCount = nullptr;

		NavMenu nav;
};
#endif 