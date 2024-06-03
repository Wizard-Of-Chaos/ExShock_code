#pragma once

#ifndef GUIMAINMENU_H
#define GUIMAINMENU_H

#include "BaseHeader.h"
#include "GuiDialog.h"

/*
* The main menu dialog. Includes the buttons and logic to handle those buttons and start the game.
*/
class GuiMainMenu : public GuiDialog
{
	public:
		GuiMainMenu() : GuiDialog(), singleplayer(0), multiplayer(0), settings(0), quitGame(0) {}

		virtual void init();
		virtual void show();
		virtual void hide();

		bool onSingleplayer(const SEvent& event);
		bool onMultiplayer(const SEvent& event);

		bool onSettings(const SEvent& event);
		bool onCredits(const SEvent& event);
		bool onQuit(const SEvent& event);
		bool onShow(f32 dt);
		bool onHide(f32 dt);
	private:
		f32 showAnimTimer;
		IGUIImage* screen;
		IGUIButton* singleplayer;
		IGUIButton* multiplayer;
		IGUIButton* settings;
		IGUIButton* credits;
		IGUIButton* quitGame;
};

class GuiLoadingMenu : public GuiDialog
{
	public:
		GuiLoadingMenu() : GuiDialog() {}
		virtual void init();
		virtual void show();
		void setTip(std::wstring str);
		bool toggleLogo(bool toggle);
	private:
		IGUIImage* logo;
		IGUIStaticText* tip;

};
#endif
