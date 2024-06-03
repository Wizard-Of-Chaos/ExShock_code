#pragma once
#ifndef GUIFABMENU_H
#define GUIFABMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"
#include "ShipBuildTab.h"
#include "WeaponBuildTab.h"
#include "UpgradeTab.h"
#include "CarrierUpgradeTab.h"

class GuiFabMenu : public GuiDialog
{
	public:
		virtual void init();
		virtual void show();
		virtual void hide();

		bool onShips(const SEvent& event);
		bool onWeps(const SEvent& event);
		bool onUpgrades(const SEvent& event);
		bool onCarrier(const SEvent& event);
		bool onBuild(const SEvent& event);
		bool onScrap(const SEvent& event);
		const bool& isBuilding() const { return this->building; }
		bool onStripUnassigned(const SEvent& event);
		void showSupplies();
		bool onAmmo(const SEvent& event);


	private:
		NavMenu nav;
		IGUIButton* ships;
		IGUIButton* weps;
		IGUIButton* upgrades;
		IGUIButton* carrierUpgrades;
		IGUIButton* build;
		IGUIButton* scrap;
		IGUIButton* ammo = nullptr;
		IGUIButton* stripUnassigned = nullptr;

		FabMenuTab* curTab = nullptr;
		bool building = true;

		ShipBuildTab shipBuild;
		WeaponBuildTab wepBuild;
		UpgradeTab upgradeBuild;
		CarrierUpgradeTab carrierBuild;

		IGUIStaticText* supplyBg = nullptr;
		IGUIStaticText* supplyCount = nullptr;
		IGUIStaticText* ammoCount = nullptr;
};

#endif 