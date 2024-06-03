#include "GuiOptionsMenu.h"
#include "GuiController.h"
#include "GameStateController.h"
#include "AudioDriver.h"

void GuiOptionsMenu::init()
{
	if (root) root->remove();
	auto bg = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	bg->setImage(driver->getTexture("assets/ui/settingsScreen.png"));
	scaleAlign(bg);
	root = bg;

	position2di basePos(262, 60);
	dimension2du size(84, 20);
	s32 buf = size.Width + 5;

	newGame = guienv->addButton(rect<s32>(basePos, size), root, -1, L"New Game", L"Start a new campaign.");
	setHoloButton(newGame);
	basePos.X += buf;
	load = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Load Game", L"Load a previous campaign.");
	setHoloButton(load);
	basePos.X += buf;
	save = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Save Game", L"Save your campaign.");
	setHoloButton(save);
	basePos.X += buf;
	settings = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Settings", L"Edit settings for the campaign.");
	setHoloButton(settings);
	basePos.X += buf;
	backToMain = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Exit", L"Return to the main menu.");
	setHoloButton(backToMain);

	guiController->setCallback(save, std::bind(&GuiOptionsMenu::onSave, this, std::placeholders::_1), GUI_OPTIONS_MENU);
	guiController->setCallback(load, std::bind(&GuiOptionsMenu::onLoad, this, std::placeholders::_1), GUI_OPTIONS_MENU);
	guiController->setCallback(newGame, std::bind(&GuiOptionsMenu::onNew, this, std::placeholders::_1), GUI_OPTIONS_MENU);
	guiController->setCallback(settings, std::bind(&GuiOptionsMenu::onSettings, this, std::placeholders::_1), GUI_OPTIONS_MENU);
	guiController->setCallback(backToMain, std::bind(&GuiOptionsMenu::onExit, this, std::placeholders::_1), GUI_OPTIONS_MENU);

	nav.build(root, GUI_OPTIONS_MENU);
	saveTab.build(root);
	loadTab.build(root);
	hide();
}

void GuiOptionsMenu::show()
{
	GuiDialog::show();
	if (!stateController->inCampaign) {
		nav.hide();
		save->setVisible(false);
	}
	else {
		nav.show();
		save->setVisible(true);
	}
	saveTab.hide();
	loadTab.hide();
}

bool GuiOptionsMenu::onSave(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (!stateController->inCampaign) {
		audioDriver->playMenuSound("menu_error.ogg");
		return false;
	}
	saveTab.show();
	//campaign->saveCampaign("saves/test_save.xml");
	loadTab.hide();
	return false;
}
bool GuiOptionsMenu::onLoad(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	loadTab.show();
	saveTab.hide();
	//set in campaign
	return false;
}

bool GuiOptionsMenu::onNew(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (stateController->inCampaign) {
		guiController->setYesNoPopup("New Campaign", "Are you sure your want to start a new campaign?\n\nAny progress made since the last save will not be saved.",
			std::bind(&GuiOptionsMenu::onNewConfirm, this, std::placeholders::_1));
		guiController->showYesNoPopup();
		return false;
	}
	return onNewConfirm(event);
}
bool GuiOptionsMenu::onNewConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	stateController->inCampaign = true;
	campaign->newCampaign();
	stateController->toggleMenuBackdrop(false);
	guiController->setActiveDialog(GUI_CAMPAIGN_MENU);
	audioDriver->playMusic(campaign->getSector()->menuMusic);
	return false;
}
bool GuiOptionsMenu::onSettings(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_SETTINGS_MENU);
	return false;
}

bool GuiOptionsMenu::onExit(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (stateController->inCampaign) { //need to exit too
		guiController->setYesNoPopup("Exit", "Are you sure your want to exit?\n\nAny progress made since the last save will not be saved.",
			std::bind(&GuiOptionsMenu::onExitConfirm, this, std::placeholders::_1));
		guiController->showYesNoPopup();
		return false;
	}
	return onExitConfirm(event);
}

bool GuiOptionsMenu::onExitConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	stateController->inCampaign = false;
	campaign->exitCampaign();
	audioDriver->playMusic("main_menu.ogg");
	guiController->setActiveDialog(GUI_MAIN_MENU);
	return false;
}