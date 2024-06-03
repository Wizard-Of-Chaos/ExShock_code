#include "LoadoutTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include "GameController.h"
#include "LoadoutData.h"
#include "ShipInstance.h"
#include "ShipUpgrades.h"
#include "WeaponUpgrades.h"
#include "ProjectileUtils.h"
#include "GameFunctions.h"
#include <sstream>
#include <string>

void LoadoutTab::m_basebg(IGUIElement* root)
{
	IGUIStaticText* img = guienv->addStaticText(L"", rect<s32>(position2di(481, 132), dimension2du(300, 246)), false, true, root);
	scaleAlign(img);
	base = img;
	descPos = position2di(0, 136);
	descSize = dimension2du(300, 90);

	desc = guienv->addStaticText(L"", rect<s32>(descPos, descSize), false, true, base);
	longdesc = guienv->addStaticText(L"", rect<s32>(longDescPos, longDescSize), false, true, base);
	setUIText(desc);
	setUITextSmall(longdesc);
	listBg = guienv->addStaticText(L"", recti(vector2di(0, 0), dimension2du(150, 246)), false, true, base);
	setUIText(listBg);
	listItemVertSizeRatio = (20.f/540.f);
	listItemHorizSizeRatio = (150.f/960.f);
	listStartXRatio = (0.f / 960.f);
	listStartYRatio = (0.f / 540.f);

	guiController->setCallback(listBg, std::bind(&LoadoutTab::onListScroll, this, std::placeholders::_1), which, GUICONTROL_KEY | GUICONTROL_MOUSE);
	m_buildLoadoutBottomShipView();
	m_buildLoadoutSideShipView();
	loadoutBottomShipView.hide();
	loadoutSideShipView.hide();
}
void LoadoutTab::m_buildLoadoutBottomShipView()
{
	//setup on the ship view tab
	loadoutBottomShipView.shipDescBg = guienv->addImage(rect<s32>(descPos, descSize), base);
	scaleAlign(loadoutBottomShipView.shipDescBg);
	loadoutBottomShipView.shipDescBg->setImage(driver->getTexture("assets/ui/wingdesc_bg.png"));
	loadoutBottomShipView.shipDescName = guienv->addStaticText(L"", recti(vector2di(0, 2), dimension2du(110, 20)), false, true, loadoutBottomShipView.shipDescBg);
	setAltUIText(loadoutBottomShipView.shipDescName);
	dimension2du wepIconSize(15, 15);
	dimension2du wepTxtSize(80, 15);
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		vector2di spos(115, 5);
		if (i % 2 != 0) spos = vector2di(210, 5);
		spos.Y += (18 * (i / 2));
		loadoutBottomShipView.shipWepNames[i] = guienv->addStaticText(L"", recti(spos, wepTxtSize), false, true, loadoutBottomShipView.shipDescBg);
		setAltUITextSmall(loadoutBottomShipView.shipWepNames[i]);
		loadoutBottomShipView.shipWepNames[i]->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

		loadoutBottomShipView.shipWepIcons[i] = guienv->addImage(recti(vector2di(-15, 0), wepIconSize), loadoutBottomShipView.shipWepNames[i]);
		loadoutBottomShipView.shipWepIcons[i]->setImage(driver->getTexture("assets/ui/ammo_greyed.png"));
		scaleAlign(loadoutBottomShipView.shipWepIcons[i]);
		loadoutBottomShipView.shipWepIcons[i]->setNotClipped(true);
	}
	loadoutBottomShipView.shipHvyWepName = guienv->addStaticText(L"", recti(vector2di(115, 70), wepTxtSize), false, true, loadoutBottomShipView.shipDescBg);
	setAltUITextSmall(loadoutBottomShipView.shipHvyWepName);
	loadoutBottomShipView.shipHvyWepName->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	loadoutBottomShipView.hvyIcon = guienv->addImage(recti(vector2di(-15, 0), wepIconSize), loadoutBottomShipView.shipHvyWepName);
	loadoutBottomShipView.hvyIcon->setImage(driver->getTexture("assets/ui/ammo_greyed.png"));
	scaleAlign(loadoutBottomShipView.hvyIcon);
	loadoutBottomShipView.hvyIcon->setNotClipped(true);

	loadoutBottomShipView.shipPhysWepName = guienv->addStaticText(L"", recti(vector2di(210, 70), wepTxtSize), false, true, loadoutBottomShipView.shipDescBg);
	setAltUITextSmall(loadoutBottomShipView.shipPhysWepName);
	loadoutBottomShipView.shipPhysWepName->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	loadoutBottomShipView.physIcon = guienv->addImage(recti(vector2di(-15, 0), wepIconSize), loadoutBottomShipView.shipPhysWepName);
	loadoutBottomShipView.physIcon->setImage(driver->getTexture("assets/ui/ammo_greyed.png"));
	scaleAlign(loadoutBottomShipView.physIcon);
	loadoutBottomShipView.physIcon->setNotClipped(true);

	loadoutBottomShipView.hpBar = guienv->addImage(recti(vector2di(10, 20), dimension2du(90, 7)), loadoutBottomShipView.shipDescBg);
	loadoutBottomShipView.hpBar->setImage(driver->getTexture("assets/ui/bars/yellowgreen.png"));
	scaleAlign(loadoutBottomShipView.hpBar);

	auto capPosL = recti(position2di(-2, 0), dimension2du(3, 7));
	auto capPosR = recti(position2di(88, 0), dimension2du(3, 7));

	auto cap = guienv->addImage(capPosL, loadoutBottomShipView.hpBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_l.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);
	cap = guienv->addImage(capPosR, loadoutBottomShipView.hpBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_r.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);

	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		loadoutBottomShipView.shipUpNames[i] = guienv->addStaticText(L"", recti(position2di(10, 22 + (18 * i)), dimension2du(90, 18)), false, true, loadoutBottomShipView.shipDescBg);
		setAltUITextSmall(loadoutBottomShipView.shipUpNames[i]);
	}
}
void LoadoutTab::m_buildLoadoutSideShipView()
{
	loadoutSideShipView.shipDescBg = guienv->addImage(recti(longDescPos, longDescSize), base);
	scaleAlign(loadoutSideShipView.shipDescBg);
	loadoutSideShipView.shipDescBg->setImage(driver->getTexture("assets/ui/sidelistbg.png"));
	loadoutSideShipView.shipDescName = guienv->addStaticText(L"", recti(vector2di(0, 0), dimension2du(150, 20)), false, true, loadoutSideShipView.shipDescBg);
	setAltUIText(loadoutSideShipView.shipDescName);

	loadoutSideShipView.hpBar = guienv->addImage(recti(vector2di(8, 20), dimension2du(134, 7)), loadoutSideShipView.shipDescBg);
	loadoutSideShipView.hpBar->setImage(driver->getTexture("assets/ui/bars/yellowgreen.png"));
	scaleAlign(loadoutSideShipView.hpBar);

	auto capPosL = recti(position2di(-2, 0), dimension2du(3, 7));
	auto capPosR = recti(position2di(132, 0), dimension2du(3, 7));

	auto cap = guienv->addImage(capPosL, loadoutSideShipView.hpBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_l.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);
	cap = guienv->addImage(capPosR, loadoutSideShipView.hpBar);
	cap->setImage(driver->getTexture("assets/ui/bars/end_r.png"));
	cap->setNotClipped(true);
	scaleAlign(cap);

	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		loadoutSideShipView.shipUpNames[i] = guienv->addStaticText(L"", recti(position2di(4, 27 + (18 * i)), dimension2du(142, 18)), false, true, loadoutSideShipView.shipDescBg);
		setAltUITextSmall(loadoutSideShipView.shipUpNames[i]);
	}

	dimension2du wepIconSize(15, 15);
	dimension2du wepTxtSize(127, 15);
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		vector2di spos(19, 72);
		spos.Y += (18 * i);
		loadoutSideShipView.shipWepNames[i] = guienv->addStaticText(L"", recti(spos, wepTxtSize), false, true, loadoutSideShipView.shipDescBg);
		setAltUITextSmall(loadoutSideShipView.shipWepNames[i]);
		loadoutSideShipView.shipWepNames[i]->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

		loadoutSideShipView.shipWepIcons[i] = guienv->addImage(recti(vector2di(-15, 0), wepIconSize), loadoutSideShipView.shipWepNames[i]);
		loadoutSideShipView.shipWepIcons[i]->setImage(driver->getTexture("assets/ui/ammo_greyed.png"));
		scaleAlign(loadoutSideShipView.shipWepIcons[i]);
		loadoutSideShipView.shipWepIcons[i]->setNotClipped(true);
	}

	loadoutSideShipView.shipHvyWepName = guienv->addStaticText(L"", recti(vector2di(19, 180), wepTxtSize), false, true, loadoutSideShipView.shipDescBg);
	setAltUITextSmall(loadoutSideShipView.shipHvyWepName);
	loadoutSideShipView.shipHvyWepName->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	loadoutSideShipView.hvyIcon = guienv->addImage(recti(vector2di(-15, 0), wepIconSize), loadoutSideShipView.shipHvyWepName);
	loadoutSideShipView.hvyIcon->setImage(driver->getTexture("assets/ui/ammo_greyed.png"));
	scaleAlign(loadoutSideShipView.hvyIcon);
	loadoutSideShipView.hvyIcon->setNotClipped(true);

	loadoutSideShipView.shipPhysWepName = guienv->addStaticText(L"", recti(vector2di(19, 195), wepTxtSize), false, true, loadoutSideShipView.shipDescBg);
	setAltUITextSmall(loadoutSideShipView.shipPhysWepName);
	loadoutSideShipView.shipPhysWepName->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	loadoutSideShipView.physIcon = guienv->addImage(recti(vector2di(-15, 0), wepIconSize), loadoutSideShipView.shipPhysWepName);
	loadoutSideShipView.physIcon->setImage(driver->getTexture("assets/ui/ammo_greyed.png"));
	scaleAlign(loadoutSideShipView.physIcon);
	loadoutSideShipView.physIcon->setNotClipped(true);
}

void LoadoutTab::_loadoutShipView::showShip(ShipInstance* inst)
{
	if (!inst) return;
	ShipData* dat = shipData.at(inst->ship.shipDataId);
	if (!dat) return;
	shipDescBg->setVisible(true);
	std::wstring name = wstr(dat->name);
	if (inst->overrideName != "") name = wstr(inst->overrideName);
	shipDescName->setText(name.c_str());
	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {

		shipWepNames[i]->setText(L"");
		if (i >= inst->hards.hardpointCount) shipWepIcons[i]->setImage(driver->getTexture("assets/ui/ammo_greyed.png"));
		else shipWepIcons[i]->setImage(driver->getTexture("assets/ui/ammo.png"));

		auto wep = campaign->getWeapon(inst->weps[i]);
		if (!wep) continue;

		shipWepNames[i]->setText(wstr(weaponData.at(wep->wep.wepDataId)->name).c_str());
	}
	shipHvyWepName->setText(L"");
	shipPhysWepName->setText(L"");
	hvyIcon->setImage((inst->inUseBy == 0) ? driver->getTexture("assets/ui/ammo.png") : driver->getTexture("assets/ui/ammo_greyed.png"));
	physIcon->setImage((inst->inUseBy == 0) ? driver->getTexture("assets/ui/ammo.png") : driver->getTexture("assets/ui/ammo_greyed.png"));
	auto hvy = campaign->getWeapon(inst->heavyWep);
	if (hvy) shipHvyWepName->setText(wstr(heavyWeaponData.at(hvy->wep.wepDataId)->name).c_str());
	auto phys = campaign->getWeapon(inst->physWep);
	if (phys) shipPhysWepName->setText(wstr(physWeaponData.at(phys->wep.wepDataId)->name).c_str());

	dimension2du scrn = driver->getScreenSize();
	dimension2du barBaseSize = dimension2du((90.f / 960.f) * scrn.Width, (7.f / 540.f) * scrn.Height);
	dimension2du hpSize = barBaseSize;
	hpSize.Width = (inst->hp.health / inst->hp.maxHealth) * barBaseSize.Width;
	hpBar->setRelativePosition(recti(hpBar->getRelativePosition().UpperLeftCorner, hpSize));

	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		shipUpNames[i]->setText(L"");
		ShipUpgradeInstance* up = campaign->getShipUpgrade(inst->upgrades[i]);
		if (!up) continue;
		shipUpNames[i]->setText(wstr(shipUpgradeData.at(up->dataId)->name).c_str());
	}
}

void LoadoutTab::_loadoutSideShipView::showShip(ShipInstance* inst)
{
	_loadoutShipView::showShip(inst);
	dimension2du scrn = driver->getScreenSize();
	dimension2du barBaseSize = dimension2du((134.f / 960.f) * scrn.Width, (7.f / 540.f) * scrn.Height);
	dimension2du hpSize = barBaseSize;
	hpSize.Width = (inst->hp.health / inst->hp.maxHealth) * barBaseSize.Width;
	hpBar->setRelativePosition(recti(hpBar->getRelativePosition().UpperLeftCorner, hpSize));

}

void LoadoutTab::show()
{
	base->setVisible(true);
	longdesc->setText(L"");
	m_showSupplies();
}

void LoadoutTab::m_showSupplies()
{
	if (dialog) {
		auto menu = (GuiLoadoutMenu*)dialog;
		menu->showSupplies();
	}
}

bool LoadoutTab::onListScroll(const SEvent& event)
{
	if (!base->isVisible() || 
		listItems.empty() || 
		listItems.size() * (listItemVertSizeRatio * driver->getScreenSize().Height) <= listBg->getRelativePosition().getHeight()) return true;

	if (event.EventType != EET_MOUSE_INPUT_EVENT && event.EventType != EET_KEY_INPUT_EVENT) return true; //this wasn't from a key or a mouse? who cares?
	if (event.EventType == EET_MOUSE_INPUT_EVENT && event.MouseInput.Event != EMIE_MOUSE_WHEEL) return true; //mouse input not from a scroll wheel? who cares?
	if (event.EventType == EET_KEY_INPUT_EVENT && (event.KeyInput.Key != KEY_UP && event.KeyInput.Key != KEY_DOWN)) return true;  // key input not from up or down? who cares?
	bool up = true;
	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		up = (event.MouseInput.Wheel > 0.f);
	}
	else if (event.EventType == EET_KEY_INPUT_EVENT) {
		up = event.KeyInput.Key == KEY_UP;
	}
	s32 dist = listItemVertSizeRatio * driver->getScreenSize().Height;
	if (up) {
		--listScrollPos;
		if (listScrollPos < 0) {
			listScrollPos = 0;
			return false;
		}
	}
	else {
		dist = -dist;
		++listScrollPos;
		if (listScrollPos == listItems.size()) {
			listScrollPos = (listItems.size() - 1);
			return false;
		}
	}
	for (auto item : listItems) {
		recti pos = item->getRelativePosition();
		item->setRelativePosition(vector2di(pos.UpperLeftCorner.X, pos.UpperLeftCorner.Y + dist));
	}
	return false;
}

void LoadoutTab::m_clearList()
{
	for (auto item : listItems) {
		if (item) {
			guiController->removeCallback(item);
			item->remove();
		}
	}
	listScrollPos = 0;
	listItems.clear();
}
dimension2du LoadoutTab::m_listItemSize()
{
	dimension2du scrn = driver->getScreenSize();
	return dimension2du(listItemHorizSizeRatio * scrn.Width, listItemVertSizeRatio * scrn.Height);
}
position2di LoadoutTab::m_listItemStart()
{
	return position2di(listStartXRatio * driver->getScreenSize().Width, listStartYRatio * driver->getScreenSize().Height);
}
u32 LoadoutTab::m_listItemHeightDiff()
{
	dimension2du scrn = driver->getScreenSize();
	return (u32)((listItemVertSizeRatio * scrn.Height));
}
bool LoadoutTab::m_hoverClickCheck(const SEvent& event)
{
	auto e = event.GUIEvent.EventType;
	if (e != EGET_BUTTON_CLICKED && e != EGET_ELEMENT_HOVERED) return true;
	return false;
}

std::string LoadoutTab::m_wingDescStr(WingmanInstance* mans)
{
	if (!mans) return "";
	std::string ret = "";
	ret += mans->name + "\n\n";
	ret += mans->description + "\n";
	return ret;
}

std::string LoadoutTab::m_wingStatStr(WingmanInstance* mans, bool name)
{
	if (!mans) return "";
	std::string ret = "";
	if(name) ret += mans->name + "\n\n";
	ret += "Kills: " + std::to_string(mans->totalKills) + " Injuries: " + std::to_string(mans->totalInjuries) + "\n";
	if (mans->id != 0) ret += "Aggro: " + fprecis(mans->ai.aggressiveness, 4) + " Resolve: " + fprecis(mans->ai.resolve, 4) + "\n"
		+ "Aim: " + fprecis(mans->ai.aim, 4) + " Reflex: " + fprecis(mans->ai.reactionSpeed, 4) + "\n";
	else ret += "Aggro/Resolve/Aim/Reflex: User Dependent";
	if (mans->injured) ret += "Currently Injured\n";
	return ret;
}

std::string LoadoutTab::m_shipDescStr(ShipInstance* inst, bool includeCostVals)
{
	if (!inst) return "";
	std::string ret = "";
	ShipData* dat = shipData.at(inst->ship.shipDataId);
	if (!dat) return "";
	ret += dat->name + " | HP: " + fprecis(inst->hp.health, 5) + "/" + fprecis(inst->hp.maxHealth, 5) + "\n\n";
	const ThrustComponent& thr = dat->thrust;
	ret += "Hardpoints: " + std::to_string(inst->hards.hardpointCount) + "\n";
	ret += "Speed | Forward: " + fprecis(thr.forward, 5) + " Brake: " + fprecis(thr.brake, 5);
	ret += " Boost: " + fprecis(thr.boost, 5) + "\n";
	ret	+= "Pitch/Yaw/Roll: " + fprecis(thr.pitch, 5) + "/" + fprecis(thr.yaw, 5) + "/" + fprecis(thr.roll, 5) + "\n";
	ret += "Max Velocity: " + fprecis(thr.linearMaxVelocity, 5) + "\n Velocity Tolerance : " + fprecis(thr.velocityTolerance, 4) + "\n\n";

	if (includeCostVals) {
		ret += "Cost: " + fprecis(dat->buildCost, 5) + "\n";
		ret += "Scrap value: " + fprecis(dat->buildCost / 2.f, 5) + "\n\n";
	}

	ret += dat->description;
	return ret;
}
std::string LoadoutTab::m_shipStatStr(ShipInstance* inst, bool includeName)
{
	if (!inst) return "";
	std::string ret = "";
	ShipData* dat = shipData.at(inst->ship.shipDataId);
	if (!dat) return "";
	if (includeName) {
		ret += dat->name + " | ";
		ret += "HP: " + fprecis(inst->hp.health, 5) + " / " + fprecis(inst->hp.maxHealth, 5) + "\n";
	}
	ret += "Hardpoints: " + std::to_string(inst->hards.hardpointCount);
	u32 count = 0;
	for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
		if (inst->weps[i] > -1) ++count;
	}
	ret += " Used: " + std::to_string(count) + "\n";
	ret += " Phys: ";
	if (inst->physWep > -1) ret += "Yes";
	else ret += "No";
	ret += " Heavy: ";
	if (inst->heavyWep > -1) ret += "Yes";
	else ret += "No";
	ret += "\n";
	count = 0;
	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		if (inst->upgrades[i] > -1) ++count;
	}
	ret += "Enhancements: " + std::to_string(count);
	return ret; 
}

std::string LoadoutTab::m_weaponDescStr(WeaponInstance* inst, bool includeCostVals)
{
	if (!inst) return "";
	std::string ret = "";
	auto type = inst->wep.hrdtype;

	WeaponData* data = nullptr;
	if (type == HRDP_HEAVY) data = heavyWeaponData[inst->wep.wepDataId];
	else if (type == HRDP_PHYSICS) data = physWeaponData[inst->wep.wepDataId];
	else data = weaponData[inst->wep.wepDataId];
	if (!data) return ""; //what the hell?

	ret += data->name;
	if (type != HRDP_PHYSICS) {
		ret += " | ";
		for (auto [key, val] : weaponStrings) {
			if (val == inst->wep.type) {
				ret += key;
				break;
			}
		}
	}
	ret += "\n\n";
	if (includeCostVals) ret += "Build cost: " + fprecis(data->buildCost,5) + "\nScrap value: " + fprecis(data->buildCost / 2.f, 5) + "\n\n";
	ret += data->description;
	return ret;
}
std::string LoadoutTab::m_weaponStatStr(WeaponInstance* inst, bool includeName)
{
	if (!inst) return "";

	std::string ret = "";
	auto type = inst->wep.hrdtype;

	WeaponData* data = nullptr;
	if (type == HRDP_HEAVY) {
		data = heavyWeaponData[inst->wep.wepDataId];
	}
	else if (type == HRDP_PHYSICS) data = physWeaponData[inst->wep.wepDataId];
	else data = weaponData[inst->wep.wepDataId];

	if (!data) return ""; //what the hell?

	if (includeName) ret += data->name + "\n\n";
	if (type != HRDP_PHYSICS) {
		ret += "Damage Type: ";
		for (auto [key, val] : damageStrings) {
			if (val == data->wepComp.dmgtype) ret += key;
		}
		ret += "\n";
	}
	ret += "Damage: " + fprecis(getDamageDifficulty(inst->wep.damage, true), 5) + "\n";
	ret += "Firerate: " + fprecis(inst->wep.firingSpeed, 4) + "\n";
	ret += "Range: " + fprecis(((inst->wep.projectileSpeed / .1f) * inst->wep.lifetime), 5);
	ret += " (Velocity: " + fprecis(inst->wep.projectileSpeed, 5) + ")\n";
	if (data->wepComp.projectilesPerShot > 1) ret += "Projectiles Per Shot: " + std::to_string(data->wepComp.projectilesPerShot) + "\n";
	if (data->wepComp.usesPower) ret += "Power Cost: " + fprecis(data->wepComp.powerCost, 5) + "\n";
	if (data->wepComp.usesAmmunition) ret += "Reload Time: " + fprecis(inst->wep.reloadTime, 5) + "\n";
	return ret;
}

std::string LoadoutTab::m_shipUpgradeDescStr(ShipUpgradeInstance* inst, bool includeCostVals)
{
	if (!inst) return "";
	ShipUpgradeData* dat = shipUpgradeData[inst->dataId];
	if (!dat) return "";
	std::string ret = "";
	ret += dat->name + "\n\n";
	ret += dat->description + "\n\n";
	if (includeCostVals) {
		f32 scrapVal = dat->baseCost + (((inst->value - dat->baseValue) / dat->scaleValue) * dat->scaleCost);
		ret += "Scrap value: " + fprecis(scrapVal, 4);
	}
	return ret;
}
std::string LoadoutTab::m_shipUpgradeStatStr(ShipUpgradeInstance* inst, bool includeName)
{
	if (!inst) return "";
	ShipUpgradeData* dat = shipUpgradeData[inst->dataId];
	if (!dat) return "";
	std::string ret = "";
	if (includeName) ret += dat->name + "\n\n";
	ret += "Mod value: " + fprecis(inst->value, 4) + "\n";;
	ret += "In Use: ";
	ret += (inst->usedBy > -1) ? "Yes" : "No";
	return ret;
}
std::string LoadoutTab::m_wepUpgradeDescStr(WeaponUpgradeInstance* inst, bool includeCostVals)
{
	if (!inst) return "";
	WeaponUpgradeData* dat = weaponUpgradeData[inst->dataId];
	if (!dat) return "";
	std::string ret = "";
	ret += dat->name + "\n\n";
	ret += dat->description + "\n\n";
	if (includeCostVals) {
		f32 scrapVal = dat->baseCost + (((inst->value - dat->baseValue) / dat->scaleValue) * dat->scaleCost);
		ret += "Scrap value: " + fprecis(scrapVal, 4);
	}
	return ret;
}
std::string LoadoutTab::m_wepUpgradeStatStr(WeaponUpgradeInstance* inst, bool includeName)
{
	if (!inst) return "";
	WeaponUpgradeData* dat = weaponUpgradeData[inst->dataId];
	if (!dat) return "";
	std::string ret = "";
	if (includeName) ret += dat->name + "\n\n";
	ret += "Mod value: " + fprecis(inst->value, 4) + "\n";;
	ret += "In Use: ";
	ret += (inst->usedBy > -1) ? "Yes" : "No";
	return ret;
}

void LoadoutTab::m_buildShipList(std::function<bool(const SEvent&)> cb, bool hideUsed, bool includesNone, BUTTON_COLOR color)
{
	m_clearList();
	u32 count = 0;
	if (includesNone) {
		auto none = guienv->addButton(rect<s32>(m_listItemStart(), m_listItemSize()), listBg, -5, L"None", L"Deselect ship.");
		setThinHoloButton(none, BCOL_RED);
		guiController->setCallback(none, cb, which);
		listItems.push_back(none);
		++count;
	}
	for (auto ship : campaign->ships()) {
		if (hideUsed && ship->inUseBy > -1) continue;
		std::string name = shipData[ship->ship.shipDataId]->name;
		if (ship->inUseBy > -1) {
			std::stringstream s(campaign->getWingman(ship->inUseBy)->name);
			std::string firstname;
			std::getline(s, firstname, ' ');
			name += " (" + firstname + ")";
		}
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, ship->id, wstr(name).c_str(), L"Select this ship.");
		setThinHoloButton(button, color);
		guiController->setCallback(button, cb, which);
		listItems.push_back(button);
	}
}

void LoadoutTab::m_buildWepList(std::function<bool(const SEvent&)> cb, bool hideUsed, HARDPOINT_TYPE type, bool includesNone, BUTTON_COLOR color)
{
	m_clearList();
	s32 nullId = -5;
	const std::list<WeaponInstance*>* list = &campaign->weapons();
	if (type == HRDP_PHYSICS) {
		list = &campaign->physWeapons();
		nullId = -6;
	}
	if (type == HRDP_HEAVY) {
		list = &campaign->heavyWeapons();
		nullId = -7;
	}
	u32 count = 0;
	if (includesNone) {
		auto none = guienv->addButton(rect<s32>(m_listItemStart(), m_listItemSize()), listBg, nullId, L"None", L"Deselect weapon.");
		setThinHoloButton(none, BCOL_RED);
		guiController->setCallback(none, cb, which);
		listItems.push_back(none);
		++count;
	}

	for (auto wep : *list) {
		if (hideUsed && wep->usedBy > -1) continue;
		std::string name;
		WeaponData* dat = nullptr;
		if (type == HRDP_HEAVY) dat = heavyWeaponData[wep->wep.wepDataId];
		else if (type == HRDP_PHYSICS) dat = physWeaponData[wep->wep.wepDataId];
		else dat = weaponData[wep->wep.wepDataId];

		name = dat->name;

		if (wep->usedBy >-1) {
			std::stringstream s(shipData[campaign->getShip(wep->usedBy)->ship.shipDataId]->name);
			std::string firstname;
			std::getline(s, firstname, ' ');
			name += " (" + firstname + ")";
		}
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, wep->id, wstr(name).c_str(), L"Select this weapon.");
		setThinHoloButton(button, color);
		guiController->setCallback(button, cb, which);
		listItems.push_back(button);
	}
}

void LoadoutTab::m_buildShipUpgradeList(std::function<bool(const SEvent&)> cb, bool hideUsed, bool includesNone, BUTTON_COLOR color)
{
	m_clearList();
	s32 nullId = -5;
	u32 count = 0;
	if (includesNone) {
		auto none = guienv->addButton(rect<s32>(m_listItemStart(), m_listItemSize()), listBg, nullId, L"None", L"Deselect upgrade.");
		setThinHoloButton(none, BCOL_RED);
		guiController->setCallback(none, cb, which);
		listItems.push_back(none);
		++count;
	}
	for (auto up : campaign->shipUpgrades()) {
		if (hideUsed && up->usedBy > -1) continue;
		ShipUpgradeData* dat = shipUpgradeData[up->dataId];
		if (!dat) continue;
		std::string name = dat->name;
		if (up->usedBy > -1) name += "(" + shipData[campaign->getShip(up->usedBy)->ship.shipDataId]->name + ")";
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, up->id, wstr(name).c_str(), L"Select this upgrade.");
		setThinHoloButton(button, color);
		guiController->setCallback(button, cb, which);
		listItems.push_back(button);

	}
}

void LoadoutTab::m_buildWepUpgradeList(std::function<bool(const SEvent&)> cb, bool hideUsed, bool includesNone, BUTTON_COLOR color)
{
	m_clearList();
	s32 nullId = -5;
	u32 count = 0;
	if (includesNone) {
		auto none = guienv->addButton(rect<s32>(m_listItemStart(), m_listItemSize()), listBg, nullId, L"None", L"Deselect upgrade.");
		setThinHoloButton(none, BCOL_RED);
		guiController->setCallback(none, cb, which);
		listItems.push_back(none);
		++count;
	}
	for (auto up : campaign->weaponUpgrades()) {
		if (hideUsed && up->usedBy > -1) continue;
		WeaponUpgradeData* dat = weaponUpgradeData[up->dataId];
		if (!dat) continue;

		std::string name = dat->name;
		if (up->usedBy > -1) name += "(" + weaponData[campaign->getWeapon(up->usedBy)->wep.wepDataId]->name + ")";
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, up->id, wstr(name).c_str(), L"Select this upgrade.");
		setThinHoloButton(button, color);
		guiController->setCallback(button, cb, which);
		listItems.push_back(button);

	}
}

bool LoadoutTab::m_hoverShip(s32 id, bool full)
{
	ShipInstance* inst = nullptr;
	if (id == 0) inst = campaign->getPlayerShip();
	else inst = campaign->getShip(id);
	if (!inst) return true;
	if (full) {
		loadoutBottomShipView.hide();
		loadoutSideShipView.showShip(inst);
		//longdesc->setText(wstr(m_shipDescStr(inst)).c_str());
	}
	else {
		loadoutSideShipView.hide();
		loadoutBottomShipView.showShip(inst);
	}
	return false;
}

bool LoadoutTab::m_hoverWeapon(s32 id, bool full)
{
	WeaponInstance* inst = nullptr;
	inst = campaign->getWeapon(id);
	if (!inst) return true;
	desc->setText(wstr(m_weaponStatStr(inst)).c_str());
	if (full) longdesc->setText(wstr(m_weaponStatStr(inst)).c_str());
	return false;
}

bool LoadoutTab::m_hoverShipUpgrade(s32 id, bool full)
{
	ShipUpgradeInstance* inst = nullptr;
	inst = campaign->getShipUpgrade(id);
	if (!inst) return true;
	desc->setText(wstr(m_shipUpgradeStatStr(inst)).c_str());
	if (full) longdesc->setText(wstr(m_shipUpgradeDescStr(inst) + "\n" + m_shipUpgradeStatStr(inst, false)).c_str());
	return false;
}
bool LoadoutTab::m_hoverWepUpgrade(s32 id, bool full)
{
	WeaponUpgradeInstance* inst = nullptr;
	inst = campaign->getWeaponUpgrade(id);
	if (!inst) return true;
	desc->setText(wstr(m_wepUpgradeStatStr(inst)).c_str());
	if (full) longdesc->setText(wstr(m_wepUpgradeDescStr(inst)).c_str());
	return false;
}