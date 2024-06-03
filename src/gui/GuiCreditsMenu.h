#pragma once
#ifndef GUICREDITSMENU_H
#define GUICREDITSMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"

class GuiCreditsMenu : public GuiDialog
{
	public:
		virtual void init();
		virtual void show();
		bool onBack(const SEvent& event);
	private:
		IGUIButton* back;
};

#endif