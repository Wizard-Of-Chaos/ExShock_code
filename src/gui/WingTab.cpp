#include "WingTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include "GameController.h"
#include "LoadoutData.h"
#include "ShipInstance.h"
#include "ShipUpgrades.h"
#include <iostream>
#include <sstream>
#include <string>

void WingTab::show()
{
	LoadoutTab::show();
	m_clearList();
	m_displayCurrentWing();
	//set up other crap
}
void WingTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE menu)
{
	dialog = dial;
	which = menu;
	longDescSize.Height = 156;
	m_basebg(root);
	setUITextSmall(longdesc);
	wingDescBg = guienv->addImage(rect<s32>(descPos, descSize), base);
	scaleAlign(wingDescBg);
	wingDescBg->setImage(driver->getTexture("assets/ui/wingdesc_bg.png"));
	wingDescName = guienv->addStaticText(L"", recti(vector2di(0,0), dimension2du(110,24)), false, true, wingDescBg);
	setAltUIText(wingDescName);
	wingDescPersonality = guienv->addStaticText(L"", recti(vector2di(0, 24), dimension2du(110, 41)), false, true, wingDescBg);
	setUIText(wingDescPersonality);
	wingDescKills = guienv->addStaticText(L"", recti(vector2di(30, 65), dimension2du(55, 24)), false, true, wingDescBg);
	setUITextSmall(wingDescKills);
	wingDescKills->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	wingDescInjuries = guienv->addStaticText(L"", recti(vector2di(95, 65), dimension2du(55, 24)), false, true, wingDescBg);
	setUITextSmall(wingDescInjuries);
	wingDescInjuries->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	IGUIImage* stat = guienv->addImage(recti(position2di(10, 65), dimension2du(20, 20)), wingDescBg);
	stat->setImage(driver->getTexture("assets/ui/kills.png"));
	scaleAlign(stat);
	stat = guienv->addImage(recti(position2di(75, 65), dimension2du(20, 20)), wingDescBg);
	stat->setImage(driver->getTexture("assets/ui/injuries.png"));
	scaleAlign(stat);

	stat = guienv->addImage(recti(position2di(150, 2), dimension2du(20, 20)), wingDescBg);
	stat->setImage(driver->getTexture("assets/ui/aggro.png"));
	IGUIStaticText* txt = guienv->addStaticText(L"Aggro", recti(position2di(110, 2), dimension2du(40, 20)), false, true, wingDescBg);
	setAltUITextSmall(txt);
	txt->setTextAlignment(EGUIA_LOWERRIGHT, EGUIA_CENTER);
	scaleAlign(stat);
	stat = guienv->addImage(recti(position2di(150, 24), dimension2du(20, 20)), wingDescBg);
	stat->setImage(driver->getTexture("assets/ui/aim.png"));
	txt = guienv->addStaticText(L"Aim", recti(position2di(110, 24), dimension2du(40, 20)), false, true, wingDescBg);
	setAltUITextSmall(txt);
	txt->setTextAlignment(EGUIA_LOWERRIGHT, EGUIA_CENTER);
	scaleAlign(stat);
	stat = guienv->addImage(recti(position2di(150, 46), dimension2du(20, 20)), wingDescBg);
	stat->setImage(driver->getTexture("assets/ui/reflex.png"));
	txt = guienv->addStaticText(L"Reflex", recti(position2di(110, 46), dimension2du(40, 20)), false, true, wingDescBg);
	setAltUITextSmall(txt);
	txt->setTextAlignment(EGUIA_LOWERRIGHT, EGUIA_CENTER);
	scaleAlign(stat);
	stat = guienv->addImage(recti(position2di(150, 68), dimension2du(20, 20)), wingDescBg);
	stat->setImage(driver->getTexture("assets/ui/resolve.png"));
	txt = guienv->addStaticText(L"Resolve", recti(position2di(110, 68), dimension2du(40, 20)), false, true, wingDescBg);
	setAltUITextSmall(txt);
	txt->setTextAlignment(EGUIA_LOWERRIGHT, EGUIA_CENTER);
	scaleAlign(stat);

	aggroBar = guienv->addImage(recti(position2di(170, 4), dimension2du(120, 16)), wingDescBg);
	aggroBar->setImage(driver->getTexture("assets/ui/bars/green.png"));
	scaleAlign(aggroBar);

	recti capPosL(position2di(-2, 0), dimension2du(4, 16));
	recti capPosR(position2di(118, 0), dimension2du(4, 16));
	IGUIImage* cap = guienv->addImage(capPosL, aggroBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_l.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);
	cap = guienv->addImage(capPosR, aggroBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_r.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);

	aimBar = guienv->addImage(recti(position2di(170, 26), dimension2du(120, 16)), wingDescBg);
	aimBar->setImage(driver->getTexture("assets/ui/bars/yellowgreen.png"));
	scaleAlign(aimBar);

	cap = guienv->addImage(capPosL, aimBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_l.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);
	cap = guienv->addImage(capPosR, aimBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_r.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);

	reflexBar = guienv->addImage(recti(position2di(170, 48), dimension2du(120, 16)), wingDescBg);
	reflexBar->setImage(driver->getTexture("assets/ui/bars/yellow.png"));
	scaleAlign(reflexBar);

	cap = guienv->addImage(capPosL, reflexBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_l.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);
	cap = guienv->addImage(capPosR, reflexBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_r.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);

	resolveBar = guienv->addImage(recti(position2di(170, 70), dimension2du(120, 16)), wingDescBg);
	resolveBar->setImage(driver->getTexture("assets/ui/bars/red.png"));
	scaleAlign(resolveBar);

	cap = guienv->addImage(capPosL, resolveBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_l.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);
	cap = guienv->addImage(capPosR, resolveBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_r.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);

	for (u32 i = 0; i < (MAX_WINGMEN_ON_WING +1); ++i) {
		_wingSelect wing;
		dimension2du size(100, 20);
		std::wstring name = L"";
		if (i == 0) name = L"YOU";
		wing.name = guienv->addButton(rect<s32>(position2di(8, 15 + (size.Height+10) * i), size), base, i, name.c_str(), L"Name of this wingman.");
		wing.ship = guienv->addButton(rect<s32>(position2di(8 + (size.Width + 2), 15 + (size.Height+10) * i), size), base, i, L"Ship", L"Ship available for this wingman.");
		wing.load = guienv->addButton(rect<s32>(position2di(8 + (size.Width + 2) * 2, 15 + (size.Height+10) * i), dimension2du(20, 20)), base, i, L"", L"Change loadout of this ship.");
		setThinHoloButton(wing.name, BCOL_BLUE);
		setThinHoloButton(wing.ship, BCOL_ORANGE);
		setLoadoutButton(wing.load);
		wing.repair = guienv->addButton(rect<s32>(position2di(8 + (size.Width + 2) * 2 + 21, 15 + (size.Height+10) * i), dimension2du(20, 20)), base, i, L"", L"Repair this ship.");
		setRepairButton(wing.repair);
		wing.reload = guienv->addButton(rect<s32>(position2di(8 + (size.Width + 2) * 2 + 42, 15 + (size.Height+10) * i), dimension2du(20, 20)), base, i, L"", L"Reload this ship's weapons.");
		setReloadButton(wing.reload);
		if (i != 0) {
			wing.ship->setVisible(false);
			wing.setShipVis(false);
		}
		guiController->setCallback(wing.name, std::bind(&WingTab::onChangeWingman, this, std::placeholders::_1), which);
		guiController->setCallback(wing.ship, std::bind(&WingTab::onChangeShip, this, std::placeholders::_1), which);
		guiController->setCallback(wing.load, std::bind(&WingTab::onShipLoadout, this, std::placeholders::_1), which);
		guiController->setCallback(wing.repair, std::bind(&WingTab::onRepair, this, std::placeholders::_1), which);
		guiController->setCallback(wing.reload, std::bind(&WingTab::onReload, this, std::placeholders::_1), which);

		wingButtons[i] = wing;
	}
	wingDescBg->setVisible(false);
	desc->setVisible(false);
	longdesc->setVisible(false);
	loadoutBottomShipView.hide();
}

void WingTab::m_hideAll()
{
	for (u32 i = 0; i < 4; ++i) {
		wingButtons[i].name->setVisible(false);
		wingButtons[i].ship->setVisible(false);
		wingButtons[i].repair->setVisible(false);
		wingButtons[i].reload->setVisible(false);
		wingButtons[i].setShipVis(false);
	}
	wingDescBg->setVisible(false);
	loadoutBottomShipView.hide();
	loadoutSideShipView.hide();
}

void WingTab::m_displayCurrentWing()
{
	for (u32 i = 0; i < (MAX_WINGMEN_ON_WING+1); ++i) {
		wingButtons[i].setAllVis(true);
	}
	wingButtons[0].ship->setText(wstr(shipData[campaign->getPlayerShip()->ship.shipDataId]->name).c_str());
	if (campaign->getPlayerShip()->hp.health >= campaign->getPlayerShip()->hp.maxHealth) {
		wingButtons[0].repair->setVisible(false);
	}
	wingButtons[0].reload->setVisible(m_checkAmmo(campaign->getPlayerShip()->id));

	for (u32 i = 1; i < (MAX_WINGMEN_ON_WING + 1); ++i) {
		if (campaign->getAssignedWingman(i - 1)) {
			std::stringstream s(campaign->getAssignedWingman(i - 1)->name);
			std::string firstname;
			std::getline(s, firstname, ' ');
			wingButtons[i].name->setText(wstr(firstname).c_str());
		}
		else {
			wingButtons[i].name->setText(L"");
			wingButtons[i].ship->setVisible(false);
			wingButtons[i].setShipVis(false);
		}
		if (campaign->getAssignedShip(i - 1)) {
			wingButtons[i].ship->setText(wstr(shipData[campaign->getAssignedShip(i - 1)->ship.shipDataId]->name).c_str());
			if (campaign->getAssignedShip(i - 1)->hp.health >= campaign->getAssignedShip(i - 1)->hp.maxHealth) {
				wingButtons[i].repair->setVisible(false);
			}

			wingButtons[i].reload->setVisible(m_checkAmmo(campaign->getAssignedShip(i - 1)->id));
		}
		else {
			wingButtons[i].ship->setText(L"");
			wingButtons[i].setShipVis(false);
			wingButtons[i].repair->setVisible(false);
			wingButtons[i].reload->setVisible(false);
		}

	}
}

bool WingTab::m_hoverWingman(s32 id, bool full)
{
	WingmanInstance* mans = nullptr;
	if (id == 0) mans = campaign->getPlayer();
	else mans = campaign->getWingman(id);
	if (!mans) return true;
	wingDescBg->setVisible(true);
	loadoutBottomShipView.hide();
	if (full) {
		longdesc->setText(wstr(mans->description).c_str());
	}
	std::string upperName = mans->name;
	std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
	wingDescName->setText(wstr(upperName).c_str());
	wingDescPersonality->setText(wstr(mans->personality).c_str());
	wingDescKills->setText(wstr(std::to_string(mans->totalKills)).c_str());
	wingDescInjuries->setText(wstr(std::to_string(mans->totalInjuries)).c_str());

	dimension2du scrn = driver->getScreenSize();
	dimension2du barBaseSize = dimension2du((95.f/960.f) * scrn.Width, (16.f/540.f)*scrn.Height);
	dimension2du aggroSize = barBaseSize;
	aggroSize.Width = (id == 0) ?  barBaseSize.Width : (mans->ai.aggressiveness / 2.f) * barBaseSize.Width;
	if (aggroSize.Width > barBaseSize.Width) aggroSize.Width = barBaseSize.Width;
	aggroBar->setRelativePosition(recti(aggroBar->getRelativePosition().UpperLeftCorner, aggroSize));
	
	dimension2du resolveSize = barBaseSize;
	resolveSize.Width = (id == 0) ? barBaseSize.Width : (mans->ai.resolve / 1.f) * barBaseSize.Width;
	if (resolveSize.Width > barBaseSize.Width) resolveSize.Width = barBaseSize.Width;
	resolveBar->setRelativePosition(recti(resolveBar->getRelativePosition().UpperLeftCorner, resolveSize));

	dimension2du reflexSize = barBaseSize;
	reflexSize.Width = (id == 0) ? barBaseSize.Width : ((2.f - mans->ai.reactionSpeed) / 2.f) * barBaseSize.Width;
	if (reflexSize.Width > barBaseSize.Width) reflexSize.Width = barBaseSize.Width;
	reflexBar->setRelativePosition(recti(reflexBar->getRelativePosition().UpperLeftCorner, reflexSize));

	dimension2du aimSize = barBaseSize;
	aimSize.Width = (id == 0) ? barBaseSize.Width : (mans->ai.aim / 8.f) * barBaseSize.Width;
	if (aimSize.Width > barBaseSize.Width) aimSize.Width = barBaseSize.Width;
	aimBar->setRelativePosition(recti(aimBar->getRelativePosition().UpperLeftCorner, aimSize));

	struct _sorter {
		_sorter() {};
		_sorter(IGUIImage* i, dimension2du d) : img(i), dim(d) {}
		IGUIImage* img = 0;
		dimension2du dim;
		bool operator < (const _sorter& other) const {
			return (dim.Width < other.dim.Width);
		}
	};
	array<_sorter> sortarr;
	sortarr.push_back(_sorter(aggroBar, aggroSize));
	sortarr.push_back(_sorter(resolveBar, resolveSize));
	sortarr.push_back(_sorter(reflexBar, reflexSize));
	sortarr.push_back(_sorter(aimBar, aimSize));
	sortarr.sort();

	sortarr[3].img->setImage(driver->getTexture("assets/ui/bars/green.png"));
	sortarr[2].img->setImage((id == 0) ? driver->getTexture("assets/ui/bars/green.png") : driver->getTexture("assets/ui/bars/yellowgreen.png"));
	sortarr[1].img->setImage((id == 0) ? driver->getTexture("assets/ui/bars/green.png") : driver->getTexture("assets/ui/bars/yellow.png"));
	sortarr[0].img->setImage((id == 0) ? driver->getTexture("assets/ui/bars/green.png") : driver->getTexture("assets/ui/bars/red.png"));

	return false;
}

bool WingTab::m_hoverShip(s32 id, bool full)
{
	wingDescBg->setVisible(false);
	ShipInstance* inst = nullptr;
	inst = campaign->getShip(id);
	if (!inst) return true;
	ShipData* dat = shipData.at(inst->ship.shipDataId);
	if (full) loadoutSideShipView.showShip(inst);
	else loadoutBottomShipView.showShip(inst);

	return false;
}

bool WingTab::onChangeWingman(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	s32 pos = event.GUIEvent.Caller->getID();
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		if (pos == 0) return m_hoverWingman(0);
		if (campaign->getAssignedWingman(pos - 1)) return m_hoverWingman(campaign->getAssignedWingman(pos - 1)->id);
		return m_hoverWingman(-1);
	}
	if (pos == 0) return true;

	m_hideAll();
	m_showWingList();
	currentSlot = pos - 1;

	return false;
}
bool WingTab::wingmanSelect(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		return m_hoverWingman(event.GUIEvent.Caller->getID(), true);
	}
	s32 id = event.GUIEvent.Caller->getID();
	if (id == -5) {
		if (campaign->getAssignedWingman(currentSlot)) campaign->removeAssignedWingman(currentSlot);
	}
	else campaign->setAssignedWingman(campaign->getWingman(id), currentSlot);

	longdesc->setText(L"");
	m_clearList();
	m_displayCurrentWing();
	return false;
}

bool WingTab::shipSelect(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		return m_hoverShip(event.GUIEvent.Caller->getID(), true);
	}
	s32 id = event.GUIEvent.Caller->getID();
	if (id == -5) {
		if (currentSlot == -1) {
			guiController->setOkPopup("",
				"You can't de-select your own ship! How are you supposed to fly without one?", "Wings?");
			guiController->showOkPopup();
		}
		else campaign->removeAssignedShip(currentSlot);
	}
	else if (currentSlot == -1) {
		campaign->setAssignedShip(campaign->getShip(id), currentSlot);
		campaign->assignWingmanToShip(campaign->getPlayer(), campaign->getShip(id));
	}
	else {
		campaign->setAssignedShip(campaign->getShip(id), currentSlot);
		campaign->assignWingmanToShip(campaign->getAssignedWingman(currentSlot), campaign->getShip(id));
	}
	loadoutSideShipView.hide();
	loadoutBottomShipView.hide();
	wingDescBg->setVisible(false);
	longdesc->setText(L"");
	m_clearList();
	m_displayCurrentWing();
	return false;
}

bool WingTab::onChangeShip(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	s32 pos = event.GUIEvent.Caller->getID();

	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		if (pos == 0) return m_hoverShip(campaign->getPlayerShip()->id);
		else if (campaign->getAssignedShip(pos - 1)) return m_hoverShip(campaign->getAssignedShip(pos - 1)->id);
		return m_hoverShip(-1);
	}
	if (campaign->getAssignedWingman(pos-1)) {
		if (campaign->getAssignedWingman(pos-1)->id == 5) {
			guiController->setOkPopup("",
				"ARTHUR and his ship cannot be separated.", "Aww...");
			guiController->showOkPopup();
			return false;
		}
	}
	desc->setText(L"");
	wingDescBg->setVisible(false);
	m_hideAll();
	m_showShipList();
	currentSlot = pos - 1;
	return false;
}
bool WingTab::onShipLoadout(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;

	hide();
	GuiLoadoutMenu* menu = (GuiLoadoutMenu*)dialog;
	menu->curTab = &menu->shipTab;
	menu->shipTab.show();
	ShipInstance* inst = nullptr;
	if (event.GUIEvent.Caller->getID() == 0) inst = campaign->getPlayerShip();
	else inst = campaign->getAssignedShip(event.GUIEvent.Caller->getID() - 1);
	menu->shipTab.displayShip(inst);

	return false;
}

bool WingTab::m_checkAmmo(instId ship)
{
	if (ship <= -1) return false;
	ShipInstance* inst = campaign->getShip(ship);
	for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
		if (inst->weps[i] <= -1) continue;
		auto wep = campaign->getWeapon(inst->weps[i]);
		if (wep->wep.usesAmmunition && wep->fire.ammunition < wep->wep.maxAmmunition) return true;
	}
	if (inst->physWep > -1) {
		auto wep = campaign->getWeapon(inst->physWep);
		if (wep->wep.usesAmmunition && wep->fire.ammunition < wep->wep.maxAmmunition) return true;
	}
	if (inst->heavyWep > -1) {
		auto wep = campaign->getWeapon(inst->heavyWep);
		if (wep->wep.usesAmmunition && wep->fire.ammunition < wep->wep.maxAmmunition) return true;
	}
	return false;
}

bool WingTab::onRepair(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (campaign->getFlag(L"MARTIN_BUILDING_CLOAK")) {
		guiController->setOkPopup("Locked", "Repairs aren't possible right now. Martin and his team are too busy with the cloak.");
		guiController->showOkPopup();
		m_displayCurrentWing();
		m_showSupplies();
		return false;
	}
	ShipInstance* inst = nullptr;
	if(id==0) inst = campaign->getPlayerShip();
	else inst = campaign->getAssignedShip(id - 1);

	f32 amt = inst->hp.maxHealth - inst->hp.health;
	f32 supplyCost = amt / 2.f;
	f32 usedSupply = campaign->removeSupplies(supplyCost);
	inst->hp.health += supplyCost * 2.f;
	if (supplyCost != usedSupply) {
		guiController->setOkPopup("Out of Supplies", "You're out of supplies to fully repair this ship. Repairs have been made with as much as is available.", "Ah, hell");
		guiController->showOkPopup();
	}
	m_displayCurrentWing();
	m_showSupplies();
	return false;
}

bool WingTab::onReload(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	ShipInstance* inst = nullptr;
	if (id == 0) inst = campaign->getPlayerShip();
	else inst = campaign->getAssignedShip(id - 1);

	u32 totalAmmoRequired = 0;
	for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
		if (inst->weps[i] <= -1) continue;
		auto wep = campaign->getWeapon(inst->weps[i]);
		if (wep->wep.usesAmmunition && wep->fire.ammunition < wep->wep.maxAmmunition) {
			totalAmmoRequired += (wep->wep.maxAmmunition - wep->fire.ammunition) / wep->wep.maxClip;
		}
	}
	if (inst->physWep > -1) {
		auto wep = campaign->getWeapon(inst->physWep);
		if (wep->wep.usesAmmunition && wep->fire.ammunition < wep->wep.maxAmmunition) {
			totalAmmoRequired += (wep->wep.maxAmmunition - wep->fire.ammunition) / wep->wep.maxClip;
		}
	}
	if (inst->heavyWep > -1) {
		auto wep = campaign->getWeapon(inst->heavyWep);
		if (wep->wep.usesAmmunition && wep->fire.ammunition < wep->wep.maxAmmunition) {
			totalAmmoRequired += (wep->wep.maxAmmunition - wep->fire.ammunition) / wep->wep.maxClip;
		}
	}

	if (totalAmmoRequired > campaign->getAmmo()) {
		guiController->setOkPopup("Insufficient Ammo", "You don't have the ammo to reload every single weapon on this ship. Go to the ship tab and be a bit more picky this time.");
		guiController->showOkPopup();
		m_displayCurrentWing();
		m_showSupplies();
		return false;
	}
	for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
		if (inst->weps[i] <= -1) continue;
		auto wep = campaign->getWeapon(inst->weps[i]);
		if (wep->wep.usesAmmunition) wep->fire.ammunition = wep->wep.maxAmmunition;

	}
	if (inst->physWep > -1) {
		auto wep = campaign->getWeapon(inst->physWep);
		if (wep->wep.usesAmmunition) wep->fire.ammunition = wep->wep.maxAmmunition;

	}
	if (inst->heavyWep > -1) {
		auto wep = campaign->getWeapon(inst->heavyWep);
		if (wep->wep.usesAmmunition) wep->fire.ammunition = wep->wep.maxAmmunition;
	}
	campaign->removeAmmo(totalAmmoRequired);
	m_displayCurrentWing();
	m_showSupplies();
	return false;
}

void WingTab::m_showWingList()
{
	auto none = guienv->addButton(rect<s32>(m_listItemStart(), m_listItemSize()), base, -5, L"None", L"Deselect wingman.");
	setThinHoloButton(none, BCOL_RED);
	guiController->setCallback(none, std::bind(&WingTab::wingmanSelect, this, std::placeholders::_1), which);
	listItems.push_back(none);
	u32 count = 1;
	for (auto man : campaign->wingmen()) {
		if (man->injured) continue;
		if (man->assigned) continue;
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), base, man->id, wstr(man->name).c_str(), L"Select this wingman.");
		setThinHoloButton(button, BCOL_BLUE);
		guiController->setCallback(button, std::bind(&WingTab::wingmanSelect, this, std::placeholders::_1), which);
		listItems.push_back(button);
	}
}

void WingTab::m_showShipList()
{
	m_buildShipList(std::bind(&WingTab::shipSelect, this, std::placeholders::_1));
}