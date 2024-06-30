#pragma once

#ifndef GUICONTROLLER_H
#define GUICONTROLLER_H
#include "BaseHeader.h"
#include "MenuData.h"

typedef std::function<bool(const SEvent&)> GuiCallback;
typedef std::function<bool(f32)> AnimationCallback; //should return FALSE when done and TRUE when not done

struct OkPopup
{
	IGUIImage* bg;
	IGUIButton* button;
	IGUIStaticText* title;
	IGUIStaticText* body;
};

struct YesNoPopup
{
	IGUIImage* bg;
	IGUIButton* yes;
	IGUIButton* no;
	IGUIStaticText* title;
	IGUIStaticText* body;
	bool hasNoCb = false;
};

struct KeybindPopup
{
	IGUIImage* bg;
	IGUIStaticText* title;
	IGUIStaticText* body;
};
/*
* The GUI controller handles updates for all the menus in the game (start menu, settings menu, death menu, etc).
* It takes in events with the OnEvent call, and holds pointers to any necessary information.
* 
* The big thing it does is contains the active dialogs and a list of dialogs that it can run. For example, if the
* player is currently on the main menu, the main menu will be  the active dialog. Changing menus will call setActiveDialog()
* and other functions. It also has a list of taunts for the death screen (loaded from a text file). The event taken in
* OnEvent gets passed down the chain to the current active dialog.
*/

enum GUICONTROLLER_EVENT_FLAG
{
	GUICONTROL_GUI = 1,
	GUICONTROL_MOUSE = 2,
	GUICONTROL_KEY = 4,
	GUICONTROL_JOYSTICK = 8
};

class GuiController
{
	public:
		GuiController();
		bool OnEvent(const SEvent& event);
		void init();
		void close();
		void update();
		GuiDialog* getActiveDialog() { return activeDialog; }
		MENU_TYPE currentDialogType() { return currentDialog; }
		MENU_TYPE getPreviousDialog() { return previousDialog; }
		GuiDialog* getDialogueByType(MENU_TYPE type) { if (menus.find(type) != menus.end()) return menus.at(type); return nullptr; }
		GuiMultiplayerMenu* mpMenu() { return (GuiMultiplayerMenu*)menus.at(GUI_MULTIPLAYER_MENU); }
		void setActiveDialog(MENU_TYPE menu);

		std::wstring getTaunt();
		std::wstring getCongrats();
		std::wstring getTip();
		std::wstring getKillName();
		std::wstring getInjuryName();

		void setOkPopup(std::string title, std::string body, std::string button = "Got it");
		void showOkPopup();
		void setYesNoPopup(std::string title, std::string body, GuiCallback yesCb, std::string yes = "Yes", std::string no = "No", GuiCallback noCb=GuiCallback());
		void showYesNoPopup();
		void setKeybindPopup(std::string title, GuiCallback bindCb, std::string body = "Press the key that you want bound to this control.");
		void showKeybindPopup();
		bool hidePopup(const SEvent& event);

		void setCallback(IGUIElement* elem, GuiCallback callback, MENU_TYPE which, const u32 acceptedEvents = GUICONTROL_GUI);
		void removeCallback(IGUIElement* elem);

		void setAnimationCallback(IGUIElement* elem, AnimationCallback cb);
		void setOpenAnimationCallback(GuiDialog* dialog, AnimationCallback cb);
		void setCloseAnimationCallback(GuiDialog* dialog, AnimationCallback cb);
		void callCloseAnimation(GuiDialog* dialog);
		void callOpenAnimation(GuiDialog* dialog);
		void callAnimation(IGUIElement* elem);

		void setDialogueTree(DialogueTree tree);
		void setEventDialoguePopup() { eventDialoguePopup = true; }
		void setLoadoutTrigger() { loadoutWarningTrigger = true; }
		bool initPopupUsed = true;

		const bool renderTexNeeded() { if (currentDialog == GUI_FAB_MENU) return true; return false; }
		ITexture* getRenderTex() { return animatedRenderTex; }
	private:
		u32 then;
		f32 accumulator = 0.0f;
		f32 dt = 0.005f;
		f32 t = 0.0f;

		bool playingAnimation = false;
		bool switchMenusCalled = false;
		bool eventDialoguePopup = false;
		bool loadoutWarningTrigger = false;
		MENU_TYPE menuToSwitch;
		AnimationCallback currentAnimation;

		ITexture* animatedRenderTex = nullptr;

		bool popupActive = false;
		OkPopup okPopup;
		YesNoPopup yesNoPopup;
		KeybindPopup keybindPopup;

		GuiDialog* activeDialog = nullptr;
		MENU_TYPE currentDialog;
		MENU_TYPE previousDialog;

		std::map<MENU_TYPE, GuiDialog*> menus;
		std::vector<std::wstring> taunts;
		std::vector<std::wstring> congrats;
		std::vector<std::wstring> tips;
		std::vector<std::wstring> killnames;
		std::vector<std::wstring> injurynames;
		struct _guicb {
			u32 flags = GUICONTROL_GUI;
			GuiCallback cb;
			MENU_TYPE which;
			const bool flag(GUICONTROLLER_EVENT_FLAG flag) const { return !!(flags &flag); }
		};
		std::unordered_map<IGUIElement*, _guicb> callbacks;
		std::list<IGUIElement*> callbacksOnMenu[GUI_MENU_MAX];
		std::unordered_map<IGUIElement*, AnimationCallback> animationCallbacks;
		std::unordered_map<GuiDialog*, AnimationCallback> closeAnimationCallbacks;
		std::unordered_map<GuiDialog*, AnimationCallback> openAnimationCallbacks;
};

#endif