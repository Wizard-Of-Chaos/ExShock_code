#pragma once
#ifndef GUIMESSHALLMENU_H
#define GUIMESSHALLMENU_H
#include "BaseHeader.h"
#include "GuiDialog.h"

class GuiMessHallMenu : public GuiDialog
{
	public:
		virtual void init();
		virtual void show();
		virtual void hide();
		bool onTalker(const SEvent& event);
		bool onBlixten(const SEvent& event);
	private:
		IGUIButton* talkers[2] = {nullptr, nullptr};
		IGUIImage* bar = nullptr;
		IGUIStaticText* killboard = nullptr;
		IGUIStaticText* totalKills = nullptr;
		IGUIStaticText* totalInjuries = nullptr;
		struct _record {
			IGUIStaticText* bg=nullptr;
			IGUIStaticText* name=nullptr;
			IGUIStaticText* kills=nullptr;
			IGUIStaticText* injuries = nullptr;
		};
		_record pilotRecords[WINGMEN_PER_CAMPAIGN+1];
		NavMenu nav;
		IGUIButton* blixten = nullptr;
};

#endif 