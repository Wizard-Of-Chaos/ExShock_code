#include "CarrierTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include "GameController.h"
#include "LoadoutData.h"
#include "ShipInstance.h"
#include <iostream>
#include <sstream>
#include <string>

void CarrierTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE menu)
{
	dialog = dial;
	which = menu;
	m_basebg(root);
	auto flavor = guienv->addImage(recti(longDescPos, longDescSize), base);
	flavor->setImage(driver->getTexture("assets/ui/sidelistbg.png"));
	scaleAlign(flavor);
	hide();
	desc->setVisible(false);
}

void CarrierTab::show()
{
	LoadoutTab::show();
	desc->setText(L"");
	longdesc->setText(L"");
	m_clearList();
	m_buildUpgradeList();
	if (listItems.empty()) longdesc->setText(L"The Chaos Theory currently has no upgrades.");
}
bool CarrierTab::onHover(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_ELEMENT_HOVERED) return true;
	if (!campaign->getCarrierUpgrade(event.GUIEvent.Caller->getName())) return true;
	auto up = campaign->getCarrierUpgrade(event.GUIEvent.Caller->getName());
	std::string stats = "Tier - " + std::to_string(up->tier) + "\nMax Tier - " + std::to_string(up->maxtier);
	std::string description = up->name + "\n\n" + up->desc + "\n\n" + stats;
	longdesc->setText(wstr(description).c_str());
	return false;
}
void CarrierTab::m_buildUpgradeList()
{
	u32 count = 0;
	for (auto [name, up] : campaign->carrierUpgrades()) {
		if (up.tier == 0) continue;
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), base, count, wstr(name).c_str(), L"View this upgrade.");
		BUTTON_COLOR color = BCOL_LIGHTBLUE;
		if (up.tier >= BCOL_MAX) color = BCOL_ORANGE;
		else color = (BUTTON_COLOR)up.tier;
		setThinHoloButton(button, color);
		button->setName(name.c_str());
		guiController->setCallback(button, std::bind(&CarrierTab::onHover, this, std::placeholders::_1), which);
		listItems.push_back(button);
	}
}
