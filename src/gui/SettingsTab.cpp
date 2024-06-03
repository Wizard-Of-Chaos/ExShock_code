#include "SettingsTab.h"
#include "GuiController.h"
#include "Config.h"

void SettingsTab::m_baseBuild(IGUIElement* root)
{
	IGUIImage* img = guienv->addImage(rect<s32>(position2di(240, 85), dimension2du(480, 336)), root);
	img->setImage(driver->getTexture("assets/ui/baselistitem.png"));
	scaleAlign(img);
	base = img;

	img = guienv->addImage(rect<s32>(position2di(0, 40), dimension2du(480, 2)), base);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);

	img = guienv->addImage(rect<s32>(position2di(239, 40), dimension2du(2, 336)), base);
	img->setImage(driver->getTexture("assets/ui/uiline.png"));
	scaleAlign(img);

	explain = guienv->addStaticText(L"None", rect<s32>(position2di(241, 62), dimension2du(231, 180)), false, true, base);
	setUIText(explain);

	restart = guienv->addStaticText(L"", rect<s32>(position2di(10, 10), dimension2du(460, 25)), false, true, base);
	setUIText(restart);

}
