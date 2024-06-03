#pragma once
#ifndef GUILAUNCHMENU_H
#define GUILAUNCHMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"

class GuiLaunchMenu : public GuiDialog
{
	public:
		virtual void init();
		virtual void show();
		bool onLaunch(const SEvent& event);
	private:
		IGUIButton* launch;
		NavMenu nav;
};

#endif 