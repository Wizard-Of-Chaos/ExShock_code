#include "GuiFabMenu.h"
#include "GuiController.h"
#include "Campaign.h"
#include "GameStateController.h"
#include "AudioDriver.h"

void _setButtons(bool building, IGUIButton* build, IGUIButton* scrap)
{
	if (building) {
		setButtonImg(build, "assets/ui/fabbuild_selected.png", "assets/ui/fabbuild_selected.png");
		build->setEnabled(false);
		setButtonImg(scrap, "assets/ui/fabscrap.png", "assets/ui/fabscrap_click.png");
		scrap->setEnabled(true);
	}
	else {
		setButtonImg(build, "assets/ui/fabbuild.png", "assets/ui/fabbuild_click.png");
		build->setEnabled(true);
		setButtonImg(scrap, "assets/ui/fabscrap_selected.png", "assets/ui/fabscrap_selected.png");
		scrap->setEnabled(false);
	}
}


void GuiFabMenu::init()
{
	if (root) root->remove();
	auto bg = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	bg->setImage(driver->getTexture("assets/ui/fabScreen.png"));
	bg->setScaleImage(true);
	scaleAlign(bg);
	root = bg;

	position2di basePos(260, 60);
	position2di baseSidePos(240, 80);
	dimension2du size(115, 20);
	build = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Build", L"View build lists for the fab bay.");
	setButtonImg(build, "assets/ui/fabbuild.png", "assets/ui/fabbuild_click.png");
	build->setUseAlphaChannel(true);
	build->setDrawBorder(false);
	basePos.X += size.Width;
	scrap = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Scrap", L"View scrap lists for the fab bay.");
	setButtonImg(scrap, "assets/ui/fabscrap.png", "assets/ui/fabscrap_click.png");
	scrap->setUseAlphaChannel(true);
	scrap->setDrawBorder(false);

	dimension2du sideButtonSize(20, 84);

	ships = guienv->addButton(rect<s32>(baseSidePos, sideButtonSize), root, -1, L"", L"Select ship tab.");
	setButtonImg(ships, "assets/ui/fab_ship_selected.png", "assets/ui/fab_ship_selected.png");
	ships->setUseAlphaChannel(true);
	ships->setDrawBorder(false);

	baseSidePos.Y += sideButtonSize.Height;

	weps = guienv->addButton(rect<s32>(baseSidePos, sideButtonSize), root, -1, L"", L"Select weapon tab.");
	weps->setUseAlphaChannel(true);
	weps->setDrawBorder(false);
	setButtonImg(weps, "assets/ui/fab_weapon.png", "assets/ui/fab_weapon_click.png");
	baseSidePos.Y += sideButtonSize.Height;

	upgrades = guienv->addButton(rect<s32>(baseSidePos, sideButtonSize), root, -1, L"", L"Select upgrade tab.");
	setButtonImg(upgrades, "assets/ui/fab_upgrade.png", "assets/ui/fab_upgrade_click.png");
	upgrades->setUseAlphaChannel(true);
	upgrades->setDrawBorder(false);
	baseSidePos.Y += sideButtonSize.Height;

	carrierUpgrades = guienv->addButton(rect<s32>(baseSidePos, sideButtonSize), root, -1, L"", L"Select carrier tab.");
	setButtonImg(carrierUpgrades, "assets/ui/fab_carrier.png", "assets/ui/fab_carrier_click.png");
	carrierUpgrades->setUseAlphaChannel(true);
	carrierUpgrades->setDrawBorder(false);
	baseSidePos.Y += sideButtonSize.Height;

	supplyBg = guienv->addStaticText(L"", recti(position2di(490, 60), dimension2du(80, 20)), false, false, root);
	setUIText(supplyBg);
	auto img = guienv->addImage(recti(position2di(0, 0), dimension2du(20, 20)), supplyBg);
	scaleAlign(img);
	img->setImage(driver->getTexture("assets/ui/supplies.png"));
	supplyCount = guienv->addStaticText(L"0", recti(position2di(20, 0), dimension2du(20, 20)), false, false, supplyBg);
	setAltUITextSmall(supplyCount);
	supplyCount->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	img = guienv->addImage(recti(position2di(40, 0), dimension2du(20, 20)), supplyBg);
	scaleAlign(img);
	img->setImage(driver->getTexture("assets/ui/ammo.png"));
	ammoCount = guienv->addStaticText(L"0", recti(position2di(60, 0), dimension2du(20, 20)), false, false, supplyBg);
	setAltUITextSmall(ammoCount);
	ammoCount->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	nav.build(root, GUI_FAB_MENU);

	ammo = guienv->addButton(rect<s32>(position2di(570, 60), dimension2du(50, 20)), root, -1, L"Build Ammo", L"Trade 5 supplies for 10 ammo.");
	setThinHoloButton(ammo, BCOL_YELLOW);
	stripUnassigned = guienv->addButton(rect<s32>(position2di(620, 60), dimension2du(70, 20)), root, -1, L"Strip Unused", L"Strip unassigned ships of weapons and upgrades.");
	setThinHoloButton(stripUnassigned, BCOL_ORANGE);

	guiController->setCallback(ammo, std::bind(&GuiFabMenu::onAmmo, this, std::placeholders::_1), GUI_FAB_MENU);
	guiController->setCallback(stripUnassigned, std::bind(&GuiFabMenu::onStripUnassigned, this, std::placeholders::_1), GUI_FAB_MENU);
	guiController->setCallback(ships, std::bind(&GuiFabMenu::onShips, this, std::placeholders::_1), GUI_FAB_MENU);
	guiController->setCallback(weps, std::bind(&GuiFabMenu::onWeps, this, std::placeholders::_1), GUI_FAB_MENU);
	guiController->setCallback(upgrades, std::bind(&GuiFabMenu::onUpgrades, this, std::placeholders::_1), GUI_FAB_MENU);
	guiController->setCallback(carrierUpgrades, std::bind(&GuiFabMenu::onCarrier, this, std::placeholders::_1), GUI_FAB_MENU);
	guiController->setCallback(build, std::bind(&GuiFabMenu::onBuild, this, std::placeholders::_1), GUI_FAB_MENU);
	guiController->setCallback(scrap, std::bind(&GuiFabMenu::onScrap, this, std::placeholders::_1), GUI_FAB_MENU);
	shipBuild.build(root, this, GUI_FAB_MENU);
	wepBuild.build(root, this, GUI_FAB_MENU);
	upgradeBuild.build(root, this, GUI_FAB_MENU);
	carrierBuild.build(root, this, GUI_FAB_MENU);
	wepBuild.hide();
	upgradeBuild.hide();
	carrierBuild.hide();
	shipBuild.show();
	curTab = &shipBuild;
	_setButtons(building, build, scrap);
	hide();
}
void GuiFabMenu::show()
{
	GuiDialog::show();
	showSupplies();
	curTab->show();
}

void GuiFabMenu::hide()
{
	GuiDialog::hide();
	if (curTab) curTab->hide();
}

bool GuiFabMenu::onShips(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	build->setVisible(true);
	scrap->setVisible(true);
	ships->setEnabled(false);
	weps->setEnabled(true);
	upgrades->setEnabled(true);
	carrierUpgrades->setEnabled(true);
	setButtonImg(ships, "assets/ui/fab_ship_selected.png", "assets/ui/fab_ship_selected.png");
	setButtonImg(weps, "assets/ui/fab_weapon.png", "assets/ui/fab_weapon_click.png");
	setButtonImg(upgrades, "assets/ui/fab_upgrade.png", "assets/ui/fab_upgrade_click.png");
	setButtonImg(carrierUpgrades, "assets/ui/fab_carrier.png", "assets/ui/fab_carrier_click.png");

	shipBuild.show();
	curTab = &shipBuild;
	return false;
}
bool GuiFabMenu::onWeps(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	build->setVisible(true);
	scrap->setVisible(true);

	ships->setEnabled(true);
	weps->setEnabled(false);
	upgrades->setEnabled(true);
	carrierUpgrades->setEnabled(true);
	setButtonImg(ships, "assets/ui/fab_ship.png", "assets/ui/fab_ship_click.png");
	setButtonImg(weps, "assets/ui/fab_weapon_selected.png", "assets/ui/fab_weapon_selected.png");
	setButtonImg(upgrades, "assets/ui/fab_upgrade.png", "assets/ui/fab_upgrade_click.png");
	setButtonImg(carrierUpgrades, "assets/ui/fab_carrier.png", "assets/ui/fab_carrier_click.png");

	wepBuild.show();
	curTab = &wepBuild;
	return false;
}
bool GuiFabMenu::onUpgrades(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	build->setVisible(true);
	scrap->setVisible(true);

	ships->setEnabled(true);
	weps->setEnabled(true);
	upgrades->setEnabled(false);
	carrierUpgrades->setEnabled(true);
	setButtonImg(ships, "assets/ui/fab_ship.png", "assets/ui/fab_ship_click.png");
	setButtonImg(weps, "assets/ui/fab_weapon.png", "assets/ui/fab_weapon_click.png");
	setButtonImg(upgrades, "assets/ui/fab_upgrade_selected.png", "assets/ui/fab_upgrade_selected.png");
	setButtonImg(carrierUpgrades, "assets/ui/fab_carrier.png", "assets/ui/fab_carrier_click.png");

	upgradeBuild.show();
	curTab = &upgradeBuild;
	return false;
}
bool GuiFabMenu::onCarrier(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (curTab) curTab->hide();
	build->setVisible(false);
	scrap->setVisible(false);

	ships->setEnabled(true);
	weps->setEnabled(true);
	upgrades->setEnabled(true);
	carrierUpgrades->setEnabled(false);
	setButtonImg(ships, "assets/ui/fab_ship.png", "assets/ui/fab_ship_click.png");
	setButtonImg(weps, "assets/ui/fab_weapon.png", "assets/ui/fab_weapon_click.png");
	setButtonImg(upgrades, "assets/ui/fab_upgrade.png", "assets/ui/fab_upgrade_click.png");
	setButtonImg(carrierUpgrades, "assets/ui/fab_carrier_selected.png", "assets/ui/fab_carrier_selected.png");

	carrierBuild.show();
	curTab = &carrierBuild;
	return false;
}
bool GuiFabMenu::onBuild(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	building = true;
	if(curTab) curTab->show();
	_setButtons(building, build, scrap);
	return false;
}
bool GuiFabMenu::onScrap(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	building = false;
	if (curTab) curTab->show();
	_setButtons(building, build, scrap);
	return false;
}

bool GuiFabMenu::onAmmo(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	f32 pay = campaign->removeSupplies(5.f);
	if (pay < 5.f) {
		guiController->setOkPopup("Not Enough Supplies",
			"You don't have the supplies to be able to craft any ammo. \n\n That's a shame. Better do a mission.", "Bullets?");
		guiController->showOkPopup();
		campaign->addSupplies(pay);
		return false;
	}
	audioDriver->playMenuSound("ammo_build.ogg");
	campaign->addAmmo(10);
	showSupplies();
	return false;
}

bool GuiFabMenu::onStripUnassigned(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	campaign->stripAllUnassignedShips();
	campaign->stripAllUnassignedWeapons();
	guiController->setOkPopup("Stripped", "All unassigned ships have been stripped of weapons and upgrades. Unassigned weapons have been stripped of their upgrades.");
	guiController->showOkPopup();
	return false;
}

void GuiFabMenu::showSupplies()
{
	if (!stateController->inCampaign) return;
	if (supplyBg) supplyBg->setVisible(true);
	if (supplyCount) supplyCount->setText(wstr(fprecis(campaign->getSupplies(), 7)).c_str());
	if (ammoCount) ammoCount->setText(std::to_wstring(campaign->getAmmo()).c_str());
}