#include "CarrierUpgradeTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include <iostream>

void CarrierUpgradeTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE which)
{
	m_basebg(root);
	guiController->setCallback(confirm, std::bind(&CarrierUpgradeTab::onConfirm, this, std::placeholders::_1), GUI_FAB_MENU);
	fab = (GuiFabMenu*)dial;

	upgradeBlocksBg = guienv->addStaticText(L"", recti(vector2di(160, 210), dimension2du(300, 30)), false, true, base);
	setUIText(upgradeBlocksBg);

	hide();

}
void CarrierUpgradeTab::show()
{
	FabMenuTab::show();
	m_showSupplies();
	m_clearList();
	m_buildUpgradeList();
	longdesc->setText(L"");
	confirm->setVisible(false);
	upgradeBlocksBg->setVisible(false);
	confirm->setText(L"Upgrade Chaos Theory");
	confirm->setToolTipText(L"Confirm the selected upgrade for the Chaos Theory.");
}

bool CarrierUpgradeTab::onConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;

	if (campaign->getFlag(L"MARTIN_BUILDING_CLOAK")) {
		guiController->setOkPopup("Locked", "The fabrication bay is not currently operational, owing to Martin's ongoing work with the cloaking device.");
		guiController->showOkPopup();
		return true;
	}

	if (!campaign->getCarrierUpgrade(currentSelection)) return true;

	if (campaign->getCarrierUpgrade(currentSelection)->cost > campaign->getSupplies()) {
		guiController->setOkPopup("Not Enough Supplies", "You don't have the supplies to get this upgrade. Go on some more scrap missions.");
		guiController->showOkPopup();
		return false;
	}
	campaign->removeSupplies(campaign->getCarrierUpgrade(currentSelection)->cost);
	campaign->increaseCarrierUpgradeTier(currentSelection);
	m_clearList();
	m_buildUpgradeList();
	m_showSupplies();
	m_buildBlocks();
	longdesc->setText(wstr(m_carrUpgradeStr(currentSelection)).c_str());

	return false;
}
bool CarrierUpgradeTab::onUpgradeSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	std::string name = event.GUIEvent.Caller->getName();
	auto up = campaign->getCarrierUpgrade(name);
	if (!up) return false;
	currentSelection = name;
	longdesc->setText(wstr(m_carrUpgradeStr(name)).c_str());
	confirm->setVisible(true);
	upgradeBlocksBg->setVisible(true);
	m_buildBlocks();
	return false;
}

void CarrierUpgradeTab::m_buildBlocks()
{
	f32 blockVertRatio = (25.f / 540.f);
	f32 totalHorizRatio = (300.f / 960.f);
	u32 realEstate = totalHorizRatio * driver->getScreenSize().Width;
	u32 blockHorizSize = realEstate / campaign->getCarrierUpgrade(currentSelection)->maxtier;
	u32 blockVertSize = upgradeBlocksBg->getRelativePosition().getHeight();
	dimension2du blockSize(blockHorizSize, blockVertSize);
	for (auto& block : blocks) {
		block->remove();
	}
	blocks.clear();

	for (u32 i = 0; i < campaign->getCarrierUpgrade(currentSelection)->maxtier; ++i) {

		if (i == campaign->getCarrierUpgrade(currentSelection)->maxtier - 1) { //make sure the last one fills
			u32 used = i * blockSize.Width;
			blockSize.Width = realEstate - used;
		}

		auto block = guienv->addImage(recti(vector2di(blockHorizSize * i, 0), blockSize), upgradeBlocksBg);
		block->setImage(driver->getTexture("assets/ui/upgradeblock.png"));
		scaleAlign(block);
		f32 ratio = ((f32)i / (f32)campaign->getCarrierUpgrade(currentSelection)->maxtier);
		SColor start(255, 215, 220, 70);
		SColor end(255, 20, 255, 50);
		if (i < campaign->getCarrierUpgrade(currentSelection)->tier) block->setColor(end.getInterpolated(start, ratio));
		else block->setColor(SColor(200 * ratio + 40, 200, 200, 200));
		block->setNotClipped(true);
		blocks.push_back(block);
	}
}

void CarrierUpgradeTab::m_buildUpgradeList()
{
	u32 count = 0;
	for (auto &[name, up] : campaign->carrierUpgrades()) {
		if (!up.canBuild) continue;
		bool maxed = (up.tier == up.maxtier);
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		std::string tierupgrade = name + " - ";

		if (maxed)
			tierupgrade += "Maximum";
		else
			tierupgrade += "Tier " + std::to_string(up.tier + 1);

		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, count, wstr(tierupgrade).c_str(), L"Increase this carrier upgrade.");
		button->setName(name.c_str());
		BUTTON_COLOR color = BCOL_LIGHTBLUE;
		if (up.tier >= BCOL_MAX) color = BCOL_ORANGE;
		else color = (BUTTON_COLOR)up.tier;
		setThinHoloButton(button, color);
		if (!maxed) {
			guiController->setCallback(button, std::bind(&CarrierUpgradeTab::onUpgradeSelect, this, std::placeholders::_1), GUI_FAB_MENU);
		}
		else {
			button->setEnabled(false);
		}
		listItems.push_back(button);
	}
}

std::string CarrierUpgradeTab::m_carrUpgradeStr(std::string which)
{
	auto up = campaign->getCarrierUpgrade(which);
	if (!up) return "";

	std::string ret = up->name + "\n\n";
	ret += up->desc;
	ret += "\n\n";
	ret += "Cost - " + fprecis(up->cost, 5) + "\n";
	return ret;
}