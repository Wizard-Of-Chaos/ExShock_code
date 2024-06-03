#pragma once
#ifndef HUDCONTACT_H
#define HUDCONTACT_H
#include "BaseHeader.h"
#include "HUDElement.h"
#include "FactionComponent.h"

/*
* This element shows the given contact that's currently in range of the ship's sensors. It contains the contact,
* info to display itself, and logic to show when the contact is on-screen and a marker at the edge of the screen when
* it's not.
*/
class HUDContact : public HUDElement
{
public:

	HUDContact(IGUIElement* root, flecs::entity contactId, flecs::entity playerId, bool objective=false, bool radio=false);
	~HUDContact() override;
	void setObjective(bool obj, std::string str="");
	virtual void updateElement(flecs::entity playerId);
	bool isValidContact(); //checks self
	void updateType(const FactionComponent* contactFac);
	void checkRange();
	flecs::entity contact;

	IGUIImage* contactView;
	IGUIImage* offscreenMarker;
	FACTION_TYPE lastType = FACTION_NEUTRAL;
	std::string objtype = "";
	bool isObjective = false;
	bool isRadio = false;
	bool inRange = false;
	bool isDirty = false;
	bool isSelected = false;
private:
	enum HUDOBJ_TYPE {
		NEUTRAL = 0,
		FRIENDLY = 1,
		HOSTILE = 2,
		HAZARD = 3,
		RADIO = 4,
		OBJECTIVE = 5
	};
	const inline HUDOBJ_TYPE m_getType(const FactionComponent* contactFac) const;
	inline ITexture* m_contact(const HUDOBJ_TYPE& which, const bool& important) const;
	inline ITexture* m_marker(const HUDOBJ_TYPE& which, const bool& important) const;
	HUDOBJ_TYPE m_type = (HUDOBJ_TYPE)-1; //this always feels so fucking silly. like I know it's just an int but it feels wrong
};

#endif 