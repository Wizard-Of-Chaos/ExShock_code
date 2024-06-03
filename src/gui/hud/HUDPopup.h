#pragma once
#ifndef HUDPOPUP_H
#define HUDPOPUP_H
#include "BaseHeader.h"
#include "HUDElement.h"

class HUDPopup : public HUDElement
{
	public:
		HUDPopup(IGUIElement* root, s32 slot, std::string msg="", std::string spkr="");
		~HUDPopup() override;
		virtual void updateElement(flecs::entity playerId);
		void set(std::string spkr, std::string msg);
		void setMsg(std::string msg);
		f32 duration = 3.5f;
		f32 currentDuration = 0.f;
		bool finished = false;
	private:
		IGUIImage* bg = nullptr;
		IGUIStaticText* msg=nullptr;
		dimension2du size;
};

#endif