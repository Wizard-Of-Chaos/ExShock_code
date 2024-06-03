#include "GuiCreditsMenu.h"
#include "GuiController.h"
#include "GameStateController.h"

void GuiCreditsMenu::init()
{
	if (root) root->remove();
	root = guienv->addStaticText(L"", recti(position2di(0, 0), baseSize), false, false);
	scaleAlign(root);

	dimension2du txtSize = baseSize;
	txtSize.Height -= 90;
	txtSize.Width /= 2;
	auto txt = guienv->addStaticText(L"", recti(vector2di(0, 25), txtSize), false, true, root);
	setAltUIText(txt);
	std::ifstream in("assets/CREDITS.txt");
	std::stringstream buf;
	buf << in.rdbuf();
	txt->setText(wstr(buf.str()).c_str());

	buf.clear();
	in.close();
	in.clear();

	txt = guienv->addStaticText(L"", recti(vector2di(baseSize.Width / 2, 25), txtSize), false, true, root);
	setUIText(txt);
	std::ifstream thanksin("assets/SPECIALTHANKS.txt");
	std::stringstream thanksbuf;
	thanksbuf << thanksin.rdbuf();
	txt->setText(wstr(thanksbuf.str()).c_str());

	back = guienv->addButton(rect<s32>(position2di(420,490), dimension2du(120, 40)), root, -1, L"Back", L"Stop appreciating the devs... for now.");
	setHoloButton(back);

	guiController->setCallback(back, std::bind(&GuiCreditsMenu::onBack, this, std::placeholders::_1), GUI_CREDITS_MENU);
	hide();
}

void GuiCreditsMenu::show()
{
	GuiDialog::show();
	if (stateController->isInitialized()) stateController->toggleMenuBackdrop();
}

bool GuiCreditsMenu::onBack(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	guiController->setActiveDialog(GUI_MAIN_MENU);
	return false;
}