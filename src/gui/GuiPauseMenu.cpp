#include "GuiPauseMenu.h"
#include "GuiController.h"
#include "GameStateController.h"
#include "AudioDriver.h"

void GuiPauseMenu::init()
{
	if (root) root->remove();
	root = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), baseSize));

	dimension2du screenSize = driver->getScreenSize();
	u32 verticalSlice = baseSize.Height / 6;
	u32 horizontalPos = baseSize.Width / 3;
	dimension2du buttonSize(horizontalPos, verticalSlice);

	resumeGame = guienv->addButton(rect<s32>(position2di(horizontalPos, 32), buttonSize), root, PAUSEMENU_RESUME, L"Resume Game", L"Get back in there!");
	pauseSettings = guienv->addButton(rect<s32>(position2di(horizontalPos, 32 * 2 + verticalSlice), buttonSize), root, PAUSEMENU_SETTINGS, L"Settings", L"What, is your sensitivity too low?");
	exitToMenus = guienv->addButton(rect<s32>(position2di(horizontalPos, 32 * 3 + verticalSlice*2), buttonSize), root, PAUSEMENU_EXIT, L"Exit to Main Menu", L"Run, coward!");
	setMetalButton(resumeGame);
	setMetalButton(pauseSettings);
	setMetalButton(exitToMenus);

	guiController->setCallback(resumeGame, std::bind(&GuiPauseMenu::onResume, this, std::placeholders::_1), GUI_PAUSE_MENU);
	guiController->setCallback(pauseSettings, std::bind(&GuiPauseMenu::onSettings, this, std::placeholders::_1), GUI_PAUSE_MENU);
	guiController->setCallback(exitToMenus, std::bind(&GuiPauseMenu::onExit, this, std::placeholders::_1), GUI_PAUSE_MENU);

	hide();
}

bool GuiPauseMenu::onResume(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;

	stateController->setState(GAME_RUNNING);
	return false;
}
bool GuiPauseMenu::onSettings(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_SETTINGS_MENU);
	return false;
}
bool GuiPauseMenu::onExit(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	campaign->exitCampaign();
	stateController->inCampaign = false;
	stateController->setState(GAME_MENUS);
	audioDriver->playMusic("main_menu.ogg");
	audioDriver->setMusicGain(0, 1.f);
	audioDriver->stopMusic(1);
	return false;
}