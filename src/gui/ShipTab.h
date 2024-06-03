#pragma once
#ifndef SHIPTAB_H
#define SHIPTAB_H
#include "BaseHeader.h"
#include "LoadoutTab.h"
#include "ShipInstance.h"

class ShipTab : public LoadoutTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
		bool onChangeWep(const SEvent& event);
		bool onWeaponLoadout(const SEvent& event);
		bool onReload(const SEvent& event);
		bool onUpgrade(const SEvent& event);
		bool onShipSelect(const SEvent& event);
		bool onWepSelect(const SEvent& event);
		bool onUpgradeSelect(const SEvent& event);
		bool onRepair(const SEvent& event);
		bool onStrip(const SEvent& event);
		bool onViewShipList(const SEvent& event);
		virtual void show();

		void displayShip(ShipInstance* inst);
	private:
		void m_hideSelects();
		void m_displayShipList();
		void m_displayWeaponList(HARDPOINT_TYPE type=HRDP_REGULAR);
		void m_displayWeapon(s32 pos, WeaponInstance* inst);

		struct _weaponSelect {
			IGUIButton* wep;
			IGUIButton* loadout;
			IGUIButton* reload;
			void hide() {
				if(wep) wep->setVisible(false);
				if(loadout) loadout->setVisible(false);
				if(reload) reload->setVisible(false);
			}
			void show() {
				if(wep) wep->setVisible(true);
				if(loadout) loadout->setVisible(true);
				if(reload) reload->setVisible(true);
			}
		};
		_weaponSelect reg[MAX_HARDPOINTS];
		_weaponSelect phys;
		_weaponSelect heavy;
		IGUIButton* repair;
		IGUIButton* strip;
		IGUIButton* viewShipList;
		IGUIButton* upgrades[MAX_SHIP_UPGRADES];
		IGUIImage* flavorBg;
		ShipInstance* currentShip = nullptr;
		u32 currentSlot;
};
#endif 