#include "WeaponTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include "GameController.h"
#include "LoadoutData.h"
#include "ShipInstance.h"
#include "WeaponUpgrades.h"
#include <iostream>
#include <sstream>
#include <string>
#include "AudioDriver.h"

void WeaponTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which)
{
	dialog = dial;
	this->which = which;
	descPos = position2di(150, 126);
	descSize = dimension2du(150, 120);
	m_basebg(root);

	dimension2du iconSize(25, 25);
	dimension2du buttonSize(50, 20);
	dimension2du upgradeButtonSize(142, 17);
	dimension2du bgSize(150, 220);

	wepBg = guienv->addImage(recti(vector2di(150, 20), bgSize), base);
	wepBg->setImage(driver->getTexture("assets/ui/sidelistbg.png"));
	scaleAlign(wepBg);

	curWepTitle = guienv->addStaticText(L"None", rect<s32>(position2di(154, 20), dimension2du(142, 20)), false, true, base);
	setAltUIText(curWepTitle);
	ammo = guienv->addStaticText(L"Ammo 0/0", rect<s32>(position2di(154, 40), dimension2du(142, 20)), false, true, base);
	setUITextSmall(ammo);

	position2di start(154, 55);
	for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
		start.Y += (i * upgradeButtonSize.Height);
		auto button = guienv->addButton(rect<s32>(start, upgradeButtonSize), base, i, L"No Upgrade", L"Select upgrades for this weapon.");
		setThinHoloButton(button, BCOL_LIGHTBLUE);
		upgradeSlots[i] = button;
		guiController->setCallback(button, std::bind(&WeaponTab::onChangeUpgrade, this, std::placeholders::_1), which);
	}
	start.Y += upgradeButtonSize.Height + 4;
	strip = guienv->addButton(recti(start, upgradeButtonSize), base, -1, L"Strip Upgrades", L"Strip upgrades from this weapon.");
	setThinHoloButton(strip, BCOL_BLUE);
	guiController->setCallback(strip, std::bind(&WeaponTab::onStrip, this, std::placeholders::_1), which);

	start.Y += upgradeButtonSize.Height;
	reload = guienv->addButton(rect<s32>(position2di(150,start.Y), dimension2du(150,18)), base, -1, L"Reload", L"Reload this weapon.");
	setThinHoloButton(reload, BCOL_ORANGE);
	guiController->setCallback(reload, std::bind(&WeaponTab::onReload, this, std::placeholders::_1), which);
	start.Y += upgradeButtonSize.Height;
	start.X += 4;

	//use this for the description size for weps at the end after setup
	dimension2du descSize(138, (bgSize.Height - start.Y) + 20);
	wepDesc = guienv->addStaticText(L"", recti(start, descSize), false, true, base);
	setAltUITextSmall(wepDesc);
	wepDesc->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	start = position2di(150, 0);
	reg = guienv->addButton(rect<s32>(start, buttonSize), base, HRDP_REGULAR, L"Regular", L"View regular weapon list.");
	setThinHoloButton(reg, BCOL_BLUE);
	start.X += buttonSize.Width;
	phys = guienv->addButton(rect<s32>(start, buttonSize), base, HRDP_PHYSICS, L"Physical", L"View physical weapon list.");
	setThinHoloButton(phys, BCOL_BLUE);
	start.X += buttonSize.Width;
	heavy = guienv->addButton(rect<s32>(start, buttonSize), base, HRDP_HEAVY, L"Heavy", L"View heavy weapon list.");
	setThinHoloButton(heavy, BCOL_BLUE);

	guiController->setCallback(reg, std::bind(&WeaponTab::onViewWepList, this, std::placeholders::_1), which);
	guiController->setCallback(phys, std::bind(&WeaponTab::onViewWepList, this, std::placeholders::_1), which);
	guiController->setCallback(heavy, std::bind(&WeaponTab::onViewWepList, this, std::placeholders::_1), which);
	m_toggleSelects(false);
	desc->setVisible(false);
	longdesc->setVisible(false);
	curType = (HARDPOINT_TYPE)10; //lol
}

bool WeaponTab::onReload(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (!curWep) return true; //wtf
	if (!curWep->wep.usesAmmunition) return true; //why was this visible??
	if (reloadGreyed) return true;

	s32 amt = (curWep->wep.maxAmmunition - curWep->fire.ammunition) / curWep->wep.maxClip;
	s32 usedAmt = campaign->removeAmmo(amt);
	curWep->fire.ammunition += (usedAmt * curWep->wep.maxClip);
	if (usedAmt != amt) {
		guiController->setOkPopup("Out of Ammo", "You don't have enough ammunition to fully reload this weapon.", "Damn it");
		guiController->showOkPopup();
	}
	reloadGreyed = true;
	setThinHoloButton(reload, BCOL_GREYED);

	displayWeapon(curWep);
	m_showSupplies();
	return false;
}

bool WeaponTab::onStrip(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (!curWep) return true;
	campaign->stripWeapon(curWep);
	displayWeapon(curWep);
	return false;
}

bool WeaponTab::onChangeUpgrade(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	if (curWep == nullptr) return true;
	u32 slot = event.GUIEvent.Caller->getID();
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		u32 id = -5;
		if (curWep) {
			if (curWep->upgrades[slot] > -1) id = curWep->upgrades[slot];
		}
		m_hoverWepUpgrade(id);
		return false;
	}
	currentSlot = slot;
	m_clearList();
	m_buildWepUpgradeList(std::bind(&WeaponTab::onUpgradeSelect, this, std::placeholders::_1));
	m_toggleSelects(false);
	return false;
}
bool WeaponTab::onViewWepList(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	HARDPOINT_TYPE type = (HARDPOINT_TYPE)event.GUIEvent.Caller->getID();
	if (type == curType) return true;
	curType = type;
	m_buildCurWepList();
	return false;
}
void WeaponTab::m_buildCurWepList()
{
	m_clearList();
	m_buildWepList(std::bind(&WeaponTab::onWepSelect, this, std::placeholders::_1), false, curType, false);

	setThinHoloButton(reg, BCOL_BLUE);
	setThinHoloButton(heavy, BCOL_BLUE);
	setThinHoloButton(phys, BCOL_BLUE);

	if (curType == HRDP_PHYSICS) setThinHoloButton(phys, BCOL_ORANGE);
	else if (curType == HRDP_HEAVY) setThinHoloButton(heavy, BCOL_ORANGE);
	else setThinHoloButton(reg, BCOL_ORANGE);
}

bool WeaponTab::onWepSelect(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		m_hoverWeapon(event.GUIEvent.Caller->getID());
		return false;
	}
	WeaponInstance* wep = campaign->getWeapon(event.GUIEvent.Caller->getID());
	curWep = wep;
	displayWeapon(curWep);
	return false;
}
bool WeaponTab::onUpgradeSelect(const SEvent& event)
{
	if (m_hoverClickCheck(event)) return true;
	if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
		m_hoverWepUpgrade(event.GUIEvent.Caller->getID(), true);
		return false;
	}
	WeaponUpgradeInstance* inst = campaign->getWeaponUpgrade(event.GUIEvent.Caller->getID());

	if (inst)
		audioDriver->playMenuSound("fighter_equip.ogg");
	else
		audioDriver->playMenuSound("fighter_unequip.ogg");

	campaign->assignWepUpgrade(curWep, currentSlot, inst);
	displayWeapon(curWep);
	m_clearList();
	longdesc->setText(L"");
	desc->setText(L"");
	return false;
}

void WeaponTab::m_toggleSelects(bool vis)
{
	reload->setVisible(vis);
	curWepTitle->setVisible(vis);
	ammo->setVisible(vis);
	strip->setVisible(vis);
	if (!vis) wepDesc->setText(L"");
	for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
		upgradeSlots[i]->setVisible(vis);
	}
}

void WeaponTab::displayWeapon(WeaponInstance* inst)
{
	if (!inst) {
		m_toggleSelects(false);
		return;
	}
	m_toggleSelects(true);
	curWepTitle->setVisible(true);
	HARDPOINT_TYPE type = inst->wep.hrdtype;
	WeaponData* dat = nullptr;
	s32 datId = inst->wep.wepDataId;
	if (type == HRDP_PHYSICS) dat = physWeaponData[datId];
	else if (type == HRDP_HEAVY) dat = heavyWeaponData[datId];
	else dat = weaponData[datId];
	auto str = m_weaponStatStr(inst, false);
	str += "\n" + dat->description;

	wepDesc->setText(wstr(str).c_str());
	curWepTitle->setText(wstr(dat->name).c_str());
	if (inst->wep.usesAmmunition) {
		ammo->setVisible(true);
		std::wstring ammostr = L"Clips: " + wstr(std::to_string(inst->fire.ammunition / inst->wep.maxClip)) + L"/" + wstr(std::to_string(inst->wep.maxAmmunition / inst->wep.maxClip) 
			+ " (" + std::to_string(inst->wep.maxClip) + ")");
		ammo->setText(ammostr.c_str());
		if (inst->fire.ammunition < inst->wep.maxAmmunition) {
			reloadGreyed = false;
			setThinHoloButton(reload, BCOL_ORANGE);
		}
		else {
			reloadGreyed = true;
			setThinHoloButton(reload, BCOL_GREYED);
		}
	}
	else {
		reloadGreyed = true;
		ammo->setVisible(false);
		setThinHoloButton(reload, BCOL_GREYED);
	}
	for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
		if (inst->upgrades[i] > -1) {
			auto up = campaign->getWeaponUpgrade(inst->upgrades[i]);
			s32 upId = up->dataId;
			std::string name = weaponUpgradeData.at(upId)->name;
			upgradeSlots[i]->setText(wstr(name).c_str());
		}
		else {
			upgradeSlots[i]->setText(L"No Upgrade");
		}
		upgradeSlots[i]->setVisible(true);
	}
	m_buildCurWepList();

	desc->setText(wstr(m_weaponStatStr(inst, false)).c_str());
}