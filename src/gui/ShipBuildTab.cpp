#include "ShipBuildTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include "ShipComponent.h"
#include <iostream>

void ShipBuildTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which)
{
	FabMenuTab::m_basebg(root);
	dialog = dial;
	this->which = which;
	guiController->setCallback(confirm, std::bind(&ShipBuildTab::onConfirm, this, std::placeholders::_1), which);
	if (desc) desc->remove();
	if (longdesc) longdesc->remove();
	desc = nullptr;
	longdesc = nullptr;
	fab = (GuiFabMenu*)dial;
	shipPos = vector3df(0, 0, 18);
	name = guienv->addStaticText(L"", recti(position2di(160, 30), dimension2du(300, 20)), false, true, base);
	setAltUIText(name);
	statsNames = guienv->addStaticText(L"", recti(position2di(160, 50), dimension2du(80, 130)), false, true, base);
	setAltUITextSmall(statsNames);
	statsNames->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	statsNumbers = guienv->addStaticText(L"", recti(position2di(240, 50), dimension2du(30, 130)), false, true, base);
	setAltUITextSmall(statsNumbers);
	description = guienv->addStaticText(L"", recti(position2di(160, 180), dimension2du(300, 100)), false, true, base);
	setUIText(description);
	shipImage = guienv->addImage(recti(position2di(270, 50), dimension2du(190, 130)), base);
	scaleAlign(shipImage);

	lineBase = guienv->addStaticText(L"", recti(vector2di(160, 30), dimension2du(250, 300)), false, false, base);
	scaleAlign(lineBase);

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

	img = guienv->addImage(recti(position2di(-1,20), dimension2du(301, 1)), lineBase);
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

	shipImage->setVisible(false);
	lineBase->setVisible(false);
}

void ShipBuildTab::hide()
{
	FabMenuTab::hide();
	if (camera) camera->remove();
	camera = nullptr;
	m_cleanMeshes();
	shipImage->setVisible(false);
	lineBase->setVisible(false);
}

void ShipBuildTab::show()
{
	FabMenuTab::show();
	m_clearList();
	m_showSupplies();
	currentSelection = -1;
	name->setText(L"");
	description->setText(L"");
	statsNames->setText(L"");
	statsNumbers->setText(L"");
	shipImage->setVisible(false);
	lineBase->setVisible(false);
	if (fab->isBuilding()) {
		confirm->setText(L"Build Ship");
		confirm->setToolTipText(L"Build this ship.");
		setHoloButton(confirm, BCOL_GREEN);
		m_buildBuildableShipList();
	}
	else {
		confirm->setText(L"Scrap Ship");
		confirm->setToolTipText(L"Scrap this ship.");
		setHoloButton(confirm, BCOL_RED);
		m_buildHangarShipList();
	}
	if (camera) camera->remove();
	camera = smgr->addCameraSceneNode(0, vector3df(0,2.8,0), shipPos, -1, true);
	camera->setFOV(PI/4.1);
	shipImage->setImage(guiController->getRenderTex());
}

void ShipBuildTab::m_cleanMeshes()
{
	if (shipMesh) smgr->getMeshCache()->removeMesh(shipMesh);
	shipMesh = nullptr;
	if (rotatingShip) rotatingShip->remove();
	rotatingShip = nullptr;
}

void ShipBuildTab::m_setText(ShipData* dat)
{
	name->setText(wstr(dat->name).c_str());
	description->setText(wstr(dat->description).c_str());
	std::wstring stats = L"Health:\nHardpoints:\nForward:\nBrake:\nBoost:\nPitch:\nYaw:\nRoll:\nMax Speed:\nVelocity Tolerance:\nCost:\nScrap Value:\n";
	statsNames->setText(stats.c_str());
	std::wstring nums = L"";
	nums += wstr(fprecis(dat->hp.maxHealth, 7)) + L"\n";
	nums += std::to_wstring(dat->hards.hardpointCount) + L"\n";
	nums += wstr(fprecis(dat->thrust.forward, 7)) + L"\n";
	nums += wstr(fprecis(dat->thrust.brake, 7)) + L"\n";
	nums += wstr(fprecis(dat->thrust.boost, 7)) + L"\n";
	nums += wstr(fprecis(dat->thrust.pitch, 7)) + L"\n";
	nums += wstr(fprecis(dat->thrust.yaw, 7)) + L"\n";
	nums += wstr(fprecis(dat->thrust.roll, 7)) + L"\n";
	nums += wstr(fprecis(dat->thrust.linearMaxVelocity, 7)) + L"\n";
	nums += wstr(fprecis(dat->thrust.velocityTolerance, 4)) + L"\n";
	nums += wstr(fprecis(dat->buildCost, 7)) + L"\n";
	nums += wstr(fprecis(dat->buildCost / 2.f, 7)) + L"\n";
	statsNumbers->setText(nums.c_str());

	m_cleanMeshes();

	shipMesh = smgr->getMesh(dat->shipMesh.c_str());
	shipImage->setVisible(true);
	shipImage->setImage(guiController->getRenderTex());
	rotatingShip = smgr->addMeshSceneNode(shipMesh, 0, -1, shipPos);
	rotatingShip->setMaterialFlag(EMF_WIREFRAME, true);
	rotatingShip->setMaterialFlag(EMF_LIGHTING, false);
	rotatingShip->setScale(dat->scale);
	auto anim = smgr->createRotationAnimator(vector3df(0, .7f, 0));
	rotatingShip->addAnimator(anim);
	anim->drop();

	lineBase->setVisible(true);
}

bool ShipBuildTab::onBuildShipSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (id == currentSelection) {
		confirm->setVisible(false);
		currentSelection = -1;
		name->setText(L"");
		description->setText(L"");
		statsNames->setText(L"");
		statsNumbers->setText(L"");
		shipImage->setVisible(false);
		lineBase->setVisible(false);
		return false;
	}
	ShipData* dat = shipData[id];
	if (!dat) return false; //?!!!
	currentSelection = id;
	m_setText(dat);
	confirm->setVisible(true);
	return false;
}

bool ShipBuildTab::onScrapShipSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (id == currentSelection) {
		confirm->setVisible(false);
		currentSelection = -1;
		name->setText(L"");
		description->setText(L"");
		statsNames->setText(L"");
		statsNumbers->setText(L"");
		shipImage->setVisible(false);
		lineBase->setVisible(false);
		return false;
	}
	currentSelection = id;
	auto dat = shipData[campaign->getShip(id)->ship.shipDataId];
	m_setText(dat);
	confirm->setVisible(true);
	return false;
}

void ShipBuildTab::m_buildHangarShipList()
{
	m_buildShipList(std::bind(&ShipBuildTab::onScrapShipSelect, this, std::placeholders::_1), true, false, BCOL_RED);
}

bool ShipBuildTab::onConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;

	if (campaign->getFlag(L"MARTIN_BUILDING_CLOAK")) {
		guiController->setOkPopup("Locked", "The fabrication bay is not currently operational, owing to Martin's ongoing work with the cloaking device.");
		guiController->showOkPopup();
		return true;
	}

	if (fab->isBuilding()) {
		auto data = shipData[currentSelection];
		if (!data) {
			return true;
		}
		if (campaign->getSupplies() < data->buildCost) {
			guiController->setOkPopup("Not Enough Supplies", "You don't have enough supplies to build this ship. Scrapping some leftovers might be a good idea.", "Fine");
			guiController->showOkPopup();
			return false;
		}
		campaign->addShipInstanceToHangar(campaign->buildShipInstanceFromData(data));
		campaign->removeSupplies(data->buildCost);
	}
	else {
		auto inst = campaign->getShip(currentSelection);
		if (!inst) return true;
		bool assigned = false;
		for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
			if (inst->weps[i] > -1) {
				assigned = true;
				break;
			}
		}
		if (inst->inUseBy > -1) {
			guiController->setOkPopup("Ship In Use", "This ship is still in use by someone. Don't scrap it! \n\n Actually, how did you manage this? The list doesn't show ships in use.", "Magic");
			guiController->showOkPopup();
			return false;
		}
		if (assigned || inst->physWep > -1 || inst->heavyWep > -1) {
			guiController->setOkPopup("Weapons Attached", "This ship still has weapons attached. Hang onto them!", "Fine");
			guiController->showOkPopup();
			return false;
		}
		//assigned has to still be false
		for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
			if (inst->upgrades[i] > -1) {
				assigned = true;
				break;
			}
		}
		if (assigned) {
			guiController->setOkPopup("Upgrades Attached", "This ship still has some upgrades attached. Hang onto them!", "Fine");
			guiController->showOkPopup();
			return false;
		}
		//nothing is attached and its not in use. scrap it
		campaign->removeShipInstance(inst);
		campaign->addSupplies(shipData[inst->ship.shipDataId]->buildCost / 2.f);
		confirm->setVisible(false);
		name->setText(L"");
		description->setText(L"");
		statsNames->setText(L"");
		statsNumbers->setText(L"");
		shipImage->setVisible(false);
		lineBase->setVisible(false);
		delete inst;
		m_clearList();
		m_buildHangarShipList();
	}
	//successful build / scrap
	m_showSupplies();
	return false;
}

void ShipBuildTab::m_buildBuildableShipList()
{
	u32 count = 0;
	for (auto [id, ship] : shipData) {
		if (!ship->canBuild) continue;
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, id, wstr(ship->name).c_str(), L"Select this ship to build.");
		setThinHoloButton(button, BCOL_GREEN);
		guiController->setCallback(button, std::bind(&ShipBuildTab::onBuildShipSelect, this, std::placeholders::_1), which);
		listItems.push_back(button);
	}
}