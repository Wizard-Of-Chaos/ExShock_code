#pragma once
#ifndef SAVEGAMETAB_H
#define SAVEGAMETAB_H
#include "BaseHeader.h"
#include "OptionsTab.h"

class SaveGameTab : public OptionsTab
{
	public:
		void build(IGUIElement* root);
		virtual void show();
		bool onSave(const SEvent& event);
		bool onNewSave(const SEvent& event);
		bool saveConfirm(const SEvent& event);
	private:
		IGUIButton* newSave;

};
#endif 