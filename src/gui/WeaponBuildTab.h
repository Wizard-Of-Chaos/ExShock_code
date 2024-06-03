#pragma once
#ifndef WEAPONBUILDTAB_H
#define WEAPONBUILDTAB_H
#include "BaseHeader.h"
#include "FabMenuTab.h"
#include "WeaponInfoComponent.h"

struct WeaponData;

class WeaponBuildTab : public FabMenuTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which);
		bool onWepScrapSelect(const SEvent& event);
		bool onWepBuildSelect(const SEvent& event);
		bool onRegular(const SEvent& event);
		bool onPhysical(const SEvent& event);
		bool onHeavy(const SEvent& event);
		bool onConfirm(const SEvent& event);
		virtual void show();
		virtual void hide();
	private:
		s32 currentSelection = -1;
		HARDPOINT_TYPE curType = HRDP_REGULAR;
		IGUIButton* regList=nullptr;
		IGUIButton* hvyList=nullptr;
		IGUIButton* physList=nullptr;

		IGUIStaticText* name = nullptr;
		IGUIStaticText* statsNames = nullptr;
		IGUIStaticText* statsNumbers = nullptr;
		IGUIStaticText* description = nullptr;
		IGUIStaticText* lineBase = nullptr;
		IGUIImage* wepImage = nullptr;

		IMeshSceneNode* rotatingWep = nullptr;
		IMesh* wepMesh = nullptr;
		ICameraSceneNode* camera = nullptr;

		void m_displayWep(WeaponData* dat);
		void m_wipe();
		vector3df wepPos;
		void m_buildBuildableWepList();
};

#endif 