#pragma once
#ifndef HUDLARGEPOPUP_H
#define HUDLARGEPOPUP_H
#include "BaseHeader.h"
#include "HUDElement.h"

class HUDLargePopup : public HUDElement
{
	public:
		HUDLargePopup(IGUIElement* root);
		~HUDLargePopup() override;
		void showMsg(std::string msg, std::string spkr = "", bool banter=false);
		virtual void updateElement(flecs::entity playerId);
		f32 duration = 5.f;
		f32 curDuration = duration;
	private:
		dimension2du size;
		struct _waiting {
			std::string spkr;
			std::string msg;
			bool banter = false;
		};
		_waiting currentMsg;
		void m_changeMsg(_waiting which);

		std::list<_waiting> queuedMessages;
		bool inUse = false;

		IGUIImage* bg = nullptr;
		IGUIImage* spkrFrame = nullptr;
		IGUIImage* spkr = nullptr;
		IGUIStaticText* name = nullptr;
		IGUIStaticText* msg = nullptr;
};

#endif 