#pragma once
#ifndef SETTINGSTAB_H
#define SETTINGSTAB_H
#include "BaseHeader.h"
#include "GuiDialog.h"

struct VideoConfig;
struct KeyConfig;

class SettingsTab
{
public:
	virtual void hide() { base->setVisible(false); }
	virtual void show() { base->setVisible(true); }
protected:
	void m_baseBuild(IGUIElement* root);
	IGUIElement* base;
	IGUIStaticText* restart;
	IGUIStaticText* explain = nullptr;
};

#endif 