#pragma once
#ifndef GUIOPTIONSMENU_H
#define GUIOPTIONSMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"
#include "SaveGameTab.h"
#include "LoadGameTab.h"

class GuiOptionsMenu : public GuiDialog
{
	public:
		virtual void init();
		virtual void show();
		bool onSave(const SEvent& event);
		bool onLoad(const SEvent& event);
		bool onNew(const SEvent& event);
		bool onNewConfirm(const SEvent& event);
		bool onTutorialConfirm(const SEvent& event);
		bool onSettings(const SEvent& event);
		bool onExit(const SEvent& event);
		bool onExitConfirm(const SEvent& event);
	private:
		bool continueNoSave = false;
		IGUIButton* save;
		IGUIButton* load;
		IGUIButton* newGame;
		IGUIButton* settings;
		IGUIButton* backToMain;
		SaveGameTab saveTab;
		LoadGameTab loadTab;
		NavMenu nav;
};
#endif 