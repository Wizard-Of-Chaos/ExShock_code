#pragma once
#ifndef GAMESETTINGSTAB_H
#define GAMESETTINGSTAB_H
#include "BaseHeader.h"
#include "SettingsTab.h"

class GameTab : public SettingsTab
{
	public:
		void build(IGUIElement* root);
		virtual void show();
		bool onToggle(const SEvent& event);
		bool onLevelSet(const SEvent& event);
		bool onIncreaseVol(const SEvent& event);
		bool onDecreaseVol(const SEvent& event);
	private:
		IGUIButton* playerDmg;
		IGUIButton* enemyDmg;
		IGUIButton* aiInt;
		IGUIButton* aiAim;
		IGUIButton* aiCoward;
		IGUIButton* aiNum;
		IGUIButton* linSpaceFriction;
		IGUIButton* angSpaceFriction;
		IGUIButton* constThrust;
		IGUIButton* friendlyFire;
		IGUIButton* impact;
		IGUIButton* banter;
		struct _audioButtons {
			IGUIButton* l;
			IGUIStaticText* txt;
			IGUIStaticText* lvl;
			IGUIButton* r;
		};
		_audioButtons volumes[4];

};
#endif 