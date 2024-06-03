#include "GuiSettingsMenu.h"
#include "GuiController.h"
#include "GameStateController.h"
#include <iostream>

void GuiSettingsMenu::init()
{
	vConfig = &cfg->vid;
	kConfig = &cfg->keys;

	if (root) root->remove();
	auto bg = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	bg->setImage(driver->getTexture("assets/ui/settingsScreen.png"));
	scaleAlign(bg);
	root = bg;

	topMenu = guienv->addImage(rect<s32>(position2di(330, 480), dimension2du(300, 60)), root, -1);
	topMenu->setImage(driver->getTexture("assets/ui/bottomUI.png"));
	scaleAlign(topMenu);
	back = guienv->addButton(rect<s32>(position2di(75, 15), dimension2du(150, 35)), topMenu, -1, L"Back", L"Restart the game to apply settings.");
	setHoloButton(back);

	position2di basePos(262, 60);
	dimension2du size(140, 20);
	s32 buf = size.Width + 5;
	video = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Video Settings", L"Set video options for visual quality.");
	setHoloButton(video);
	basePos.X += buf;
	game = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Game Settings", L"Set in-game settings such as difficulty.");
	setHoloButton(game);
	basePos.X += buf;
	keys = guienv->addButton(rect<s32>(basePos, size), root, -1, L"Keybinds", L"Adjust your key.");
	setHoloButton(keys);
	basePos.X += buf;

	guiController->setCallback(back, std::bind(&GuiSettingsMenu::onReturn, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(video, std::bind(&GuiSettingsMenu::onVideo, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(game, std::bind(&GuiSettingsMenu::onGame, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	guiController->setCallback(keys, std::bind(&GuiSettingsMenu::onKeys, this, std::placeholders::_1), GUI_SETTINGS_MENU);

	videoTab.build(root, vConfig);
	videoTab.hide();
	keyTab.build(root, kConfig);
	keyTab.hide();
	gameTab.build(root);
	gameTab.hide();
	hide();
}
bool GuiSettingsMenu::onReturn(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	vConfig->saveConfig("assets/cfg/videoconfig.gdat");
	cfg->game.saveConfig("assets/cfg/gameconfig.gdat");
	cfg->keys.saveConfig("assets/cfg/keyconfig.gdat");

	guiController->setActiveDialog(guiController->getPreviousDialog());
	return false;
}

bool GuiSettingsMenu::onVideo(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (activeTab) activeTab->hide();
	activeTab = &videoTab;
	activeTab->show();
	return false;
}
bool GuiSettingsMenu::onGame(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (activeTab) activeTab->hide();
	activeTab = &gameTab;
	activeTab->show();
	return false;
}
bool GuiSettingsMenu::onKeys(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	if (activeTab) activeTab->hide();
	activeTab = &keyTab;
	activeTab->show();
	return false;
}