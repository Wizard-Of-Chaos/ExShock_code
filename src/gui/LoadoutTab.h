#pragma once
#ifndef LOADOUTTABS_H
#define LOADOUTTABS_H
#include "BaseHeader.h"
#include "WeaponInfoComponent.h"
#include "IrrlichtUtils.h"
#include "GuiDialog.h"
#include <functional>

class GuiLoadoutMenu;
struct WingmanInstance;
struct ShipInstance;
struct WeaponInstance;
class ShipUpgradeInstance;
class WeaponUpgradeInstance;

class LoadoutTab
{
	public:
		virtual void build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which) = 0;
		virtual void hide() { base->setVisible(false); }
		virtual void show();
		bool onListScroll(const SEvent& event);
	protected:
		GuiDialog* dialog;
		MENU_TYPE which;

		std::list<IGUIElement*> listItems;
		void m_clearList();
		virtual void m_basebg(IGUIElement* root);
		bool m_hoverClickCheck(const SEvent& event);
		dimension2du m_listItemSize();
		position2di m_listItemStart();
		u32 m_listItemHeightDiff();

		void m_buildShipList(std::function<bool(const SEvent&)> cb, bool hideUsed = true, bool includesNone = true, BUTTON_COLOR color=BCOL_BLUE);
		void m_buildWepList(std::function<bool(const SEvent&)> cb, bool hideUsed = true, HARDPOINT_TYPE type = HRDP_REGULAR, bool includesNone = true, BUTTON_COLOR color = BCOL_BLUE);
		void m_buildShipUpgradeList(std::function<bool(const SEvent&)> cb, bool hideUsed = true, bool includesNone = true, BUTTON_COLOR color = BCOL_BLUE);
		void m_buildWepUpgradeList(std::function<bool(const SEvent&)> cb, bool hideUsed = true, bool includesNone = true, BUTTON_COLOR color = BCOL_BLUE);

		virtual bool m_hoverWeapon(s32 id, bool full = false);
		virtual bool m_hoverShip(s32 id, bool full = false);
		virtual bool m_hoverShipUpgrade(s32 id, bool full = false);
		virtual bool m_hoverWepUpgrade(s32 id, bool full = false);
		virtual void m_showSupplies();

		std::string m_wingDescStr(WingmanInstance* mans);
		std::string m_wingStatStr(WingmanInstance* mans, bool name=true);

		std::string m_shipDescStr(ShipInstance* inst, bool includeCostVals=false);
		std::string m_shipStatStr(ShipInstance* inst, bool includeName=true);
		std::string m_weaponDescStr(WeaponInstance* inst, bool includeCostVals = false);
		std::string m_weaponStatStr(WeaponInstance* inst, bool includeName=true);
		std::string m_shipUpgradeDescStr(ShipUpgradeInstance* inst, bool includeCostVals = false);
		std::string m_shipUpgradeStatStr(ShipUpgradeInstance* inst, bool includeName = true);
		std::string m_wepUpgradeDescStr(WeaponUpgradeInstance* inst, bool includeCostVals = false);
		std::string m_wepUpgradeStatStr(WeaponUpgradeInstance* inst, bool includeName = true);

		IGUIElement* base;
		IGUIStaticText* desc;
		IGUIImage* sideImg;
		IGUIStaticText* longdesc;
		IGUIStaticText* listBg;

		f32 listItemVertSizeRatio;
		f32 listItemHorizSizeRatio;
		f32 listStartXRatio;
		f32 listStartYRatio;

		s32 listScrollPos = 0;

		position2di descPos = position2di(0, 136);
		dimension2du descSize = dimension2du(300, 90);
		position2di longDescPos = position2di(150, 0);
		dimension2du longDescSize = dimension2du(150, 240);

		void m_buildLoadoutBottomShipView();
		void m_buildLoadoutSideShipView();
		struct _loadoutShipView {
			IGUIImage* shipDescBg;
			IGUIStaticText* shipDescName;
			IGUIStaticText* shipWepNames[6];
			IGUIImage* shipWepIcons[6];

			IGUIStaticText* shipUpNames[4];

			IGUIStaticText* shipHvyWepName;
			IGUIStaticText* shipPhysWepName;
			IGUIImage* hpBar;
			IGUIImage* hvyIcon;
			IGUIImage* physIcon;
			virtual void showShip(ShipInstance* inst);
			void clear() { if (shipDescBg) shipDescBg->remove(); }
			void hide() { shipDescBg->setVisible(false); }

		};
		struct _loadoutSideShipView : public _loadoutShipView {
			virtual void showShip(ShipInstance* inst) override;

		};
		_loadoutShipView loadoutBottomShipView;
		_loadoutSideShipView loadoutSideShipView;
};
#endif