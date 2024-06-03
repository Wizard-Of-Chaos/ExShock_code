#pragma once
#ifndef OPTIONSTAB_H
#define OPTIONSTAB_H
#include "BaseHeader.h"

class OptionsTab
{
	public:
		virtual void hide() { base->setVisible(false); }
		virtual void show() { base->setVisible(true); }
		bool onGameSelect(const SEvent& event);
		bool onRemove(const SEvent& event);
		bool removeConfirm(const SEvent& event);
	protected:
		s32 m_currentSelection=-1;
		void m_baseBuild(IGUIElement* root);
		void m_loadSaves();
		void m_clearList();
		void m_displaySave();
		dimension2du m_listItemSize();
		position2di m_listItemStart();
		u32 m_listItemHeightDiff();

		std::vector<IGUIButton*> saves;
		IGUIButton* confirm;
		IGUIButton* remove;
		IGUIStaticText* savedesc;
		IGUIElement* base;

		f32 listItemVertSizeRatio = (20.f / 540.f);
		f32 listItemHorizSizeRatio = (227.f / 960.f);
		f32 listStartXRatio = (10.f / 960.f);
		f32 listStartYRatio = (45.f / 540.f);
		f32 listBufferRatio = (4.f / 540.f);
};

#endif