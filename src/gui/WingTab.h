#pragma once
#ifndef WINGTAB_H
#define WINGTAB_H
#include "BaseHeader.h"
#include "LoadoutTab.h"

class WingTab : public LoadoutTab
{
public:
	WingTab() : LoadoutTab() {}
	virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
	bool onChangeWingman(const SEvent& event);
	bool onChangeShip(const SEvent& event);
	bool onShipLoadout(const SEvent& event);
	bool onRepair(const SEvent& event);
	bool onReload(const SEvent& event);

	bool wingmanSelect(const SEvent& event);
	bool shipSelect(const SEvent& event);
	virtual void show();
private:
	void m_hideAll();
	bool m_hoverWingman(s32 id, bool full = false);
	bool m_hoverShip(s32 id, bool full = false) override;
	void m_showWingList();
	void m_showShipList();
	bool m_checkAmmo(instId ship);
	void m_displayCurrentWing();
	struct _wingSelect {
		IGUIButton* name;
		IGUIButton* ship;
		IGUIButton* load;
		IGUIButton* repair;
		IGUIButton* reload;
		void setShipVis(bool val) {
			load->setVisible(val);
		}
		void setAllVis(bool val) {
			name->setVisible(val);
			ship->setVisible(val);
			load->setVisible(val);
			repair->setVisible(val);
			reload->setVisible(val);
		}
	};
	_wingSelect wingButtons[4];
	s32 currentSlot;

	IGUIImage* wingDescBg;
	IGUIStaticText* wingDescName;
	IGUIStaticText* wingDescPersonality;
	IGUIStaticText* wingDescKills;
	IGUIStaticText* wingDescInjuries;
	IGUIImage* resolveBar;
	IGUIImage* aimBar;
	IGUIImage* aggroBar;
	IGUIImage* reflexBar;
};

#endif 