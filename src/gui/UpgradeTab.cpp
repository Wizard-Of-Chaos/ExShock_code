#include "UpgradeTab.h"
#include "GuiController.h"
#include "Campaign.h"
#include "ShipUpgrades.h"
#include "WeaponUpgrades.h"
#include "AudioDriver.h"

void UpgradeTab::build(IGUIElement* root, GuiDialog* dial, MENU_TYPE menu)
{
	m_basebg(root);
	dialog = dial;
	which = menu;
	fab = (GuiFabMenu*)dial;

	listItemVertSizeRatio = (20.f / 540.f);
	listItemHorizSizeRatio = (150.f / 960.f);
	if (desc) desc->remove();
	if (longdesc) longdesc->remove();

	if (listBg) {
		guiController->removeCallback(listBg);
		listBg->remove();
	}

	listBg = guienv->addStaticText(L"", recti(vector2di(10, 30), dimension2du(150, 320)), false, true, base);
	setUIText(listBg);
	guiController->setCallback(listBg, std::bind(&LoadoutTab::onListScroll, this, std::placeholders::_1), which, GUICONTROL_KEY | GUICONTROL_MOUSE);

	shipList = guienv->addButton(recti(vector2di(10, 0), dimension2du(75, 30)), base, -1, L"Ships", L"Show ship list.");
	setThinHoloButton(shipList);
	wepList = guienv->addButton(recti(vector2di(85, 0), dimension2du(75, 30)), base, -1, L"Weapons", L"Show weapon list.");
	setThinHoloButton(wepList, BCOL_BLUE);

	shipList->setEnabled(false);

	decrease = guienv->addButton(recti(position2di(160, 30), dimension2du(80, 20)), base, 0, L"Decrease", L"Decrement how strong this upgrade is.");
	setThinHoloButton(decrease, BCOL_RED);
	costDisplay = guienv->addStaticText(L"Value Here", recti(position2di(240, 30), dimension2du(140, 20)), false, true, base);
	setAltUIText(costDisplay);

	valDisplay = guienv->addStaticText(L"Desc Here", recti(position2di(160, 50), dimension2du(300, 25)), false, true, base);;
	setAltUITextSmall(valDisplay);

	flavorText = guienv->addStaticText(L"Flavor Here", recti(position2di(160, 125), dimension2du(300, 100)), false, true, base);
	setUIText(flavorText);

	upgradeBlocksBg = guienv->addStaticText(L"", recti(vector2di(160, 100), dimension2du(300, 25)), false, true, base);
	setUIText(upgradeBlocksBg);

	increase = guienv->addButton(recti(position2di(380, 30), dimension2du(80, 20)), base, 0, L"Increase", L"Increment how strong this upgrade is.");
	setThinHoloButton(increase, BCOL_GREEN);

	guiController->setCallback(decrease, std::bind(&UpgradeTab::onDecrement, this, std::placeholders::_1), which);
	guiController->setCallback(increase, std::bind(&UpgradeTab::onIncrement, this, std::placeholders::_1), which);
	guiController->setCallback(shipList, std::bind(&UpgradeTab::onShip, this, std::placeholders::_1), which);
	guiController->setCallback(wepList, std::bind(&UpgradeTab::onWeapon, this, std::placeholders::_1), which);

	guiController->setCallback(confirm, std::bind(&UpgradeTab::onConfirm, this, std::placeholders::_1), which);
}

void UpgradeTab::show()
{
	base->setVisible(true);
	m_showSupplies();

	m_clear();

	m_clearList();
	if (fab->isBuilding()) {
		confirm->setText(L"Build Upgrade");
		confirm->setToolTipText(L"Build this upgrade.");
		setHoloButton(confirm, BCOL_GREEN);
		if (showingShips) m_buildableShipUpgradesList();
		else m_buildableWepUpgradesList();
	}
	else {
		confirm->setText(L"Scrap Upgrade");
		confirm->setToolTipText(L"Scrap this upgrade.");
		setHoloButton(confirm, BCOL_RED);
		if (showingShips) m_buildShipUpgradeList(std::bind(&UpgradeTab::onUpgradeScrapSelect, this, std::placeholders::_1), true, false, BCOL_RED);
		else m_buildWepUpgradeList(std::bind(&UpgradeTab::onUpgradeScrapSelect, this, std::placeholders::_1), true, false, BCOL_RED);
	}
	currentSelection = -1;
}

void UpgradeTab::m_clear()
{
	confirm->setVisible(false);
	decrease->setVisible(false);
	increase->setVisible(false);
	costDisplay->setVisible(false);
	valDisplay->setVisible(false);
	flavorText->setVisible(false);
	upgradeBlocksBg->setVisible(false);
	currentIncrements = 0;
	availableIncrements = 0;
	currentSelection = -1;
}

bool UpgradeTab::onShip(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	showingShips = true;
	m_clear();

	shipList->setEnabled(false);
	wepList->setEnabled(true);
	setThinHoloButton(shipList);
	setThinHoloButton(wepList, BCOL_BLUE);

	m_buildUpgradeList();
	return false;
}
bool UpgradeTab::onWeapon(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	showingShips = false;
	currentSelection = -1;
	m_clear();

	shipList->setEnabled(true);
	wepList->setEnabled(false);
	setThinHoloButton(shipList, BCOL_BLUE);
	setThinHoloButton(wepList);

	m_buildUpgradeList();
	return false;
}

void UpgradeTab::m_buildBlocks()
{
	f32 blockVertRatio = (25.f / 540.f);
	f32 totalHorizRatio = (300.f / 960.f);
	u32 realEstate = totalHorizRatio * driver->getScreenSize().Width;
	u32 blockHorizSize = realEstate / availableIncrements;
	u32 blockVertSize = upgradeBlocksBg->getRelativePosition().getHeight();
	dimension2du blockSize(blockHorizSize, blockVertSize);
	for (auto& block : blocks) {
		block->remove();
	}
	blocks.clear();

	for (u32 i = 0; i < availableIncrements; ++i) {

		if (i == availableIncrements - 1) { //make sure the last one fills
			u32 used = i * blockSize.Width;
			blockSize.Width = realEstate - used;
		}

		auto block = guienv->addImage(recti(vector2di(blockHorizSize * i, 0), blockSize), upgradeBlocksBg);
		block->setImage(driver->getTexture("assets/ui/upgradeblock.png"));
		scaleAlign(block);
		f32 ratio = ((f32)i / (f32)availableIncrements);
		SColor start(255, 215, 220, 70);
		SColor end(255, 20, 255, 50);
		if (i < currentIncrements) block->setColor(end.getInterpolated(start, ratio));
		else block->setColor(SColor(200 * ratio, 200, 200, 200));
		block->setNotClipped(true);
		blocks.push_back(block);
	}
}

bool UpgradeTab::onConfirm(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;

	if (campaign->getFlag(L"MARTIN_BUILDING_CLOAK")) {
		guiController->setOkPopup("Locked", "The fabrication bay is not currently operational, owing to Martin's ongoing work with the cloaking device.");
		guiController->showOkPopup();
		return true;
	}

	if (fab->isBuilding()) {
		if (showingShips) {
			if (curCost > campaign->getSupplies()) {
				guiController->setOkPopup("No Supplies", "You don't have the supplies required to build this upgrade. Tone it down a notch!");
				guiController->showOkPopup();
				return false;
			}
			ShipUpgradeData* dat = shipUpgradeData[currentSelection];
			f32 supplies = campaign->removeSupplies(curCost);
			campaign->addNewShipUpgradeInstance(dat, curVal);
		}
		else {
			if (curCost > campaign->getSupplies()) {
				guiController->setOkPopup("No Supplies", "You don't have the supplies required to build this upgrade. Tone it down a notch!");
				guiController->showOkPopup();
				return false;
			}
			WeaponUpgradeData* dat = weaponUpgradeData[currentSelection];
			f32 supplies = campaign->removeSupplies(curCost);
			campaign->addNewWeaponUpgradeInstance(dat, curVal);
		}
	}
	else {
		if (showingShips) {
			ShipUpgradeInstance* inst = campaign->getShipUpgrade(currentSelection);
			if (!inst) return false;
			if (inst->usedBy > -1) {
				guiController->setOkPopup("In Use", "This upgrade is currently in use. \n\n Wait, what? This upgrade shouldn't be visible from this screen. That's a bug.", "Owned!");
				guiController->showOkPopup();
				return false;
			}
			ShipUpgradeData* dat = shipUpgradeData[inst->dataId];
			campaign->addSupplies(dat->baseCost);
			campaign->removeShipUpgrade(inst);
			delete inst;
		}
		else {
			WeaponUpgradeInstance* inst = campaign->getWeaponUpgrade(currentSelection);
			if (!inst) return false;
			if (inst->usedBy > -1) {
				guiController->setOkPopup("In Use", "This upgrade is currently in use. \n\n Wait, what? This upgrade shouldn't be visible from this screen. That's a bug.", "Owned!");
				guiController->showOkPopup();
				return false;
			}
			WeaponUpgradeData* dat = weaponUpgradeData[inst->dataId];
			campaign->addSupplies(dat->baseCost);
			campaign->removeWeaponUpgrade(inst);
			delete inst;
		}
		m_clear();
		m_buildUpgradeList();
	}
	m_showSupplies();
	return false;
}

static void _strReplace(std::string& init, std::string const& find, std::string const& replace)
{
	std::size_t pos = init.find(find);

	if (pos == std::string::npos) return;
	init.replace(pos, init.length(), replace);
}

bool UpgradeTab::onUpgradeScrapSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (id == currentSelection) {
		m_clear();
		return false;
	}
	currentSelection = id;
	confirm->setVisible(true);
	costDisplay->setVisible(true);
	valDisplay->setVisible(true);
	flavorText->setVisible(true);
	upgradeBlocksBg->setVisible(true);
	if (showingShips) {
		auto inst = campaign->getShipUpgrade(currentSelection);
		auto dat = shipUpgradeData[inst->dataId];
		//set values
		std::string scrapval = "Scrap value: " + fprecis(dat->baseCost, 5);
		std::string desc = dat->upgradeDescription;

		_strReplace(desc, "{number}", fprecis(inst->value, 5));
		costDisplay->setText(wstr(scrapval).c_str());
		valDisplay->setText(wstr(desc).c_str());
		flavorText->setText(wstr(dat->description).c_str());
		curVal = inst->value;

		availableIncrements = ((dat->maxValue - dat->baseValue) / dat->scaleValue) + 1;
		u32 increments = ((dat->maxValue - inst->value) / dat->scaleValue);
		currentIncrements = 1 + increments;
	}
	else {
		auto inst = campaign->getWeaponUpgrade(currentSelection);
		auto dat = weaponUpgradeData[inst->dataId];
		//set values
		std::string scrapval = "Scrap value: " + fprecis(dat->baseCost, 5);
		std::string desc = dat->upgradeDescription;
		_strReplace(desc, "{number}", fprecis(inst->value, 5));
		costDisplay->setText(wstr(scrapval).c_str());
		valDisplay->setText(wstr(desc).c_str());
		flavorText->setText(wstr(dat->description).c_str());
		curVal = inst->value;

		availableIncrements = ((dat->maxValue - dat->baseValue) / dat->scaleValue) + 1;
		u32 increments = ((dat->maxValue - inst->value) / dat->scaleValue);
		currentIncrements = 1 + increments;
	}
	m_buildBlocks();
	return false;
}

bool UpgradeTab::onUpgradeBuildSelect(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	s32 id = event.GUIEvent.Caller->getID();
	if (id == currentSelection) {
		m_clear();
		return false;
	}
	currentSelection = id;
	confirm->setVisible(true);
	decrease->setVisible(true);
	increase->setVisible(true);
	costDisplay->setVisible(true);
	valDisplay->setVisible(true);
	flavorText->setVisible(true);
	upgradeBlocksBg->setVisible(true);
	if (showingShips) {
		auto dat = shipUpgradeData[currentSelection];
		std::string str = "Cost: " + fprecis(dat->baseCost, 5);
		std::string desc = dat->upgradeDescription;
		curVal = dat->baseValue;
		curCost = dat->baseCost;
		costDisplay->setText(wstr(str).c_str());
		_strReplace(desc, "{number}", fprecis(curVal, 5));
		valDisplay->setText(wstr(desc).c_str());
		flavorText->setText(wstr(dat->description).c_str());

		availableIncrements = ((dat->maxValue - dat->baseValue) / dat->scaleValue) + 1;
		currentIncrements = 1;
	}
	else {
		auto dat = weaponUpgradeData[currentSelection];
		std::string str = "Cost: " + fprecis(dat->baseCost, 5);
		std::string desc = dat->upgradeDescription;
		curVal = dat->baseValue;
		curCost = dat->baseCost;
		costDisplay->setText(wstr(str).c_str());
		_strReplace(desc, "{number}", fprecis(curVal, 5));
		valDisplay->setText(wstr(desc).c_str());
		flavorText->setText(wstr(dat->description).c_str());

		availableIncrements = ((dat->maxValue - dat->baseValue) / dat->scaleValue) + 1;
		currentIncrements = 1;
	}
	m_buildBlocks();
	return false;
}

bool UpgradeTab::onIncrement(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	std::string desc = "";
	if (showingShips) {
		auto dat = shipUpgradeData[currentSelection];
		if ((curVal + dat->scaleValue) > dat->maxValue) {
			audioDriver->playMenuSound("menu_error.ogg");
			return false;
		}
		curVal += dat->scaleValue;
		curCost += dat->scaleCost;
		desc = dat->upgradeDescription;
		currentIncrements += 1;
	}
	else {
		auto dat = weaponUpgradeData[currentSelection];
		if ((curVal + dat->scaleValue) > dat->maxValue) {
			audioDriver->playMenuSound("menu_error.ogg");
			return false;
		}
		curVal += dat->scaleValue;
		curCost += dat->scaleCost;
		desc = dat->upgradeDescription;
		currentIncrements += 1;
	}
	std::string str = "Cost: " + fprecis(curCost, 5);
	costDisplay->setText(wstr(str).c_str());
	_strReplace(desc, "{number}", fprecis(curVal, 5));
	valDisplay->setText(wstr(desc).c_str());
	m_buildBlocks();
	return false;
}
bool UpgradeTab::onDecrement(const SEvent& event)
{
	if (event.GUIEvent.EventType != EGET_BUTTON_CLICKED) return true;
	std::string desc = "";
	if (showingShips) {
		auto dat = shipUpgradeData[currentSelection];
		if ((curVal - dat->scaleValue) < dat->baseValue) {
			audioDriver->playMenuSound("menu_error.ogg");
			return false;
		}
		curVal -= dat->scaleValue;
		curCost -= dat->scaleCost;
		desc = dat->upgradeDescription;
		currentIncrements -= 1;
	}
	else {
		auto dat = weaponUpgradeData[currentSelection];
		if ((curVal - dat->scaleValue) < dat->baseValue) {
			audioDriver->playMenuSound("menu_error.ogg");
			return false;
		}
		curVal -= dat->scaleValue;
		curCost -= dat->scaleCost;
		desc = dat->upgradeDescription;
		currentIncrements -= 1;
	}
	std::string str = "Cost: " + fprecis(curCost, 5);
	costDisplay->setText(wstr(str).c_str());
	_strReplace(desc, "{number}", fprecis(curVal, 5));
	valDisplay->setText(wstr(desc).c_str());
	m_buildBlocks();
	return false;
}

void UpgradeTab::m_buildUpgradeList()
{
	m_clearList();
	if (fab->isBuilding()) {
		if (showingShips) m_buildableShipUpgradesList();
		else m_buildableWepUpgradesList();
	}
	else {
		if (showingShips) m_buildShipUpgradeList(std::bind(&UpgradeTab::onUpgradeScrapSelect, this, std::placeholders::_1), true, false, BCOL_RED);
		else m_buildWepUpgradeList(std::bind(&UpgradeTab::onUpgradeScrapSelect, this, std::placeholders::_1), true, false, BCOL_RED);
	}
}

void UpgradeTab::m_buildableShipUpgradesList()
{
	u32 count = 0;
	for (auto [id, up] : shipUpgradeData) {
		if (!up->canBuild) continue;
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, id, wstr(up->name).c_str(), L"Select this ship upgrade to build.");
		setThinHoloButton(button, BCOL_GREEN);
		guiController->setCallback(button, std::bind(&UpgradeTab::onUpgradeBuildSelect, this, std::placeholders::_1), which);
		listItems.push_back(button);
	}
}
void UpgradeTab::m_buildableWepUpgradesList()
{
	u32 count = 0;
	for (auto [id, up] : weaponUpgradeData) {
		if (!up->canBuild) continue;
		position2di pos = m_listItemStart();
		pos.Y += m_listItemHeightDiff() * count;
		++count;
		auto button = guienv->addButton(rect<s32>(pos, m_listItemSize()), listBg, id, wstr(up->name).c_str(), L"Select this weapon upgrade to build.");
		setThinHoloButton(button, BCOL_GREEN);
		guiController->setCallback(button, std::bind(&UpgradeTab::onUpgradeBuildSelect, this, std::placeholders::_1), which);
		listItems.push_back(button);
	}
}