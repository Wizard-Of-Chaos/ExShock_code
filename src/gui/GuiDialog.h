#pragma once

#ifndef GUIDIALOG_H
#define GUIDIALOG_H
#include "BaseHeader.h"
#include "IrrlichtUtils.h"
//Enum for the different types of dialog. Add to this enum when implementing a new GuiDialog.
//This allows the GuiController to be able to set the active dialog accordingly.
enum MENU_TYPE {
	GUI_MAIN_MENU = 1,
	GUI_PAUSE_MENU = 2,
	GUI_DEATH_MENU = 3,
	GUI_SETTINGS_MENU = 4,
	GUI_CAMPAIGN_MENU = 5,
	GUI_LOADOUT_MENU = 6,
	GUI_LOOT_MENU = 7,
	GUI_DIALOGUE_MENU = 8,
	GUI_MESS_HALL_MENU = 9,
	GUI_LAUNCH_MENU = 10,
	GUI_FAB_MENU = 11,
	GUI_LOADING_MENU = 12,
	GUI_OPTIONS_MENU = 13,
	GUI_CREDITS_MENU = 14,
	GUI_MULTIPLAYER_MENU = 15,
	GUI_MENU_MAX
};

class NavMenu
{
	public:
		void show();
		void hide();
		void build(IGUIElement* root, MENU_TYPE which);
		bool onLoadout(const SEvent& event);
		bool onExit(const SEvent& event);
		bool onCommand(const SEvent& event);
		bool onFab(const SEvent& event);
		bool onMess(const SEvent& event);
		bool onLaunch(const SEvent& event);
	private:
		IGUIImage* bottomUI;
		IGUIButton* loadout;
		IGUIButton* exit;
		IGUIButton* command;
		IGUIButton* fabbay;
		IGUIButton* mess;
		IGUIButton* launch;

		IGUIElement* navRoot;
};

/*
* The base class for all GUI dialogs. Includes a show/hide function as well as a root GUI element and a pointer to the controller.
* All buttons and text and whatnot NEEDS to have the root GUI element as a parent, so the dialog can be shown or hidden.
*/
class GuiDialog
{
	public:

		GuiDialog();

		//This function MUST be implemented. How that gets done can vary.
		//For an example, go check GuiMainMenu.h
		virtual void init() = 0;

		//Shows the root node (and therefore shows all children of the root node).
		virtual void show();
		//Hides the root node (and therefore hides all children of the root node).
		virtual void hide();
		//Checks whether or not the dialog is visible by checking the root node's visibility.
		bool isDialogVisible() { if (root) { return root->isVisible(); } return false; }
		//Gets the root element for this menu.
		IGUIElement* getRoot() { return root; }
	protected:
		//The base size will be set to 960x540. GUI design can assume that you're working for a screen of that size.
		//The elements will automatically scale with the size of the UI - assuming you set that up properly.
		dimension2du baseSize;
		//The root node is an effectively empty node that show and hide gets called on. All following GUI elements
		//need to be set as a child of the root node.
		IGUIElement* root;
};

#endif

