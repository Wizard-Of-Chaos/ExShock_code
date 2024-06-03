#include "HUDPopup.h"
#include "GameController.h"
#include "GameAssets.h"
#include "IrrlichtUtils.h"

HUDPopup::HUDPopup(IGUIElement* root, s32 slot, std::string msgstr, std::string spkrstr) : HUDElement(root)
{

	const f32 horRatio = (399.f / 1920.f); //Horatio loves you. Join Horatio.
	const f32 verRatio = (98.f / 1080.f);
	rect<s32> screen = root->getRelativePosition();
	size = dimension2du(screen.getWidth() * horRatio, screen.getHeight() * verRatio);
	u32 starter = (u32)(screen.getHeight() * (100 / 1080.f));
	s32 len = (s32)size.Width;
	bg = guienv->addImage(rect<s32>(position2di(-len, starter + size.Height * slot), size), root);
	scaleAlign(bg);
	//starts offscreen and then zoots in-screen
	bg->setImage(assets->getTexture("assets/hud/popup_low.png"));
	bg->setUseAlphaChannel(true);
	bg->setColor(SColor(255, 255, 255, 255));
	bg->setNotClipped(true);
	bg->setVisible(true);

	msg = guienv->addStaticText(L"", rect<s32>(position2di(0,0), size), false, true, bg);
	set(spkrstr, msgstr);
	setHUDTextSmall(msg);
}
HUDPopup::~HUDPopup()
{
	msg->remove();
	//spkr->remove();
	bg->remove();
}
void HUDPopup::updateElement(flecs::entity playerId)
{
	//we don't actually need to update here? just dealing with timers
	s32 len = (s32)size.Width;
	auto pos = bg->getRelativePosition().UpperLeftCorner;
	if (currentDuration <= .15f) {
		bg->setRelativePosition(position2di(-len + (len * (currentDuration / .15f)), pos.Y));
	}
	else if (currentDuration > duration - .15f) {
		f32 percent = ((currentDuration - (duration - .15f)) / .15f);
		bg->setRelativePosition(position2di(-len * percent, pos.Y));
	}
}
void HUDPopup::set(std::string spkr, std::string msgstr)
{
	if (msg) msg->setText(wstr(spkr + "\n" + msgstr).c_str());

}
void HUDPopup::setMsg(std::string msgstr)
{
	if (msg) msg->setText(wstr(msgstr).c_str());
}