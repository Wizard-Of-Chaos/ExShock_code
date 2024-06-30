#include "HUDLargePopup.h"
#include "GameAssets.h"
#include "IrrlichtUtils.h"
#include "AudioDriver.h"
#include "GameController.h"

HUDLargePopup::HUDLargePopup(IGUIElement* root) : HUDElement(root)
{
	//Join Horatio. Initialize the GUI element. Become one with Horatio.
	f32 horRatio = 928.f / 1920.f;
	f32 verRatio = 205.f / 1080.f;

	rect<s32> screen = root->getRelativePosition();
	size = dimension2du(screen.getWidth() * horRatio, screen.getHeight() * verRatio);
	s32 height = (s32)(verRatio * screen.getHeight());
	position2di startPos = position2di((496.f/1920.f) * screen.getWidth(), -height);
	bg = guienv->addImage(rect<s32>(startPos, size), root, -1);
	bg->setImage(assets->getTexture("assets/hud/popup_high.png"));
	scaleAlign(bg);
	bg->setUseAlphaChannel(true);
	bg->setColor(SColor(255, 255, 255, 255));
	bg->setNotClipped(true);
	bg->setVisible(true);
	
	dimension2du boxSize((892.f / 1920.f) * screen.getWidth(), (187.f / 1080.f) * screen.getHeight());

	dimension2du spkrSize(screen.getWidth() * (128.f / 1920.f), screen.getHeight() * (128.f / 1080.f));
	dimension2du frameSize(screen.getWidth() * (136.f / 1920.f), screen.getHeight() * (136.f / 1080.f));
	position2di spkrPos(spkrSize.Width / 2.f, (boxSize.Height - spkrSize.Height) / 3.f);
	spkr = guienv->addImage(rect<s32>(spkrPos, spkrSize), bg, -1);
	scaleAlign(spkr);
	spkr->setColor(SColor(240, 140, 235, 255));
	spkr->setVisible(true);
	spkrFrame = guienv->addImage(rect<s32>(position2di(-screen.getWidth()*(4.f/1920.f), -screen.getHeight()*(4.f/1080.f)), frameSize), spkr);
	scaleAlign(spkrFrame);
	spkrFrame->setImage(assets->getTexture("assets/hud/popup_headshot_high.png"));
	spkrFrame->setUseAlphaChannel(true);
	spkrFrame->setVisible(true);
	spkrFrame->setNotClipped(true);

	name = guienv->addStaticText(L"", rect<s32>(position2di(-(s32)(spkrSize.Width)/2.f, spkrSize.Height + ((10.f / 1080.f) * screen.getHeight())), dimension2du(spkrSize.Width*2, spkrPos.Y)), false, true, spkr);
	setHUDTextSmall(name);
	name->setNotClipped(true);
	name->setVisible(true);

	u32 xpos = (spkrPos.X * 2) + spkrSize.Width;
	msg = guienv->addStaticText(L"", rect<s32>(position2di(xpos, 0), dimension2du(boxSize.Width - xpos, boxSize.Height)), false, true, bg);
	setHUDText(msg);
	msg->setVisible(true);
}

HUDLargePopup::~HUDLargePopup()
{
	if (spkrFrame) spkrFrame->remove();
	if (name) name->remove();
	if (spkr) spkr->remove();
	if (msg) msg->remove();
	if (bg) bg->remove();
}
void HUDLargePopup::updateElement(flecs::entity playerId)
{
	if (gameController->startAnim()) return;

	if (curDuration >= duration) {
		if (!queuedMessages.empty()) {
			m_changeMsg(queuedMessages.front());
			currentMsg = queuedMessages.front();
			queuedMessages.pop_front();
			curDuration = 0.f;
		}
		else {
			return;
		}
	}
	s32 len = (s32)size.Height;
	auto pos = bg->getRelativePosition().UpperLeftCorner;
	if (curDuration <= .2f) {
		auto newPos = position2di(pos.X, -len + (len * (curDuration / .2f)));
		bg->setRelativePosition(newPos);
	}
	else if (curDuration > duration - .2f) {
		f32 percent = ((curDuration - (duration - .2f)) / .2f);
		auto newPos = position2di(pos.X, -len * percent);
		bg->setRelativePosition(newPos);
	}
}

void HUDLargePopup::m_changeMsg(_waiting which)
{
	msg->setText(wstr(which.msg).c_str());
	ITexture* tex = nullptr;
	if (which.spkr == "Tutorial")
		tex = assets->getTexture("assets/ui/aim.png", false);
	else
		tex = assets->getTexture("assets/ui/portraits/" + which.spkr + ".png", false);

	if (tex) {
		spkr->setImage(tex);
		spkr->setVisible(true);
	}
	else {
		spkr->setVisible(false);
	}
	if (!which.banter) {
		audioDriver->playGameSound(gameController->getPlayer(), "incoming_message.ogg", 1.f, 20.f, 1200.f, false, true);
	}
	else {
		audioDriver->playGameSound(gameController->getPlayer(), "incoming_banter.ogg", 1.f, 20.f, 1200.f, false, true);
	}
	name->setText(wstr(which.spkr).c_str());
}

void HUDLargePopup::showMsg(std::string msg, std::string spkr, bool banter)
{
	if (banter) {
		queuedMessages.push_back({ spkr, msg, banter });
	}
	else {
		if (currentMsg.banter) {
			curDuration = duration;
			queuedMessages.push_front({ spkr, msg, banter });
		}
		else {
			queuedMessages.push_back({ spkr, msg, banter });
		}
	}
}