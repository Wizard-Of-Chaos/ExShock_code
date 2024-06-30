#include "WeaponBuildTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include <iostream>
#include "AudioDriver.h"

void WeaponBuildTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which)
{
	dialog = dial;
	fab = (GuiFabMenu*)dial;
	this->which = which;
	m_basebg(root);

	listItemVertSizeRatio = (20.f / 540.f);
	listItemHorizSizeRatio = (150.f / 960.f);
	if (desc) desc->remove();
	if (longdesc) longdesc->remove();

	if (listBg) {
		guiController->removeCallback(listBg);
		listBg->remove();
	}
	wepPos = vector3df(0, 0, 18);

	listBg = guienv->addStaticText(L"", recti(vector2di(10, 30), dimension2du(150, 320)), false, true, base);
	setUIText(listBg);
	guiController->setCallback(listBg, std::bind(&LoadoutTab::onListScroll, this, std::placeholders::_1), which, GUICONTROL_KEY | GUICONTROL_MOUSE);

	regList = guienv->addButton(recti(vector2di(10, 5), dimension2du(50, 25)), base, -1, L"Regular", L"View regular weapon list.");
	setThinHoloButton(regList);
	hvyList = guienv->addButton(recti(vector2di(60, 5), dimension2du(50, 25)), base, -1, L"Heavy", L"View heavy weapon list.");
	setThinHoloButton(hvyList, BCOL_BLUE);
	physList = guienv->addButton(recti(vector2di(110, 5), dimension2du(50, 25)), base, -1, L"Physics", L"View physics weapon list.");
	setThinHoloButton(physList, BCOL_BLUE);
	regList->setEnabled(false);

	lineBase = guienv->addStaticText(L"", recti(vector2di(160, 30), dimension2du(250, 300)), false, false, base);
	scaleAlign(lineBase);

	name = guienv->addStaticText(L"", recti(position2di(160, 30), dimension2du(300, 20)), false, true, base);
	setAltUIText(name);
	statsNames = guienv->addStaticText(L"", recti(position2di(160, 50), dimension2du(80, 130)), false, true, base);
	setAltUITextSmall(statsNames);
	statsNames->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	statsNumbers = guienv->addStaticText(L"", recti(position2di(240, 50), dimension2du(30, 130)), false, true, base);
	setAltUITextSmall(statsNumbers);
	description = guienv->addStaticText(L"", recti(position2di(160, 180), dimension2du(300, 100)), false, true, base);
	setUIText(description);
	wepImage = guienv->addImage(recti(position2di(270, 50), dimension2du(190, 130)), base);
	scaleAlign(wepImage);
	wepImage->setVisible(false);

	auto img = guienv->addImage(recti(position2di(-1, -1), dimension2du(1, 251)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);
	img = guienv->addImage(recti(position2di(-1, -1), dimension2du(301, 1)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);
	img = guienv->addImage(recti(position2di(300, -1), dimension2du(1, 251)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);
	img = guienv->addImage(recti(position2di(0, 250), dimension2du(301, 1)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);

	img = guienv->addImage(recti(position2di(-1, 20), dimension2du(301, 1)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);
	img = guienv->addImage(recti(position2di(-1, 150), dimension2du(301, 1)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);

	img = guienv->addImage(recti(position2di(79, 20), dimension2du(1, 130)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);
	img = guienv->addImage(recti(position2di(110, 20), dimension2du(1, 130)), lineBase);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);
	img->setNotClipped(true);

	guiController->setCallback(regList, std::bind(&WeaponBuildTab::onRegular, this, std::placeholders::_1), which);
	guiController->setCallback(hvyList, std::bind(&WeaponBuildTab::onHeavy, this, std::placeholders::_1), which);
	guiController->setCallback(physList, std::bind(&WeaponBuildTab::onPhysical, this, std::placeholders::_1), which);
	guiController->setCallback(confirm, std::bind(&WeaponBuildTab::onConfirm, this, std::placeholders::_1), which);
}

void WeaponBuildTab::show() 
{
	base->setVisible(true);
	m_showSupplies();
	m_clearList();
	m_wipe();
	if (fab->isBuilding()) {
		confirm->setText(L"Build Weapon");
		confirm->setToolTipText(L"Build this weapon.");
		setHoloButton(confirm, BCOL_GREEN);
		m_buildBuildableWepList();
	}
	else {
		confirm->setText(L"Scrap Weapon");
		confirm->setToolTipText(L"Scrap this weapon.");
		setHoloButton(confirm, BCOL_RED);
		m_buildWepList(std::bind(&WeaponBuildTab::onWepScrapSelect, this, std::placeholders::_1), true, curType, false, BCOL_RED);
	}
	if (camera) camera->remove();
	camera = smgr->addCameraSceneNode(0, vector3df(0,2.8,0), wepPos, -1, true);
	camera->setFOV(PI / 4.1);
	wepImage->setImage(guiController->getRenderTex());
}

void WeaponBuildTab::hide()
{
	FabMenuTab::hide();
	if (camera) camera->remove();
	camera = nullptr;
	m_wipe();
	wepImage->setVisible(false);
	lineBase->setVisible(false);

}

bool WeaponBuildTab::onRegular(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	currentSelection = -1;
	curType = HRDP_REGULAR;
	this->show();
	regList->setEnabled(false);
	physList->setEnabled(true);
	hvyList->setEnabled(true);
	setThinHoloButton(regList);
	setThinHoloButton(physList, BCOL_BLUE);
	setThinHoloButton(hvyList, BCOL_BLUE);
	return false;
}

bool WeaponBuildTab::onPhysical(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	currentSelection = -1;
	curType = HRDP_PHYSICS;
	this->show();
	regList->setEnabled(true);
	physList->setEnabled(false);
	hvyList->setEnabled(true);
	setThinHoloButton(regList, BCOL_BLUE);
	setThinHoloButton(physList);
	setThinHoloButton(hvyList, BCOL_BLUE);
	return false;
}

bool WeaponBuildTab::onHeavy(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	currentSelection = -1;
	curType = HRDP_HEAVY;
	this->show();
	regList->setEnabled(true);
	physList->setEnabled(true);
	hvyList->setEnabled(false);
	setThinHoloButton(regList, BCOL_BLUE);
	setThinHoloButton(physList, BCOL_BLUE);
	setThinHoloButton(hvyList);
	return false;
}

void WeaponBuildTab::m_wipe()
{
	name->setText(L"");
	description->setText(L"");
	statsNames->setText(L"");
	statsNumbers->setText(L"");
	if (wepMesh) smgr->getMeshCache()->removeMesh(wepMesh);
	wepMesh = nullptr;
	if (rotatingWep) rotatingWep->remove();
	rotatingWep = nullptr;
	wepImage->setVisible(false);
	confirm->setVisible(false);
	lineBase->setVisible(false);
}

void WeaponBuildTab::m_displayWep(WeaponData* dat)
{

	if (wepMesh) smgr->getMeshCache()->removeMesh(wepMesh);
	wepMesh = nullptr;
	if (rotatingWep) rotatingWep->remove();
	rotatingWep = nullptr;
	wepImage->setVisible(false);

	name->setText(wstr(dat->name).c_str());
	description->setText(wstr(dat->description).c_str());
	std::wstring namestr = L"Damage Type\nDamage\nFire Rate\nRange\nVelocity\n";
	if (dat->wepComp.projectilesPerShot > 1) namestr += L"Projectiles Per Shot\n";
	if (dat->wepComp.usesAmmunition) namestr += L"Clip Size\nMax Ammunition\nReload Time\n";
	if (dat->wepComp.usesPower) namestr += L"Power Cost\n";
	namestr += L"Cost\nScrap Value";
	statsNames->setText(namestr.c_str());
	std::string descstr = "";
	for (const auto& [key, val] : damageStrings) {
		if (val == dat->wepComp.dmgtype) {
			std::string upperkey = key;
			upperkey[0] = std::toupper(upperkey[0]);
			descstr += upperkey + "\n";
		}
	}
	descstr += fprecis(dat->wepComp.damage, 5) + "\n";
	descstr += fprecis(dat->wepComp.firingSpeed, 4) + "\n";
	descstr += fprecis(((dat->wepComp.projectileSpeed / .1f) * dat->wepComp.lifetime), 5) + "\n";
	descstr += fprecis(dat->wepComp.projectileSpeed, 5) + "\n";
	if (dat->wepComp.projectilesPerShot > 1) descstr += std::to_string(dat->wepComp.projectilesPerShot) + "\n";
	if (dat->wepComp.usesAmmunition) {
		descstr += std::to_string(dat->wepComp.maxClip) + "\n";
		descstr += std::to_string(dat->wepComp.maxAmmunition) + "\n";
		descstr += fprecis(dat->wepComp.reloadTime, 4) + "\n";
	}
	if (dat->wepComp.usesPower) descstr += fprecis(dat->wepComp.powerCost, 5) + "\n";
	descstr += fprecis(dat->buildCost, 7) + "\n";
	descstr += fprecis(dat->buildCost / 2, 7);
	statsNumbers->setText(wstr(descstr).c_str());

	wepMesh = smgr->getMesh(dat->weaponMesh.c_str());
	wepImage->setVisible(true);
	wepImage->setImage(guiController->getRenderTex());
	rotatingWep = smgr->addMeshSceneNode(wepMesh, 0, -1, wepPos);
	rotatingWep->setMaterialFlag(EMF_WIREFRAME, true);
	rotatingWep->setMaterialFlag(EMF_LIGHTING, false);
	rotatingWep->setScale(vector3df(2));
	auto anim = smgr->createRotationAnimator(vector3df(0, .7f, 0));
	rotatingWep->addAnimator(anim);
	anim->drop();
	lineBase->setVisible(true);

}

bool WeaponBuildTab::onWepScrapSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (id == currentSelection) {
		m_wipe();
		confirm->setVisible(false);
		currentSelection = -1;
		return false;
	} 
	currentSelection = id;
	dataId wepid = campaign->getWeapon(id)->wep.wepDataId;
	WeaponData* dat = nullptr;
	if (curType == HRDP_PHYSICS) dat = physWeaponData[wepid];
	else if (curType == HRDP_HEAVY) dat = heavyWeaponData[wepid];
	else dat = weaponData[wepid];
	m_displayWep(dat);
	confirm->setVisible(true);
	return false;
}
bool WeaponBuildTab::onWepBuildSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (id == currentSelection) {
		m_wipe();
		confirm->setVisible(false);
		currentSelection = -1;
		return false;
	}
	currentSelection = id;
	WeaponData* dat = nullptr;
	if (curType == HRDP_PHYSICS) dat = physWeaponData[id];
	else if (curType == HRDP_HEAVY) dat = heavyWeaponData[id];
	else dat = weaponData[id];
	m_displayWep(dat);
	confirm->setVisible(true);
	return false;
}
bool WeaponBuildTab::onConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;

	if (campaign->getFlag(L"MARTIN_BUILDING_CLOAK")) {
		guiController->setOkPopup("Locked", "The fabrication bay is not currently operational, owing to Martin's ongoing work with the cloaking device.");
		guiController->showOkPopup();
		return true;
	}

	if (fab->isBuilding()) {
		WeaponData* dat = nullptr;
		if (curType == HRDP_PHYSICS) dat = physWeaponData[currentSelection];
		else if (curType == HRDP_HEAVY) dat = heavyWeaponData[currentSelection];
		else dat = weaponData[currentSelection];

		if (dat->buildCost > campaign->getSupplies()) {
			guiController->setOkPopup("Not Enough Supplies", "You don't have enough supplies to build this weapon. Scrapping some things you're not using might be a good idea.", "Fine");
			guiController->showOkPopup();
			return false;
		}
		campaign->removeSupplies(dat->buildCost);
		campaign->createNewWeaponInstance(dat->wepComp);
		audioDriver->playMenuSound("item_build.ogg");
	}
	else {
		WeaponInstance* wep = campaign->getWeapon(currentSelection);
		if (!wep) return true; //the hell?
		if (wep->usedBy > -1) {
			guiController->setOkPopup("Weapon In Use", "This weapon is currently in use. Take it off before scrapping. \n\n Wait, how'd you do that? This shouldn't display.", "Owned");
			guiController->showOkPopup();
			return false;
		}
		if (wep->wep.wepDataId == 30) {
			guiController->setOkPopup("Excalibur", "This weapon may only be properly disposed of by the Lady of the Lake.");
			guiController->showOkPopup();
			return false;
		}
		bool upgrades = false;
		for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
			if (wep->upgrades[i] > -1) {
				upgrades = true;
				break;
			}
		}
		if (upgrades) {
			guiController->setOkPopup("Upgrades Attached", "This weapon still has upgrades attached. Hang onto those! You can re-use them later.", "Fine");
			guiController->showOkPopup();
			return false;
		}
		//all clear
		audioDriver->playMenuSound("item_scrap.ogg");
		campaign->removeWeapon(wep);
		confirm->setVisible(false);
		m_wipe();
		WeaponData* dat = campaign->getWepDataFromInstance(wep);
		campaign->addSupplies(dat->buildCost / 2.f);
		delete wep;
		m_clearList();
		m_buildWepList(std::bind(&WeaponBuildTab::onWepScrapSelect, this, std::placeholders::_1), true, curType, false, BCOL_RED);
	}
	m_showSupplies();
	return false;
}

void WeaponBuildTab::m_buildBuildableWepList()
{
	std::unordered_map<s32, WeaponData*>* map = nullptr;
	if (curType == HRDP_PHYSICS) map = &physWeaponData;
	else if (curType == HRDP_HEAVY) map = &heavyWeaponData;
	else map = &weaponData;

	u32 count = 0;
	for (auto [id, wep] : *map) {
		if (!wep) continue; //wha?
		if (!wep->canBuild) continue;
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, id, wstr(wep->name).c_str(), L"Select this weapon to build.");
		setThinHoloButton(button, BCOL_GREEN);
		guiController->setCallback(button, std::bind(&WeaponBuildTab::onWepBuildSelect, this, std::placeholders::_1), which);
		listItems.push_back(button);
	}
}