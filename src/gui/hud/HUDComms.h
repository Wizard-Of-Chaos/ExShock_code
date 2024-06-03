#pragma once
#include "BaseHeader.h"
#include "HUDElement.h"
#include "HUDFillBar.h"

struct InputComponent;
struct PlayerComponent;
struct SensorComponent;

enum class COMMSTATE
{
	ROOT,
	DISPLAY_ALL_CONTACTS,
	ORDER_ALL_UNITS,
	ORDER_ALL_WINGMEN,
	ORDER_SPECIFIC_UNIT
};

class HUDComms : public HUDElement
{
	public:
		HUDComms(IGUIElement* rootHUD, IGUIElement* rootPanel);
		~HUDComms() override;
		virtual void updateElement(flecs::entity playerId);
		COMMSTATE state() { return commState; }
		void setState(COMMSTATE state) { commState = state; }
		IGUIStaticText* panel;
	private:
		void m_displayOrderList();
		void m_handleWingmenHpDisplay(const InputComponent* in);
		void m_displayContactList(const SensorComponent* sns);
		void m_handleContactSelect(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns);
		void m_handleTargetOrder(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns);
		void m_handleAllWingmenOrder(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns);
		void m_handleAllOrderableUnitsOrder(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns);
		COMMSTATE commState = COMMSTATE::ROOT;

		IGUIStaticText* dummy;
		IGUIStaticText* openCommsReminder;
		IGUIStaticText* wingmenNames[MAX_WINGMEN_ON_WING];
		FillBar* wingmenHP[MAX_WINGMEN_ON_WING];
		FillBar* wingmenSP[MAX_WINGMEN_ON_WING];
		flecs::entity selectedEntity = INVALID_ENTITY;
};