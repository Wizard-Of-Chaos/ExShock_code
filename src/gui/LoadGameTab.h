#pragma once
#ifndef LOADGAMETAB_H
#define LOADGAMETAB_H
#include "BaseHeader.h"
#include "OptionsTab.h"

class LoadGameTab : public OptionsTab
{
	public:
		void build(IGUIElement* root);
		virtual void show();
		bool onLoad(const SEvent& event);
		bool onLoadConfirm(const SEvent& event);
	private:
};

#endif 