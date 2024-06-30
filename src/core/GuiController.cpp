#include "GuiController.h"
#include "GameStateController.h"
#include "AudioDriver.h"

GuiController::GuiController()
{
	activeDialog = 0;

	gvReader tauntReader;
	tauntReader.read("assets/attributes/taunts.txt");
	for (std::string line : tauntReader.lines) {
		taunts.push_back(std::wstring(line.begin(), line.end()));
	}
	tauntReader.clear();
	tauntReader.read("assets/attributes/congrats.txt");
	for (std::string line : tauntReader.lines) {
		congrats.push_back(wstr(line));
	}
	tauntReader.clear();
	tauntReader.read("assets/attributes/tips.txt");
	for (std::string line : tauntReader.lines) {
		tips.push_back(wstr(line));
	}
	tauntReader.clear();
	tauntReader.read("assets/attributes/killnames.txt");
	for (std::string line : tauntReader.lines) {
		killnames.push_back(wstr(line));
	}
	tauntReader.clear();
	tauntReader.read("assets/attributes/injuries.txt");
	for (std::string line : tauntReader.lines) {
		injurynames.push_back(wstr(line));
	}
	dimension2du baseSize(960, 540);

	dimension2du titleSize(388, 28);
	dimension2du descSize(388, 80);
	dimension2du buttonSize(100, 18);

	position2di titlePos(286, 166);
	position2di descPos(286, 194);
	position2di okButtonPos(430, 274);
	position2di yesButtonPos(380, 274);
	position2di noButtonPos(480, 274);

	okPopup.bg = guienv->addImage(recti(position2di(0, 0), baseSize));
	okPopup.bg->setImage(driver->getTexture("assets/ui/popup.png"));
	okPopup.button = guienv->addButton(recti(okButtonPos, buttonSize), okPopup.bg, 2500, L"Got it");
	okPopup.title = guienv->addStaticText(L"Heads up", recti(titlePos, titleSize), false, true, okPopup.bg);
	okPopup.body = guienv->addStaticText(L"", recti(descPos, descSize), false, true, okPopup.bg);
	scaleAlign(okPopup.bg);
	setHoloButton(okPopup.button);
	setUIText(okPopup.title);
	setUIText(okPopup.body);

	keybindPopup.bg = guienv->addImage(recti(position2di(0, 0), baseSize));
	keybindPopup.bg->setImage(driver->getTexture("assets/ui/popup.png"));
	keybindPopup.title = guienv->addStaticText(L"Keybind", recti(titlePos, titleSize), false, true, keybindPopup.bg);
	keybindPopup.body = guienv->addStaticText(L"", recti(descPos, descSize), false, true, keybindPopup.bg);
	scaleAlign(keybindPopup.bg);
	setUIText(keybindPopup.title);
	setUIText(keybindPopup.body);

	yesNoPopup.bg = guienv->addImage(recti(position2di(0, 0), baseSize));
	yesNoPopup.bg->setImage(driver->getTexture("assets/ui/popup.png"));
	yesNoPopup.title = guienv->addStaticText(L"Heads up", recti(titlePos, titleSize), false, true, yesNoPopup.bg);
	yesNoPopup.body = guienv->addStaticText(L"", recti(descPos, descSize), false, true, yesNoPopup.bg);
	yesNoPopup.yes = guienv->addButton(recti(yesButtonPos, buttonSize), yesNoPopup.bg, 2501, L"Yes");
	yesNoPopup.no = guienv->addButton(recti(noButtonPos, buttonSize), yesNoPopup.bg, 2502, L"No");
	scaleAlign(yesNoPopup.bg);
	setHoloButton(yesNoPopup.yes);
	setHoloButton(yesNoPopup.no);
	setUIText(yesNoPopup.title);
	setUIText(yesNoPopup.body);

	okPopup.bg->setVisible(false);
	yesNoPopup.bg->setVisible(false);
	keybindPopup.bg->setVisible(false);
	setCallback(okPopup.button, std::bind(&GuiController::hidePopup, this, std::placeholders::_1), GUI_MENU_MAX);
	setCallback(yesNoPopup.no, std::bind(&GuiController::hidePopup, this, std::placeholders::_1), GUI_MENU_MAX);

	animatedRenderTex = driver->addRenderTargetTexture(dimension2du(512, 512), "RTT1");
}

void GuiController::init()
{
	//All menus get initialized here. Don't delete them.
	//If you've just added a new menu, initialize it here.
	menus[GUI_MAIN_MENU] = new GuiMainMenu;
	menus[GUI_MAIN_MENU]->init();
	menus[GUI_PAUSE_MENU] = new GuiPauseMenu;
	menus[GUI_PAUSE_MENU]->init();
	menus[GUI_DEATH_MENU] = new GuiDeathMenu;
	menus[GUI_DEATH_MENU]->init();
	menus[GUI_SETTINGS_MENU] = new GuiSettingsMenu;
	menus[GUI_SETTINGS_MENU]->init();
	menus[GUI_CAMPAIGN_MENU] = new GuiCampaignMenu;
	menus[GUI_CAMPAIGN_MENU]->init();
	menus[GUI_LOOT_MENU] = new GuiLootMenu;
	menus[GUI_LOOT_MENU]->init();
	menus[GUI_DIALOGUE_MENU] = new GuiDialogueMenu;
	menus[GUI_DIALOGUE_MENU]->init();
	menus[GUI_LOADOUT_MENU] = new GuiLoadoutMenu;
	menus[GUI_LOADOUT_MENU]->init();
	menus[GUI_FAB_MENU] = new GuiFabMenu;
	menus[GUI_FAB_MENU]->init();
	menus[GUI_MESS_HALL_MENU] = new GuiMessHallMenu;
	menus[GUI_MESS_HALL_MENU]->init();
	menus[GUI_LOADING_MENU] = new GuiLoadingMenu;
	menus[GUI_LOADING_MENU]->init();
	menus[GUI_LAUNCH_MENU] = new GuiLaunchMenu;
	menus[GUI_LAUNCH_MENU]->init();
	menus[GUI_OPTIONS_MENU] = new GuiOptionsMenu;
	menus[GUI_OPTIONS_MENU]->init();
	menus[GUI_CREDITS_MENU] = new GuiCreditsMenu;
	menus[GUI_CREDITS_MENU]->init();
	menus[GUI_MULTIPLAYER_MENU] = new GuiMultiplayerMenu;
	menus[GUI_MULTIPLAYER_MENU]->init();

	activeDialog = menus[GUI_LOADING_MENU];
	activeDialog->show();
	//callOpenAnimation(activeDialog);
	guienv->getRootGUIElement()->bringToFront(activeDialog->getRoot());
	//setActiveDialog(GUI_LOADING_MENU);
	//default main menu
}

void GuiController::setOkPopup(std::string title, std::string body, std::string button)
{
	okPopup.title->setText(wstr(title).c_str());
	okPopup.body->setText(wstr(body).c_str());
	okPopup.button->setText(wstr(button).c_str());
}

void GuiController::setYesNoPopup(std::string title, std::string body, GuiCallback yesCb, std::string yes, std::string no, GuiCallback noCb)
{
	yesNoPopup.title->setText(wstr(title).c_str());
	yesNoPopup.body->setText(wstr(body).c_str());
	yesNoPopup.yes->setText(wstr(yes).c_str());
	yesNoPopup.no->setText(wstr(no).c_str());

	removeCallback(yesNoPopup.yes);
	removeCallback(yesNoPopup.no);
	setCallback(yesNoPopup.yes, yesCb, GUI_MENU_MAX);
	yesNoPopup.hasNoCb = false;
	if (noCb) {
		setCallback(yesNoPopup.no, noCb, GUI_MENU_MAX);
		yesNoPopup.hasNoCb = true;
	}
	else {
		setCallback(yesNoPopup.no, std::bind(&GuiController::hidePopup, this, std::placeholders::_1), GUI_MENU_MAX);
	}
}

void GuiController::setKeybindPopup(std::string title, GuiCallback bindCb, std::string body)
{
	keybindPopup.title->setText(wstr(title).c_str());
	keybindPopup.body->setText(wstr(body).c_str());

	removeCallback(keybindPopup.bg);
	setCallback(keybindPopup.bg, bindCb, GUI_MENU_MAX);
}
void GuiController::showKeybindPopup()
{
	audioDriver->playMenuSound("menu_error.ogg");
	dimension2du screenSize = driver->getScreenSize();
	keybindPopup.bg->setRelativePosition(recti(position2di(0, 0), screenSize));
	keybindPopup.bg->setVisible(true);
	guienv->getRootGUIElement()->bringToFront(keybindPopup.bg);
	popupActive = true;
}

void GuiController::showOkPopup()
{
	audioDriver->playMenuSound("menu_error.ogg");
	dimension2du screenSize = driver->getScreenSize();
	okPopup.bg->setRelativePosition(recti(position2di(0, 0), screenSize));
	okPopup.bg->setVisible(true);
	guienv->getRootGUIElement()->bringToFront(okPopup.bg);
	popupActive = true;
}

void GuiController::showYesNoPopup()
{
	audioDriver->playMenuSound("menu_error.ogg");
	dimension2du screenSize = driver->getScreenSize();
	yesNoPopup.bg->setRelativePosition(recti(position2di(0,0), screenSize));
	yesNoPopup.bg->setVisible(true);
	guienv->getRootGUIElement()->bringToFront(yesNoPopup.bg);
	popupActive = true;
}

bool GuiController::hidePopup(const SEvent& event)
{
	if ((event.EventType == EET_KEY_INPUT_EVENT || event.EventType == EET_MOUSE_INPUT_EVENT) && popupActive && keybindPopup.bg->isVisible()) {
		bool valid = true;
		if (event.EventType == EET_MOUSE_INPUT_EVENT) {
			if (event.EventType != EMIE_LMOUSE_PRESSED_DOWN && event.EventType != EMIE_RMOUSE_PRESSED_DOWN && event.EventType != EMIE_MMOUSE_PRESSED_DOWN) valid = false;
		}
		if (valid) {
			audioDriver->playMenuSound("menu_proceed.ogg");
			keybindPopup.bg->setVisible(false);
			popupActive = false;
			return false;
		}
	}
	if ((event.GUIEvent.EventType != EGET_BUTTON_CLICKED) || 
		(event.GUIEvent.Caller != okPopup.button) && (event.GUIEvent.Caller != yesNoPopup.yes) && (event.GUIEvent.Caller != yesNoPopup.no)
		&& (event.GUIEvent.Caller != keybindPopup.bg)) return true;

	if (event.GUIEvent.Caller == yesNoPopup.yes) audioDriver->playMenuSound("menu_proceed.ogg");
	else audioDriver->playMenuSound("menu_cancel.ogg");
	//std::cout << "hiding popup\n";
	okPopup.bg->setVisible(false);
	yesNoPopup.bg->setVisible(false);
	keybindPopup.bg->setVisible(false);
	popupActive = false;
	return false;
}

void GuiController::setAnimationCallback(IGUIElement* elem, AnimationCallback cb)
{
	animationCallbacks[elem] = cb;
}
void GuiController::setOpenAnimationCallback(GuiDialog* dialog, AnimationCallback cb)
{
	openAnimationCallbacks[dialog] = cb;
}
void GuiController::setCloseAnimationCallback(GuiDialog* dialog, AnimationCallback cb)
{
	closeAnimationCallbacks[dialog] = cb;
}

void GuiController::callCloseAnimation(GuiDialog* dialog)
{
	if (closeAnimationCallbacks.find(dialog) != closeAnimationCallbacks.end()) {
		currentAnimation = closeAnimationCallbacks[dialog];
		playingAnimation = true;
	}
}
void GuiController::callOpenAnimation(GuiDialog* dialog)
{
	if (openAnimationCallbacks.find(dialog) != openAnimationCallbacks.end()) {
		currentAnimation = openAnimationCallbacks[dialog];
		playingAnimation = true;
	}
}
void GuiController::callAnimation(IGUIElement* elem)
{
	if (animationCallbacks.find(elem) != animationCallbacks.end() && !playingAnimation) {
		currentAnimation = animationCallbacks[elem];
		playingAnimation = true;
	}
}

void GuiController::setDialogueTree(DialogueTree tree)
{
	auto menu = (GuiDialogueMenu*)menus[GUI_DIALOGUE_MENU];
	menu->setDialogue(tree);
	menu->setPreviousDialog(currentDialog);
}

std::wstring GuiController::getTaunt()
{
	return taunts[random.unumEx((unsigned int)taunts.size())]; //Pulls out a random taunt to mess with the player
}

std::wstring GuiController::getCongrats()
{
	return congrats[random.unumEx((unsigned int)congrats.size())];
}

std::wstring GuiController::getTip()
{
	return tips[random.unumEx((unsigned int)tips.size())];
}
std::wstring GuiController::getKillName()
{
	return killnames[random.unumEx((unsigned int)killnames.size())];
}
std::wstring GuiController::getInjuryName()
{
	return injurynames[random.unumEx((unsigned int)injurynames.size())];
}

void GuiController::close()
{
	if (activeDialog) {
		activeDialog->hide();
		activeDialog = nullptr;
	} //Doesn't actually delete anything; that's what menuCleanup is for (and guienv->clear() is called elsewhere to actually remove elements)
}

bool GuiController::OnEvent(const SEvent& event)
{
	//Hurls the event to the active dialog.
	if ((event.EventType == EET_KEY_INPUT_EVENT || event.EventType == EET_MOUSE_INPUT_EVENT) && popupActive && keybindPopup.bg->isVisible()) {
		if(!callbacks[keybindPopup.bg].cb(event)) return hidePopup(event);
	}

	if (activeDialog && event.EventType == EET_GUI_EVENT) {
		if (callbacks.find(event.GUIEvent.Caller) != callbacks.end()) {

			if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED) 
				audioDriver->playMenuSound("menu_click.ogg");

			if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED) {
				audioDriver->playMenuSound("menu_hover.ogg");
			}
			//todo: add expand / contract on hover / leave
			/*
			if (event.GUIEvent.EventType == EGET_ELEMENT_LEFT) {

			}
			*/
			auto& call = callbacks[event.GUIEvent.Caller];
			if (!popupActive) return callbacks.at(event.GUIEvent.Caller).cb(event);
			else {
				if (event.GUIEvent.Caller == yesNoPopup.yes) {
					hidePopup(event);
					return callbacks[yesNoPopup.yes].cb(event);
				}
				if (event.GUIEvent.Caller == yesNoPopup.no) {
					hidePopup(event);
					if (yesNoPopup.hasNoCb)
						return callbacks[yesNoPopup.no].cb(event);
					return false;
				}
				return hidePopup(event);
			}
		}
	}

	if (activeDialog && event.EventType == EET_KEY_INPUT_EVENT) {
		auto& list = callbacksOnMenu[currentDialogType()];
		bool notHandled = true;
		for (auto elem : list) {
			if (callbacks.find(elem) != callbacks.end()) {
				if (callbacks.at(elem).flag(GUICONTROL_KEY)) {
					callbacks.at(elem).cb(event);
					notHandled = false;
				}
			}
		}
		return notHandled;
	}
	if (activeDialog && event.EventType == EET_MOUSE_INPUT_EVENT) {
		auto& list = callbacksOnMenu[currentDialogType()];
		bool notHandled = true;
		for (auto elem : list) {
			if (callbacks.find(elem) != callbacks.end()) {
				if (callbacks.at(elem).flag(GUICONTROL_MOUSE)) {
					callbacks.at(elem).cb(event);
					notHandled = false;
				}
			}
		}
		return notHandled;

	}
	if (activeDialog && event.EventType == EET_JOYSTICK_INPUT_EVENT) {
		auto& list = callbacksOnMenu[currentDialogType()];
		bool notHandled = true;
		for (auto elem : list) {
			if (callbacks.find(elem) != callbacks.end()) {
				if (callbacks.at(elem).flag(GUICONTROL_JOYSTICK)) {
					callbacks.at(elem).cb(event);
					notHandled = false;
				}
			}
		}
		return notHandled;

	}

	return true;
}

void GuiController::setCallback(IGUIElement* elem, GuiCallback callback, MENU_TYPE which, const u32 acceptedEvents)
{
	callbacks[elem] = { acceptedEvents, callback, which };
	if (which != GUI_MENU_MAX) callbacksOnMenu[which].push_back(elem);
}

void GuiController::removeCallback(IGUIElement* elem)
{
	if (callbacks.find(elem) == callbacks.end()) return;

	auto& cb = callbacks.at(elem);
	if (cb.which != GUI_MENU_MAX) { // dont do this for popups
		auto it = callbacksOnMenu[cb.which].begin();
		while (it != callbacksOnMenu[cb.which].end()) {
			if (*it == elem) it = callbacksOnMenu[cb.which].erase(it);
			if(it != callbacksOnMenu[cb.which].end()) ++it;
		}
	}
	callbacks.erase(elem);
}

void GuiController::update()
{
	u32 now = device->getTimer()->getRealTime();
	f32 delta = (f32)(now - then) / 1000.f;
	if (delta > .25) { //If the delta is too big, it's .25.
		delta = .25;
	}
	then = now;
	accumulator += delta;
	while (accumulator >= dt) {
		if (playingAnimation) {
			playingAnimation = currentAnimation(dt);
		}

		if (!playingAnimation && switchMenusCalled) {
			if (activeDialog && menuToSwitch != GUI_DIALOGUE_MENU) activeDialog->hide();
			
			activeDialog = menus[menuToSwitch];
			activeDialog->show();
			callOpenAnimation(activeDialog);
			guienv->getRootGUIElement()->bringToFront(activeDialog->getRoot());
			switchMenusCalled = false;

			if (eventDialoguePopup) {
				eventDialoguePopup = false;
				setActiveDialog(GUI_DIALOGUE_MENU);
			}
			if (loadoutWarningTrigger) {
				loadoutWarningTrigger = false;
				guiController->showOkPopup();
			}
		}

		t += dt;
		accumulator -= dt;
	}
}

//If you've just added a new menu, go make sure that you added it as a menu type in MenuData.h
void GuiController::setActiveDialog(MENU_TYPE menu)
{
	if (currentDialog == menu && currentDialog != GUI_PAUSE_MENU) {
		audioDriver->playMenuSound("menu_error.ogg");
		return;
	}
	if(activeDialog) callCloseAnimation(activeDialog);
	menuToSwitch = menu;
	previousDialog = currentDialog;
	currentDialog = menu;
	switchMenusCalled = true;
}