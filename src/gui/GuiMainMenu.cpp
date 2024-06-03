#include "GuiMainMenu.h"
#include "GuiController.h"
#include "GameStateController.h"
#include <iostream>

void GuiMainMenu::init()
{
	//Convenience declarations here to determine how big a button should be.
	//In this case, it's setting up some fairly large buttons straight down the middle of the screen.
	u32 verticalSlice = baseSize.Height / 6;
	u32 horizontalPos = baseSize.Width / 3;
	dimension2du buttonSize(horizontalPos, verticalSlice); //third of the screen size and a sixth of the height
	//auto bg = (IGUIImage*)root;
	//->setImage(driver->getTexture("assets/ui/loadingscreen.png"));
	if (root) root->remove();
	root = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), baseSize), false, false);
	scaleAlign(root);
	screen = guienv->addImage(rect<s32>(vector2di(0,0), dimension2du(400, 500)), root);
	screen->setImage(driver->getTexture("assets/ui/vertscreen.png"));
	scaleAlign(screen);

	buttonSize = dimension2du(300, 80);
	//All buttons have the root node set as the parent. This allows a single call to root->setVisible in order to display or hide the menu.
	singleplayer = guienv->addButton(rect<s32>(position2di(50, 4), buttonSize), screen, -1, L"Singleplayer", L"Start a new campaign or load one.");
	multiplayer = guienv->addButton(rect<s32>(position2di(50,4 + buttonSize.Height), buttonSize), screen, -1, L"Multiplayer", L"Start a new campaign or load one.");

	settings = guienv->addButton(rect<s32>(position2di(50, 4 + buttonSize.Height * 2), buttonSize), screen, -1, L"Settings", L"Fiddle with difficulty and video.");
	credits = guienv->addButton(rect<s32>(position2di(50, 4 + buttonSize.Height * 3), buttonSize), screen, -1, L"Credits", L"Appreciate the devs!");
	quitGame = guienv->addButton(rect<s32>(position2di(50, 4 + buttonSize.Height * 4), buttonSize), screen, -1, L"Quit Game", L"You'll be back.");

	setHoloButton(singleplayer);
	setHoloButton(multiplayer);
	//setButtonImg(multiplayer, "assets/ui/buttoncolors/greyed_out.png", "assets/ui/buttoncolors/greyed_out.png");
	setHoloButton(settings);
	setHoloButton(credits);
	setHoloButton(quitGame);

	guiController->setCallback(singleplayer, std::bind(&GuiMainMenu::onSingleplayer, this, std::placeholders::_1), GUI_MAIN_MENU);
#ifdef _DEBUG
	guiController->setCallback(multiplayer, std::bind(&GuiMainMenu::onMultiplayer, this, std::placeholders::_1), GUI_MAIN_MENU);
#else
	setButtonImg(multiplayer, "assets/ui/buttoncolors/greyed_out.png", "assets/ui/buttoncolors/greyed_out.png");
#endif
	guiController->setCallback(settings, std::bind(&GuiMainMenu::onSettings, this, std::placeholders::_1), GUI_MAIN_MENU);
	guiController->setCallback(quitGame, std::bind(&GuiMainMenu::onQuit, this, std::placeholders::_1), GUI_MAIN_MENU);
	guiController->setCallback(credits, std::bind(&GuiMainMenu::onCredits, this, std::placeholders::_1), GUI_MAIN_MENU);

	hide();
}

void GuiMainMenu::show()
{
	root->setRelativePosition(rect<s32>(position2di(0, 0), driver->getScreenSize()));
	root->setVisible(true);
	if (stateController->isInitialized()) stateController->toggleMenuBackdrop();
}

void GuiMainMenu::hide()
{
	GuiDialog::hide();
}

bool GuiMainMenu::onShow(f32 dt)
{
	rect<s32> pos = root->getAbsolutePosition();
	rect<s32> imgpos = screen->getRelativePosition();
	position2di curpos = imgpos.getCenter();
	curpos.X -= imgpos.getWidth() / 2;
	curpos.Y -= imgpos.getHeight() / 2;
	f32 vertprop = 500.f / 540.f;
	s32 dist = (s32)(pos.getHeight() * vertprop);
	return smoothGuiMove(screen, .25f, showAnimTimer, position2di(curpos.X, -dist), position2di(curpos.X, -5), dt);
}
bool GuiMainMenu::onHide(f32 dt)
{
	rect<s32> pos = root->getAbsolutePosition();
	rect<s32> imgpos = screen->getRelativePosition();
	position2di curpos = imgpos.getCenter();
	curpos.X -= imgpos.getWidth() / 2;
	curpos.Y -= imgpos.getHeight() / 2;
	f32 vertprop = 500.f / 540.f;
	s32 dist = (s32)(pos.getHeight() * vertprop);
	return smoothGuiMove(screen, .25f, showAnimTimer, position2di(curpos.X, -5), position2di(curpos.X, -dist), dt);
}

bool GuiMainMenu::onSingleplayer(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_OPTIONS_MENU);
	return false;
}

bool GuiMainMenu::onMultiplayer(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_MULTIPLAYER_MENU);
	return false;
}

bool GuiMainMenu::onSettings(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true; 
	guiController->setActiveDialog(GUI_SETTINGS_MENU);
	return false;
}
bool GuiMainMenu::onQuit(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	device->closeDevice();
	return false;
}
bool GuiMainMenu::onCredits(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_CREDITS_MENU);
	return true;
}

void GuiLoadingMenu::init()
{
	if (root) root->remove();
	auto img = guienv->addImage(rect<s32>(position2di(0, 0), baseSize));
	img->setImage(driver->getTexture("assets/ui/loadscreen.png"));
	img->setScaleImage(true);
	scaleAlign(img);
	root = img;

	logo = guienv->addImage(rect<s32>(position2di(520, 135), dimension2du(207, 65)), root);
	logo->setImage(driver->getTexture("assets/ui/baeds.png"));
	logo->setScaleImage(true);
	scaleAlign(logo);

	tip = guienv->addStaticText(L"", rect<s32>(position2di(520, 190), dimension2du(207, 80)), false, true, root);
	setUIText(tip);
	tip->setVisible(true);

}
void GuiLoadingMenu::show()
{
	setTip(guiController->getTip());
	GuiDialog::show();
}
void GuiLoadingMenu::setTip(std::wstring str)
{
	tip->setText(str.c_str());
}
bool GuiLoadingMenu::toggleLogo(bool toggle)
{
	logo->setVisible(toggle);
	return toggle;
}