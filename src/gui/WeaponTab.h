#pragma once
#ifndef WEAPONTAB_H
#define WEAPONTAB_H
#include "BaseHeader.h"
#include "LoadoutTab.h"
#include "ShipInstance.h"

class WeaponUpgradeInstance;

class WeaponTab : public LoadoutTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
		bool onChangeUpgrade(const SEvent& event);
		bool onViewWepList(const SEvent& event);
		bool onWepSelect(const SEvent& event);
		bool onUpgradeSelect(const SEvent& event);
		bool onReload(const SEvent& event);
		bool onStrip(const SEvent& event);
		WeaponInstance* curWep = nullptr;
		void displayWeapon(WeaponInstance* inst);
	private:
		void m_toggleSelects(bool vis);
		void m_buildCurWepList();
		IGUIStaticText* curWepTitle;
		IGUIStaticText* wepDesc;
		IGUIButton* reload;
		IGUIButton* strip;
		IGUIStaticText* ammo;
		IGUIButton* upgradeSlots[MAX_WEP_UPGRADES];
		u32 currentSlot = -1;
		HARDPOINT_TYPE curType = HRDP_REGULAR;
		IGUIButton* reg;
		IGUIButton* heavy;
		IGUIButton* phys;
		IGUIImage* wepBg;
		bool reloadGreyed = false;
};
#endif 