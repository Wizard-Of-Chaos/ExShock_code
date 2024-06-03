#include "GuiDialog.h"
#include "GuiController.h"
#include "GameStateController.h"

void NavMenu::build(IGUIElement* root, MENU_TYPE which)
{
	navRoot = root;
	bottomUI = guienv->addImage(rect<s32>(position2di(120, 466), dimension2du(720, 74)), root);
	bottomUI->setImage(driver->getTexture("assets/ui/bottomUI.png"));
	scaleAlign(bottomUI);
	dimension2du buttonSize(94, 40);

	exit = guienv->addButton(rect<s32>(position2di(65, 25), buttonSize), bottomUI, -1, L"Options", L"View campaign options.");
	setHoloButton(exit);
	mess = guienv->addButton(rect<s32>(position2di((buttonSize.Width + 5) + 65, 25), buttonSize), bottomUI, -1, L"Mess Hall", L"Visit crew members and chat.");
	setHoloButton(mess);
	fabbay = guienv->addButton(rect<s32>(position2di((buttonSize.Width + 5) * 2 + 65, 25), buttonSize), bottomUI, -1, L"Fab Bay", L"Build and repair your armory.");
	setHoloButton(fabbay);
	loadout = guienv->addButton(rect<s32>(position2di((buttonSize.Width + 5) * 3 + 65, 25), buttonSize), bottomUI, -1, L"Loadout", L"Set your ship and loadout.");
	setHoloButton(loadout);
	command = guienv->addButton(rect<s32>(position2di((buttonSize.Width + 5) * 4 + 65, 25), buttonSize), bottomUI, -1, L"Command", L"Select the current scenario.");
	setHoloButton(command);
	launch = guienv->addButton(rect<s32>(position2di((buttonSize.Width + 5) * 5 + 65, 25), buttonSize), bottomUI, -1, L"Launch", L"Launch the current scenario.");
	setHoloButton(launch);

	guiController->setCallback(exit, std::bind(&NavMenu::onExit, this, std::placeholders::_1), which);
	guiController->setCallback(loadout, std::bind(&NavMenu::onLoadout, this, std::placeholders::_1), which);
	guiController->setCallback(fabbay, std::bind(&NavMenu::onFab, this, std::placeholders::_1), which);
	guiController->setCallback(command, std::bind(&NavMenu::onCommand, this, std::placeholders::_1), which);
	guiController->setCallback(launch, std::bind(&NavMenu::onLaunch, this, std::placeholders::_1), which);
	guiController->setCallback(mess, std::bind(&NavMenu::onMess, this, std::placeholders::_1), which);
}
void NavMenu::show()
{
	bottomUI->setVisible(true);
	navRoot->bringToFront(bottomUI);
}
void NavMenu::hide()
{
	bottomUI->setVisible(false);
}

bool NavMenu::onLoadout(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_LOADOUT_MENU);
	return false;
}
bool NavMenu::onExit(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_OPTIONS_MENU);
	return false;
}
bool NavMenu::onCommand(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_CAMPAIGN_MENU);
	return false;
}

bool NavMenu::onFab(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_FAB_MENU);
	return false;
}

bool NavMenu::onMess(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_MESS_HALL_MENU);
	return false;
}
bool NavMenu::onLaunch(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_LAUNCH_MENU);
	return false;
}

GuiDialog::GuiDialog() 
{ 
	baseSize = dimension2du(960, 540);
	root = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	IGUIImage* img = (IGUIImage*)root;
	img->setImage(driver->getTexture("assets/ui/starbg.png"));
	img->setScaleImage(true);
	scaleAlign(img);

}

void GuiDialog::show() 
{
	root->setRelativePosition(rect<s32>(position2di(0, 0), driver->getScreenSize()));
	root->setVisible(true);
};
void GuiDialog::hide() 
{
	root->setRelativePosition(rect<s32>(position2di(0, 0), driver->getScreenSize()));
	root->setVisible(false); 
};