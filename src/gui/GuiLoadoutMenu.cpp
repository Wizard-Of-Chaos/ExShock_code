#include "GuiLoadoutMenu.h"
#include "GameController.h"
#include "GuiController.h"
#include "GameStateController.h"
#include "CrashLogger.h"

void GuiLoadoutMenu::init()
{
	if (root) root->remove();
	auto bg = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	bg->setImage(driver->getTexture("assets/ui/loadoutScreen.png"));
	bg->setScaleImage(true);
	scaleAlign(bg);
	root = bg;

	martin = guienv->addButton(rect<s32>(position2di(125, 208), dimension2du(250, 332)), root, -1, L"", L"Talk to Martin.");
	setButtonImg(martin, "assets/ui/martin.png", "assets/ui/martinhover.png");
	martin->setUseAlphaChannel(true);
	martin->setDrawBorder(false);

	nav.build(root, GUI_LOADOUT_MENU);
	s32 topSizeX = 300/ 4;
	dimension2du topSize(topSizeX, 15);
	s32 topY = 115;
	wing = guienv->addButton(rect<s32>(position2di(481, 115), topSize), root, -1, L"Wing", L"Select loadouts for yourself and your wingmen.");
	ships = guienv->addButton(rect<s32>(position2di(481 + (topSize.Width), 115), topSize), root, -1, L"Ships", L"View ships and adjust their loadouts and upgrades.");
	weps = guienv->addButton(rect<s32>(position2di(481 + (topSize.Width * 2), 115), topSize), root, -1, L"Weapons", L"View available weapons and adjust their upgrades.");
	carrier = guienv->addButton(rect<s32>(position2di(481 + (topSize.Width * 3), 115), topSize), root, -1, L"Carrier", L"View carrier status and adjust any upgrades.");
	setHoloButton(wing);
	setHoloButton(ships);
	setHoloButton(weps);
	setHoloButton(carrier);
	guiController->setCallback(wing, std::bind(&GuiLoadoutMenu::onWing, this, std::placeholders::_1), GUI_LOADOUT_MENU);
	guiController->setCallback(ships, std::bind(&GuiLoadoutMenu::onShips, this, std::placeholders::_1), GUI_LOADOUT_MENU);
	guiController->setCallback(weps, std::bind(&GuiLoadoutMenu::onWeps, this, std::placeholders::_1), GUI_LOADOUT_MENU);
	guiController->setCallback(carrier, std::bind(&GuiLoadoutMenu::onCarrier, this, std::placeholders::_1), GUI_LOADOUT_MENU);
	guiController->setCallback(martin, std::bind(&GuiLoadoutMenu::onMartin, this, std::placeholders::_1), GUI_LOADOUT_MENU);

	supplyBg = guienv->addStaticText(L"", recti(position2di(480, 55), dimension2du(130, 20)), false, false, root);
	setUIText(supplyBg);
	auto img = guienv->addImage(recti(position2di(0, 0), dimension2du(20, 20)), supplyBg);
	scaleAlign(img);
	img->setImage(driver->getTexture("assets/ui/supplies.png"));
	supplyCount = guienv->addStaticText(L"0", recti(position2di(20, 0), dimension2du(45, 20)), false, false, supplyBg);
	setAltUITextSmall(supplyCount);
	supplyCount->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	img = guienv->addImage(recti(position2di(65, 0), dimension2du(20, 20)), supplyBg);
	scaleAlign(img);
	img->setImage(driver->getTexture("assets/ui/ammo.png"));
	ammoCount = guienv->addStaticText(L"0", recti(position2di(85, 0), dimension2du(45, 20)), false, false, supplyBg);
	setAltUITextSmall(ammoCount);
	ammoCount->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);

	wingTab.build(root, this, GUI_LOADOUT_MENU);
	shipTab.build(root, this, GUI_LOADOUT_MENU);
	wepTab.build(root, this, GUI_LOADOUT_MENU);
	carrierTab.build(root, this, GUI_LOADOUT_MENU);
	wingTab.hide();
	shipTab.hide();
	wepTab.hide();
	carrierTab.hide();
	curTab = nullptr;
	hide();
}

void GuiLoadoutMenu::show()
{
	GuiDialog::show();
	showSupplies();
}

void GuiLoadoutMenu::hide()
{
	GuiDialog::hide();
	if (curTab) curTab->hide();
	
	if (stateController->inCampaign) {
		bool playerValid = true;
		auto player = campaign->getPlayerShip();
		if (player->heavyWep <= INVALID_DATA_ID || player->physWep <= INVALID_DATA_ID) playerValid = false;
		for (u32 i = 0; i < player->hards.hardpointCount; ++i) {
			if (player->weps[i] <= INVALID_DATA_ID) playerValid = false;
		}
		bool wingmenCount = true;
		u32 count = campaign->getValidWingmanCount();
		u32 wingCount = 0;
		for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
			if (campaign->getAssignedWingman(i)) ++wingCount;
		}
		if (wingCount != MAX_WINGMEN_ON_WING && wingCount < count && campaign->ships().size() >= count) wingmenCount = false;

		bool wingmanWeapons = true;
		for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
			auto ship = campaign->getAssignedShip(i);
			if (ship) {
				for (u32 i = 0; i < ship->hards.hardpointCount; ++i) {
					if (ship->weps[i] <= INVALID_DATA_ID) wingmanWeapons = false;
				}
			}
		}
		if (!playerValid || !wingmenCount || !wingmanWeapons) {
			std::string notif = "";
			if (!playerValid) notif += "Your ship is missing one or more weapons on its loadout.\n\n";
			if (!wingmenCount) notif += "You do not have a full wing assigned.\n\n";
			if (!wingmanWeapons) notif += "One or more of your wingmen is missing weapons on their ship.";
			guiController->setOkPopup("Warning", notif);
			guiController->setLoadoutTrigger();
		}
	}
}

bool GuiLoadoutMenu::onWing(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	wingTab.show();
	curTab = &wingTab;
	return false;
}
bool GuiLoadoutMenu::onShips(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	shipTab.show();
	curTab = &shipTab;
	return false;
}
bool GuiLoadoutMenu::onWeps(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	wepTab.show();
	curTab = &wepTab;
	return false;
}
bool GuiLoadoutMenu::onCarrier(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	carrierTab.show();
	curTab = &carrierTab;
	return false;
}

bool GuiLoadoutMenu::onMartin(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"martin_explanatory"));
	guiController->setActiveDialog(GUI_DIALOGUE_MENU);
	return false; 
}

void GuiLoadoutMenu::showSupplies()
{
	if (!stateController->inCampaign) return;
	if (supplyBg) supplyBg->setVisible(true);
	if (supplyCount) supplyCount->setText(wstr(fprecis(campaign->getSupplies(), 7)).c_str());
	if (ammoCount) ammoCount->setText(std::to_wstring(campaign->getAmmo()).c_str());
}