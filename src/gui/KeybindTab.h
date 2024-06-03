#pragma once
#ifndef KEYBINDTAB_H
#define KEYBINDTAB_H
#include "BaseHeader.h"
#include "SettingsTab.h"
#include "InputComponent.h"
class KeybindTab : public SettingsTab
{
public:
	virtual void show();
	void build(IGUIElement* root, KeyConfig* cfg);
	bool onSetKeybind(const SEvent& event);
	bool onKeyPress(const SEvent& event);
	bool onYesUnbind(const SEvent& event);
private:
	INPUT currentInput;
	INPUT conflictingInput;
	KeyConfig* keyCfg;
	IGUIButton* keys[IN_MAX_ENUM];
};
#endif 