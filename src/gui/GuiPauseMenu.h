#pragma once

#ifndef GUIPAUSEMENU_H
#define GUIPAUSEMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"

//Enum containing the IDs for buttons on the pause menu.
enum PAUSE_MENU_BUTTONS
{
	PAUSEMENU_RESUME,
	PAUSEMENU_SETTINGS,
	PAUSEMENU_EXIT
};

/*
* The pause menu for the game. Includes the buttons on the pause menu and logic to handle those buttons.
*/
class GuiPauseMenu : public GuiDialog
{
	public:
		GuiPauseMenu() : GuiDialog(), resumeGame(0), pauseSettings(0), exitToMenus(0) {}
		~GuiPauseMenu() {}

		virtual void init();

		bool onResume(const SEvent& event);
		bool onSettings(const SEvent& event);
		bool onExit(const SEvent& exit);
	private:
		IGUIButton* resumeGame;
		IGUIButton* pauseSettings;
		IGUIButton* exitToMenus;

};

#endif