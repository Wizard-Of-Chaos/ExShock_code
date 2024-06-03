#include "SaveGameTab.h"
#include "GuiController.h"
#include "GameStateController.h"
#include <iostream>

void SaveGameTab::build(IGUIElement* root)
{
	m_baseBuild(root);
	newSave = guienv->addButton(rect<s32>(position2di(10, 10), dimension2du(100, 25)), base, -1, L"New Save", L"Create a new save file.");
	setThinHoloButton(newSave, BCOL_GREEN);
	guiController->setCallback(newSave, std::bind(&SaveGameTab::onNewSave, this, std::placeholders::_1), GUI_OPTIONS_MENU);
	guiController->setCallback(confirm, std::bind(&SaveGameTab::onSave, this, std::placeholders::_1), GUI_OPTIONS_MENU);
	hide();
}
void SaveGameTab::show()
{
	OptionsTab::show();
	m_clearList();
	m_loadSaves();
	confirm->setVisible(false);
	remove->setVisible(false);
	m_currentSelection = -1;
	m_displaySave();
}
bool SaveGameTab::onSave(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (stateController->inCampaign) {
		guiController->setYesNoPopup("Save Over File", "Are you sure you want to overwrite this save? Your previous save data will be lost.",
			std::bind(&SaveGameTab::saveConfirm, this, std::placeholders::_1));
		guiController->showYesNoPopup();
		return false;
	}
	return saveConfirm(event);
}

bool SaveGameTab::onNewSave(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	std::string newsave = "saves/" + std::to_string(device->getTimer()->getRealTime()) + ".xml";
	campaign->saveCampaign(newsave);
	m_loadSaves();
	m_currentSelection = -1;
	m_displaySave();
	return false;
}

bool SaveGameTab::saveConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	campaign->saveCampaign(saves[m_currentSelection]->getName());
	m_loadSaves();
	m_currentSelection = -1;
	m_displaySave();
	return false;
}