#include "HUDHazard.h"
#include "IrrlichtUtils.h"
#include "GameAssets.h"
#include "CrashLogger.h"

const std::unordered_map<HAZARD, std::string> hazNames =
{
	{HAZARD::SHIELD_DRAIN, "SHIELD DRAIN"},
	{HAZARD::GRAVITY_ANOMALY, "GRAVITY ANOMALY"},
	{HAZARD::RADIATION, "RADIATION"},
	{HAZARD::INCOMING_MISSILE, "INCOMING MISSILE"},
	{HAZARD::SLOWDOWN, "SLOWDOWN"},
	{HAZARD::SPEED_BOOST, "SPEED BOOST"},
	{HAZARD::CAPTURING_STATION, "CAPTURING STATION"}
};

HUDHazard::HUDHazard(IGUIElement* root, flecs::entity cause, HAZARD which, u32 hazCount) : HUDElement(root), cause(cause), which(which)
{
	f32 ratio = (120.f / 1920.f);
	f32 vertRatio = (120.f / 1080.f);
	rect<s32> screen = root->getRelativePosition();
	dimension2du size(screen.getWidth() * ratio, screen.getHeight() * vertRatio);
	s32 width = screen.getWidth();
	position2di pos(width - ((size.Width * (hazCount+1)) + 40), 8);
	img = guienv->addImage(rect<s32>(pos, size), root);
	img->setImage(assets->getTexture("assets/hud/" + hazNames.at(which) + ".png"));
	scaleAlign(img);
	img->setVisible(true);

	txt = guienv->addStaticText(wstr(hazNames.at(which)).c_str(), rect<s32>(position2di(0, size.Height), dimension2du(size.Width, 48)), false, true, img);
	txt->setNotClipped(true);
	setHUDTextSmall(txt);
	txt->setOverrideColor(SColor(255, 255, 133, 0));
}

HUDHazard::~HUDHazard()
{
	if (img) img->remove();
	//if (txt) txt->remove();
}

const bool HUDHazard::valid() const {
	if (!cause.is_alive() || invalid) {
		return false;
	}
	if (timeout && timeoutTimer >= timeoutTime) {
		return false;
	}
	return true;
}

void HUDHazard::updateElement(flecs::entity playerId)
{
	if (!cause.is_alive()) return;
	if (flashTimer > flashTime) {
		bool vis = img->isVisible();
		img->setVisible(!vis);
		flashTimer = 0.f;
	}
}