#pragma once
#include "LoadGameTab.h"
#include "GuiController.h"
#include "AudioDriver.h"
#include "GameStateController.h"
void LoadGameTab::build(IGUIElement* root)
{
	m_baseBuild(root);
	guiController->setCallback(confirm, std::bind(&LoadGameTab::onLoad, this, std::placeholders::_1), GUI_OPTIONS_MENU);
	hide();
}
void LoadGameTab::show()
{
	OptionsTab::show();
	m_clearList();
	m_loadSaves();
	confirm->setVisible(false);
	remove->setVisible(false);
	m_currentSelection = -1;
	m_displaySave();
}

bool LoadGameTab::onLoad(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (stateController->inCampaign) {
		guiController->setYesNoPopup("Load Game", "Are you sure your want to load a different game?\n\nAny progress made since the last save will not be saved.",
			std::bind(&LoadGameTab::onLoadConfirm, this, std::placeholders::_1));
		guiController->showYesNoPopup();
		return false;
	}
	return onLoadConfirm(event);
}

bool LoadGameTab::onLoadConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;

	if (campaign->loadCampaign(saves[m_currentSelection]->getName())) {
		stateController->inCampaign = true;
		stateController->toggleMenuBackdrop(false);
		guiController->setActiveDialog(GUI_CAMPAIGN_MENU);
		audioDriver->playMusic(campaign->getSector()->menuMusic);
	}
	else {
		guiController->setOkPopup("Error", "Something went horribly wrong while loading this save.");
		guiController->showOkPopup();
		m_loadSaves();
	}

	m_currentSelection = -1;
	m_displaySave();
	hide();
	return false;
}