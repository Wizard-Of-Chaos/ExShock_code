#include "FabMenuTab.h"
#include "GuiController.h"
#include "Campaign.h"

void FabMenuTab::m_basebg(IGUIElement* root)
{
	dimension2du tabSize = dimension2du(460, 330);
	IGUIImage* img = guienv->addImage(recti(position2di(260, 80), tabSize), root);
	img->setImage(driver->getTexture("assets/ui/topfabbar.png"));
	scaleAlign(img);
	base = img;
	listBg = guienv->addStaticText(L"", recti(vector2di(5, 5), dimension2du(160, 336)), false, true, base);
	setUIText(listBg);
	guiController->setCallback(listBg, std::bind(&LoadoutTab::onListScroll, this, std::placeholders::_1), which, GUICONTROL_KEY | GUICONTROL_MOUSE);

	listItemVertSizeRatio = (20.f / 540.f);
	listItemHorizSizeRatio = (160.f / 960.f);
	listStartXRatio = (0.f / 960.f);
	listStartYRatio = (0.f / 540.f);

	longdesc = guienv->addStaticText(L"None", recti(position2di(160, 30), dimension2du(300, 180)), false, true, base);
	setUIText(longdesc);
	desc = guienv->addStaticText(L"None", recti(position2di(160, 210), dimension2du(300, 70)), false, true, base);
	setUIText(desc);
	confirm = guienv->addButton(recti(position2di(160, 0), dimension2du(300, 30)), base, -1, L"Confirm", L"Confirm item selection for building or scrapping.");
	setHoloButton(confirm, BCOL_GREEN); //ditto
	confirm->setVisible(false);
}

void FabMenuTab::show()
{
	base->setVisible(true);
	m_clearList();
	if(desc) desc->setText(L"");
	if(longdesc) longdesc->setText(L"");
	confirm->setVisible(false);
}

void FabMenuTab::m_showSupplies()
{
	if (fab) fab->showSupplies();
}