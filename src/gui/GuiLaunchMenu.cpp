#include "GuiLaunchMenu.h"
#include "GuiController.h"
#include "AudioDriver.h"
#include "GameStateController.h"
#include "Campaign.h"
#include <iostream>

void GuiLaunchMenu::init()
{
	if (root) root->remove();
	IGUIImage* img = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	img->setImage(driver->getTexture("assets/ui/hangarScreen.png"));
	scaleAlign(img);
	root = img;

	launch = guienv->addButton(rect<s32>(position2di(300, 240), dimension2du(329, 255)), root, -1, L"", L"Launch to space!");
	setButtonImg(launch, "assets/ui/shiplaunch.png", "assets/ui/shiplaunchclick.png");
	launch->setUseAlphaChannel(true);
	launch->setDrawBorder(false);
	
	guiController->setCallback(launch, std::bind(&GuiLaunchMenu::onLaunch, this, std::placeholders::_1), GUI_LAUNCH_MENU);
	nav.build(root, GUI_LAUNCH_MENU);
	hide();
}
void GuiLaunchMenu::show()
{
	GuiDialog::show();
	if (campaign->getSector()->getCurrentScenario()) launch->setVisible(true);
	else launch->setVisible(false);
	if (campaign->getFlag(L"EVENT_AVAILABLE")) {
		campaign->rollForEvent();
		guiController->setDialogueTree(campaign->currentEvent());
		guiController->setEventDialoguePopup();
	}
	if (campaign->getSector()->getEncounterNum() == 0 && !campaign->isTreeUsed(campaign->getSector()->introDialogue)) {
		campaign->setDialogueTreeUsed(campaign->getSector()->introDialogue);
		guiController->setDialogueTree(campaign->getCharacterDialogue(campaign->getSector()->introDialogue));
		guiController->setEventDialoguePopup();
	}
}
bool GuiLaunchMenu::onLaunch(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (!campaign->isTreeUsed(L"steven_flight_tutorial")) {
		campaign->setDialogueTreeUsed(L"steven_flight_tutorial");
		guiController->setDialogueTree(campaign->getCharacterDialogue(L"steven_flight_tutorial"));
		guiController->setActiveDialog(GUI_DIALOGUE_MENU);
		return false;
	}
	guiController->setActiveDialog(GUI_LOADING_MENU);
	audioDriver->playMenuSound("menu_confirm.ogg");
	stateController->setState(GAME_RUNNING);
	audioDriver->playMusic(campaign->getSector()->combatMusic, 0);
	audioDriver->setMusicGain(0, 0.f);
	audioDriver->playMusic(campaign->getSector()->ambientMusic, 1);
	audioDriver->setMusicGain(1, 1.f);
	return false;
}