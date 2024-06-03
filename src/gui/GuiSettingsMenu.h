#pragma once
#ifndef GUISETTINGSMENU_H
#define GUISETTINGSMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"
#include "VideoTab.h"
#include "KeybindTab.h"
#include "GameTab.h"
#include "Config.h"

/*
* The GuiSettingsMenu should be pretty self explanatory; what it does is keep track of the buttons for enabling / disabling
* video options as well as gameplay options and keybinds.
*/
class GuiSettingsMenu : public GuiDialog
{
	public:
		GuiSettingsMenu() : GuiDialog() {}
		virtual void init();

		bool onReturn(const SEvent& event);
		bool onVideo(const SEvent& event);
		bool onGame(const SEvent& event);
		bool onKeys(const SEvent& event);

	private:
		VideoTab videoTab;
		KeybindTab keyTab;
		GameTab gameTab;
		IGUIImage* topMenu;
		IGUIButton* back;
		IGUIButton* video;
		IGUIButton* game;
		IGUIButton* keys;

		SettingsTab* activeTab = nullptr;

		VideoConfig* vConfig;
		KeyConfig* kConfig;
};
#endif 