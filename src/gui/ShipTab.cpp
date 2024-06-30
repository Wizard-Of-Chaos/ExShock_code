#include "ShipTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include "GameController.h"
#include "LoadoutData.h"
#include "ShipInstance.h"
#include "ShipUpgrades.h"
#include "WeaponUpgrades.h"
#include "AudioDriver.h"
#include <iostream>
#include <sstream>
#include <string>

void ShipTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE menu)
{
	dialog = dial;
	which = menu;
	dimension2du size(110, 20);
	dimension2du iconSize(20, 20);
	descPos.Y = 12 + (size.Height * (MAX_HARDPOINTS + 2));
	descSize.Height = 246 - descPos.Y;
	m_basebg(root);

	u32 widthbuf = (size.Width + 4);
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		_weaponSelect sel;
		u32 height = 6 + (size.Height * i);
		sel.wep = guienv->addButton(rect<s32>(position2di(8, height), size), base, i, L"", L"Weapon for this hardpoint.");
		setThinHoloButton(sel.wep, BCOL_BLUE);
		sel.loadout = guienv->addButton(rect<s32>(position2di(8 + widthbuf, height), iconSize), base, i, L"", L"Set upgrades for this weapon.");
		setLoadoutButton(sel.loadout);
		sel.reload = guienv->addButton(rect<s32>(position2di(8 + widthbuf + 22, height), iconSize), base, i, L"", L"Reload this weapon.");
		setReloadButton(sel.reload);
		guiController->setCallback(sel.wep, std::bind(&ShipTab::onChangeWep, this, std::placeholders::_1), which);
		guiController->setCallback(sel.reload, std::bind(&ShipTab::onReload, this, std::placeholders::_1), which);
		guiController->setCallback(sel.loadout, std::bind(&ShipTab::onWeaponLoadout, this, std::placeholders::_1), which);

		reg[i] = sel;
	}
	u32 height = 12 + (size.Height * MAX_HARDPOINTS);

	phys.wep = guienv->addButton(rect<s32>(position2di(8, height), size), base, PHYS_HARDPOINT, L"", L"Physical weapon hardpoint.");
	setThinHoloButton(phys.wep, BCOL_YELLOW);
	phys.loadout = guienv->addButton(rect<s32>(position2di(8 + widthbuf, height), iconSize), base, PHYS_HARDPOINT, L"", L"Set upgrades for this weapon.");
	setLoadoutButton(phys.loadout);

	repair = guienv->addButton(rect<s32>(position2di(102, height), dimension2du(80,20)), base, -1, L"", L"Repair this ship.");
	setThinHoloButton(repair, BCOL_GREEN);
	repair->setVisible(false);

	height += size.Height;
	heavy.wep = guienv->addButton(rect<s32>(position2di(8, height), size), base, HEAVY_HARDPOINT, L"", L"Heavy weapon hardpoint.");
	setThinHoloButton(heavy.wep, BCOL_RED);

	heavy.loadout = guienv->addButton(rect<s32>(position2di(8 + widthbuf, height), iconSize), base, HEAVY_HARDPOINT, L"", L"Set upgrades for this weapon.");
	setLoadoutButton(heavy.loadout);
	heavy.reload = guienv->addButton(rect<s32>(position2di(8 + widthbuf + 22, height), iconSize), base, HEAVY_HARDPOINT, L"", L"Reload the heavy weapon.");
	setReloadButton(heavy.reload);

	guiController->setCallback(phys.wep, std::bind(&ShipTab::onChangeWep, this, std::placeholders::_1), which);
	guiController->setCallback(heavy.wep, std::bind(&ShipTab::onChangeWep, this, std::placeholders::_1), which);
	guiController->setCallback(heavy.reload, std::bind(&ShipTab::onReload, this, std::placeholders::_1), which);
	guiController->setCallback(phys.loadout, std::bind(&ShipTab::onWeaponLoadout, this, std::placeholders::_1), which);
	guiController->setCallback(heavy.loadout, std::bind(&ShipTab::onWeaponLoadout, this, std::placeholders::_1), which);
	guiController->setCallback(repair, std::bind(&ShipTab::onRepair, this, std::placeholders::_1), which);

	auto upsize = dimension2du(120, 20);
	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		position2di pos(180, 6 + (upsize.Height*i));
		upgrades[i] = guienv->addButton(rect<s32>(pos, upsize), base, i, L"Empty", L"Select upgrades for this ship.");
		setThinHoloButton(upgrades[i], BCOL_LIGHTBLUE);
		guiController->setCallback(upgrades[i], std::bind(&ShipTab::onUpgrade, this, std::placeholders::_1), which);
	}

	strip = guienv->addButton(rect<s32>(position2di(200, height), dimension2du(100, 20)), base, -1, L"Strip All", L"Strip this ship of all weapons and upgrades.");
	setThinHoloButton(strip, BCOL_RED);
	strip->setVisible(false);
	guiController->setCallback(strip, std::bind(&ShipTab::onStrip, this, std::placeholders::_1), which);

	viewShipList = guienv->addButton(rect<s32>(position2di(200, height-30), dimension2du(100, 20)), base, -1, L"View Ship List", L"View a list of all ships.");
	setThinHoloButton(viewShipList, BCOL_GREEN);
	viewShipList->setVisible(true);
	guiController->setCallback(viewShipList, std::bind(&ShipTab::onViewShipList, this, std::placeholders::_1), which);
	loadoutBottomShipView.hide();
	desc->setVisible(false);

	flavorBg = guienv->addImage(recti(longDescPos, longDescSize), base);
	flavorBg->setImage(driver->getTexture("assets/ui/sidelistbg.png"));
	scaleAlign(flavorBg);

}
void ShipTab::show()
{
	m_hideSelects();
	m_displayShipList();
	loadoutBottomShipView.hide();
	flavorBg->setVisible(true);
	LoadoutTab::show();
}
bool ShipTab::onShipSelect(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	s32 id = event.GUIEvent.Caller->getID();
	flavorBg->setVisible(false);
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		//bool full = false;
		//if (!currentShip) full = true;
		m_hoverShip(id, true);
		return false;
	}
	if (id == -5) {
		currentShip = nullptr;
		longdesc->setText(L"");
		desc->setText(L"");
		m_hideSelects();
		return false;
	}
	currentShip = campaign->getShip(id);
	if (!currentShip) return true;
	longdesc->setText(L"");
	displayShip(currentShip);
	loadoutBottomShipView.hide();
	loadoutSideShipView.hide();

	return false;
}
void ShipTab::m_displayShipList()
{
	m_buildShipList(std::bind(&ShipTab::onShipSelect, this, std::placeholders::_1), false);
}
void ShipTab::m_displayWeaponList(HARDPOINT_TYPE type)
{
	m_buildWepList(std::bind(&ShipTab::onWepSelect, this, std::placeholders::_1), true, type);
}

bool ShipTab::onViewShipList(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	m_hideSelects();
	m_displayShipList();
	flavorBg->setVisible(false);
	return false;
}

void ShipTab::m_displayWeapon(s32 pos, WeaponInstance* inst)
{
	_weaponSelect* sel = nullptr;
	std::string name;
	if (pos < MAX_HARDPOINTS) {
		name = weaponData[inst->wep.wepDataId]->name;
		sel = &reg[pos];
	}
	else if (pos == PHYS_HARDPOINT) {
		name = physWeaponData[inst->wep.wepDataId]->name;
		sel = &phys;
	}
	else if (pos == HEAVY_HARDPOINT) {
		name = heavyWeaponData[inst->wep.wepDataId]->name;
		sel = &heavy;
	}
	else return; //something horrible happened

	sel->wep->setVisible(true);
	sel->wep->setText(wstr(name).c_str());
	sel->loadout->setVisible(true);
	if (inst->wep.usesAmmunition) {
		if (inst->fire.ammunition < inst->wep.maxAmmunition) {
			sel->reload->setVisible(true);
		}
	}

}

void ShipTab::displayShip(ShipInstance* inst)
{
	longdesc->setText(L"");
	m_clearList();
	m_hideSelects();
	//m_hoverShip(inst->id);
	for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
		if (inst->weps[i] > -1) m_displayWeapon(i, campaign->getWeapon(inst->weps[i]));
		else {
			reg[i].wep->setVisible(true);
			reg[i].wep->setText(L"");
		}
	}
	for (u32 i = inst->hards.hardpointCount; i < MAX_HARDPOINTS; ++i) {
		reg[i].hide();
	}
	if (inst->physWep > -1) m_displayWeapon(PHYS_HARDPOINT, campaign->getWeapon(inst->physWep));
	else {
		phys.wep->setVisible(true);
		phys.wep->setText(L"");
	}
	if (inst->heavyWep > -1) m_displayWeapon(HEAVY_HARDPOINT, campaign->getWeapon(inst->heavyWep));
	else {
		heavy.wep->setVisible(true);
		heavy.wep->setText(L"");
	}

	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		upgrades[i]->setVisible(true);
		if (inst->upgrades[i] > -1) {
			auto up = campaign->getShipUpgrade(inst->upgrades[i]);
			s32 upId = up->dataId;
			std::string name = shipUpgradeData.at(upId)->name;
			upgrades[i]->setText(wstr(name).c_str());
		} else {
			upgrades[i]->setText(L"No Upgrade");
		}
	}

	if (inst->hp.health < inst->hp.maxHealth) {
		std::wstring repstr = L"Repair " + wstr(fprecis(inst->hp.maxHealth - inst->hp.health, 5)) + L" HP";
		repair->setText(repstr.c_str());
		repair->setVisible(true);
	}
	else {
		repair->setVisible(false);
	}
	strip->setVisible(true);
	viewShipList->setVisible(true);
	loadoutBottomShipView.hide();
	loadoutSideShipView.hide();
	flavorBg->setVisible(false);

	//m_displayShipList();
	currentShip = inst;
}

void ShipTab::m_hideSelects()
{
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		reg[i].hide();
	}
	phys.hide();
	heavy.hide();
	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		upgrades[i]->setVisible(false);
	}
	repair->setVisible(false);
	strip->setVisible(false);
	viewShipList->setVisible(false);
}
bool ShipTab::onChangeWep(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	s32 id = event.GUIEvent.Caller->getID();
	HARDPOINT_TYPE type = HRDP_REGULAR;
	if (id == PHYS_HARDPOINT) type = HRDP_PHYSICS;
	else if (id == HEAVY_HARDPOINT) type = HRDP_HEAVY;

	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		s32 wepUId= -10;
		if (type == HRDP_PHYSICS && currentShip->physWep > -1) wepUId = campaign->getWeapon(currentShip->physWep)->id;
		else if (type == HRDP_HEAVY && currentShip->heavyWep > -1) wepUId = campaign->getWeapon(currentShip->heavyWep)->id;
		else if (id < MAX_HARDPOINTS && currentShip->weps[id] > -1) wepUId = campaign->getWeapon(currentShip->weps[id])->id;
		return m_hoverWeapon(wepUId);
	}

	m_hideSelects();
	m_clearList();
	m_displayWeaponList(type);
	flavorBg->setVisible(true);

	currentSlot = id;

	return false;
}
bool ShipTab::onWepSelect(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		return m_hoverWeapon(id, true);
	}
	WeaponInstance* wep = campaign->getWeapon(id);
	//if (!wep) return true; //something went really wrong

	if (currentSlot == PHYS_HARDPOINT) {
		if (currentShip->physWep >= 0) campaign->getWeapon(currentShip->physWep)->usedBy = -1;
		currentShip->physWep = id;
		if(currentShip->physWep >= 0) campaign->getWeapon(currentShip->physWep)->usedBy = currentShip->id;
	}
	else if (currentSlot == HEAVY_HARDPOINT) {
		if (currentShip->heavyWep >= 0) campaign->getWeapon(currentShip->heavyWep)->usedBy = -1;
		currentShip->heavyWep = id;
		if(currentShip->heavyWep >= 0) campaign->getWeapon(currentShip->heavyWep)->usedBy = currentShip->id;
	}
	else if (currentSlot >= 0 && currentSlot < MAX_HARDPOINTS) {
		bool valid = true;
		if (wep) {
			if (wep->wep.wepDataId == 30 && currentShip->ship.shipDataId != 5) {
				valid = false;
				guiController->setOkPopup("Regis Futuri", "Please leave this weapon in the stone for a more appropriate wielder.");
				guiController->showOkPopup();
			}
		}
		if (valid) {
			if (currentShip->weps[currentSlot] > -1) campaign->getWeapon(currentShip->weps[currentSlot])->usedBy = -1;
			currentShip->weps[currentSlot] = id;
			if (currentShip->weps[currentSlot] > -1) campaign->getWeapon(currentShip->weps[currentSlot])->usedBy = currentShip->id;
		}
	}
	if (wep)
		audioDriver->playMenuSound("fighter_equip.ogg");
	else
		audioDriver->playMenuSound("fighter_unequip.ogg");

	//if it's trash don't do anything
	displayShip(currentShip);
	flavorBg->setVisible(false);
	return false;
}

bool ShipTab::onUpgradeSelect(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		m_hoverShipUpgrade(id, true);
		return false;
	}
	ShipUpgradeInstance* inst = campaign->getShipUpgrade(id);
	campaign->assignShipUpgrade(currentShip, currentSlot, inst);
	displayShip(currentShip);
	return false;
}

bool ShipTab::onWeaponLoadout(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	this->hide();
	GuiLoadoutMenu* menu = (GuiLoadoutMenu*)dialog;
	menu->curTab = &menu->wepTab;
	menu->wepTab.show();

	u32 id = event.GUIEvent.Caller->getID();
	if (id == PHYS_HARDPOINT) menu->wepTab.curWep = campaign->getWeapon(currentShip->physWep);
	else if (id == HEAVY_HARDPOINT) menu->wepTab.curWep = campaign->getWeapon(currentShip->heavyWep);
	else menu->wepTab.curWep = campaign->getWeapon(currentShip->weps[id]);
	menu->wepTab.displayWeapon(menu->wepTab.curWep);

	return false;
}
bool ShipTab::onUpgrade(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		u32 uId = -5;
		if (currentShip->upgrades[id] > -1) uId = currentShip->upgrades[id];
		m_hoverShipUpgrade(uId);
		return false;
	}
	m_hideSelects();
	m_clearList();
	m_buildShipUpgradeList(std::bind(&ShipTab::onUpgradeSelect, this, std::placeholders::_1));
	currentSlot = id;
	return false;
}
bool ShipTab::onReload(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	WeaponInstance* inst = nullptr;
	if (id == PHYS_HARDPOINT) inst = campaign->getWeapon(currentShip->physWep);
	else if (id == HEAVY_HARDPOINT) inst = campaign->getWeapon(currentShip->heavyWep);
	else if (id >= 0 && id < MAX_HARDPOINTS) inst = campaign->getWeapon(currentShip->weps[id]);
	else return true; //this should literally never happen
	if (!inst->wep.usesAmmunition) return true; //why was this visible??

	s32 amt = (inst->wep.maxAmmunition - inst->fire.ammunition) / inst->wep.maxClip;
	s32 usedAmt = campaign->removeAmmo(amt);
	inst->fire.ammunition += (usedAmt * inst->wep.maxClip);
	if (usedAmt != amt) {
		guiController->setOkPopup("Out of Ammo", 
			"You don't have enough ammunition to fully reload this weapon.\n\n Weapon was reloaded with " + std::to_string(usedAmt) + " clips.",
			"Damn it");
		guiController->showOkPopup();
	}
	displayShip(currentShip);
	m_showSupplies();
	return false;
}

bool ShipTab::onRepair(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (!currentShip) return true;
	if (campaign->getFlag(L"MARTIN_BUILDING_CLOAK")) {
		guiController->setOkPopup("Locked", "Repairs aren't possible right now. Martin and his team are too busy with the cloak.");
		guiController->showOkPopup();
		displayShip(currentShip);
		m_showSupplies();
		return false;
	}
	f32 amt = currentShip->hp.maxHealth - currentShip->hp.health;
	f32 supplyCost = amt / 2.f;
	f32 usedSupply = campaign->removeSupplies(supplyCost);
	currentShip->hp.health += supplyCost * 2.f;
	if (usedSupply != supplyCost / 2.f) {
		guiController->setOkPopup("Out of Supplies",
			"You don't have enough supplies to repair this ship completely.\n\n Ship was repaired for " + fprecis(supplyCost*2.f, 5) + " HP.");
		guiController->showOkPopup();
	}
	displayShip(currentShip);
	m_showSupplies();
	return false;
}

bool ShipTab::onStrip(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	campaign->stripShip(currentShip);
	displayShip(currentShip);
	return false;
}