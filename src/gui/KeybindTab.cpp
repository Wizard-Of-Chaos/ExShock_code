#include "KeybindTab.h"
#include "GuiController.h"
#include "Config.h"
#include "AudioDriver.h"

void KeybindTab::build(IGUIElement* root, KeyConfig* confg)
{
	m_baseBuild(root);
	keyCfg = confg;
	if (explain) explain->remove();
	dimension2du itemSize(227, 16);
	position2di leftItemStart(10, 45);
	position2di rightItemStart(243, 45);
	u32 buf = itemSize.Height + 1;
	for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
		std::wstring name = inputNames.at((INPUT)i) + L": " + keyDesc.at(keyCfg->key[(INPUT)i]);
		position2di* pos = &leftItemStart;
		if (i % 2 == 0) pos = &rightItemStart;
		keys[i] = guienv->addButton(rect<s32>(*pos, itemSize), base, i, name.c_str(), L"Toggle this key.");
		setThinHoloButton(keys[i], BCOL_BLUE);
		pos->Y += buf;
		guiController->setCallback(keys[i], std::bind(&KeybindTab::onSetKeybind, this, std::placeholders::_1), GUI_SETTINGS_MENU);
	}
	restart->setText(L"Keybinds assume a US keyboard layout.");
}

void KeybindTab::show()
{
	SettingsTab::show();
}

bool KeybindTab::onSetKeybind(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	currentInput = (INPUT)event.GUIEvent.Caller->getID();
	guiController->setKeybindPopup("Set Key", std::bind(&KeybindTab::onKeyPress, this, std::placeholders::_1));
	guiController->showKeybindPopup();
	return false;
}
bool KeybindTab::onKeyPress(const SEvent& event)
{
	EKEY_CODE key = KEY_KEY_CODES_COUNT;
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		key = event.KeyInput.Key;
	}
	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) key = KEY_LBUTTON;
		if (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN) key = KEY_RBUTTON;
		if (event.MouseInput.Event == EMIE_MMOUSE_PRESSED_DOWN) key = KEY_MBUTTON;
	}
	if (key == KEY_KEY_CODES_COUNT) return true;

	if (keyDesc.find(key) == keyDesc.end()) {
		audioDriver->playMenuSound("menu_error.ogg");
		return true;
	}
	if (keyCfg->keyBound(key)) {
		INPUT previousBind = keyCfg->boundTo(key);
		if (previousBind != currentInput) {
			conflictingInput = previousBind;
			//keyCfg->key[previousBind] = KEY_KEY_CODES_COUNT;
			std::string conflictingBind = wstrToStr(inputNames.at(previousBind));
			std::string bindMsg = wstrToStr(keyDesc.at(key)) + " was already bound to " + conflictingBind + ".\n\n" + "Do you want to unbind " + conflictingBind + "?";
			std::wstring bind = inputNames.at(previousBind) + L": " + keyDesc.at(KEY_KEY_CODES_COUNT);
			//keys[previousBind]->setText(bind.c_str());

			guiController->setYesNoPopup("Key Conflict", bindMsg, std::bind(&KeybindTab::onYesUnbind, this, std::placeholders::_1));
			guiController->showYesNoPopup();
		}
	}
	keyCfg->key[currentInput] = key;
	std::wstring name = inputNames.at(currentInput) + L": " + keyDesc.at(keyCfg->key[currentInput]);
	keys[currentInput]->setText(name.c_str());
	return false;
}

bool KeybindTab::onYesUnbind(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	keyCfg->key[conflictingInput] = KEY_KEY_CODES_COUNT;
	std::wstring bind = inputNames.at(conflictingInput) + L": " + keyDesc.at(KEY_KEY_CODES_COUNT);
	keys[conflictingInput]->setText(bind.c_str());
	guiController->hidePopup(event);
	return false;
}