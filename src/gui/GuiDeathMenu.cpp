#include "GuiDeathMenu.h"
#include "GameController.h"
#include "AudioDriver.h"
#include "GuiController.h"
#include "GameStateController.h"

void GuiDeathMenu::init()
{
	if (root) root->remove();
	root = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), baseSize));

	u32 verticalSlice = baseSize.Height / 6;
	u32 horizontalPos = baseSize.Width / 3;
	dimension2du buttonSize(horizontalPos, verticalSlice);

	returnToCampaign = guienv->addButton(rect<s32>(position2di(horizontalPos, verticalSlice + 8), buttonSize), root, -1, L"Return To Ship", L"Repairs are for winners.");
	options = guienv->addButton(rect<s32>(position2di(horizontalPos, verticalSlice + 8), buttonSize), root, -1, L"Load Save", L"Try again, champ.");
	options->setVisible(false);
	returnToCampaign->setVisible(false);
	returnToMenu = guienv->addButton(rect<s32>(position2di(horizontalPos, verticalSlice * 2 + 8 * 2), buttonSize), root, DEATHMENU_RETURN, L"Main Menu", L"You'll be back.");
	taunt = guienv->addStaticText(guiController->getTaunt().c_str(), rect<s32>(position2di(horizontalPos, 32), buttonSize), false, true, root);

	setUIText(taunt);
	taunt->setOverrideFont(guienv->getFont("assets/fonts/qaz_sans/20.xml"));
	setMetalButton(returnToMenu);
	setMetalButton(returnToCampaign);
	setMetalButton(options);

	guiController->setCallback(returnToMenu, std::bind(&GuiDeathMenu::onReturnMenu, this, std::placeholders::_1), GUI_DEATH_MENU);
	guiController->setCallback(returnToCampaign, std::bind(&GuiDeathMenu::onReturnCampaign, this, std::placeholders::_1), GUI_DEATH_MENU);
	guiController->setCallback(options, std::bind(&GuiDeathMenu::onOptions, this, std::placeholders::_1), GUI_DEATH_MENU);
	hide();
}

void GuiDeathMenu::show()
{
	if (gameController->isPlayerAlive && gameController->objective().get()->success()) {
		taunt->setText(guiController->getCongrats().c_str());
		returnToCampaign->setVisible(true);
		options->setVisible(false);
	}
	else {
		taunt->setText(guiController->getTaunt().c_str());
		returnToCampaign->setVisible(false);
		options->setVisible(true);
	}
	if (stateController->inCampaign) returnToCampaign->setVisible(true);
	else returnToCampaign->setVisible(false);
	root->setRelativePosition(rect<s32>(position2di(0, 0), driver->getScreenSize()));
	root->setVisible(true);
}

bool GuiDeathMenu::onReturnCampaign(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED || !gameController->isPlayerAlive) return true;
	if (stateController->inCampaign) {
		stateController->backToCampaign();
	}
	return false;
}

bool GuiDeathMenu::onReturnMenu(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (stateController->inCampaign) campaign->exitCampaign();
	stateController->inCampaign = false;
	stateController->setState(GAME_MENUS);
	audioDriver->playMusic("main_menu.ogg");
	audioDriver->setMusicGain(0, 1.f);
	audioDriver->stopMusic(1);
	return false;
}

bool GuiDeathMenu::onOptions(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (stateController->inCampaign) campaign->exitCampaign();
	stateController->inCampaign = false;
	stateController->goToOptions = true;
	stateController->setState(GAME_MENUS);
	audioDriver->playMusic("main_menu.ogg");
	audioDriver->setMusicGain(0, 1.f);
	audioDriver->stopMusic(1);
	return false;

}