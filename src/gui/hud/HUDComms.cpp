#include "HUDComms.h"
#include "GameController.h"
#include "AudioDriver.h"
#include "GameAssets.h"
#include "SensorComponent.h"
#include "AIComponent.h"
#include "Config.h"
#include "HealthComponent.h"
#include "GameFunctions.h"
#include "PlayerComponent.h"

HUDComms::HUDComms(IGUIElement* rootHUD, IGUIElement* rootPanel) : HUDElement(rootHUD)
{
	auto panelRect = rootPanel->getAbsolutePosition();
	panel = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), dimension2du(panelRect.getWidth(), panelRect.getHeight())), false, true, rootPanel);
	setHUDText(panel);
	panel->setVisible(true);
	dummy = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), dimension2du(panelRect.getWidth(), panelRect.getHeight())), false, true, rootPanel);
	dummy->setVisible(false);

	u32 totalheight = (panelRect.getHeight() / 5);
	s32 height = (totalheight) / 5;
	dimension2du sliceSize(panelRect.getWidth() * (4.f/5.f), height);
	openCommsReminder = guienv->addStaticText((L"Open Comms: " + getKeyDesc(IN_OPEN_COMMS)).c_str(), 
		rect<s32>(position2di(panelRect.getWidth() * (.5f / 5.f), -height), sliceSize), false, true, rootPanel);
	setHUDTextSmall(openCommsReminder);
	openCommsReminder->setNotClipped(true);
	openCommsReminder->setVisible(true);

	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		wingmenHP[i] = nullptr;
		wingmenSP[i] = nullptr;
		wingmenNames[i] = nullptr;
		if (gameController->getWingman(i) == INVALID_ENTITY) continue;
		position2di zero(0, 0);
		u32 wingheight = ((totalheight * i) + totalheight);
		position2di txtPos(panelRect.getWidth() * (.5f/5.f), wingheight);
		position2di spPos(panelRect.getWidth()* (.5f / 5.f), height + wingheight);
		position2di hpPos(panelRect.getWidth() * (.5f / 5.f), (height * 2) + wingheight);
		auto ai = gameController->getWingman(i).get<AIComponent>();
		std::string aiNameStr;
		if (ai) aiNameStr = ai->AIName;
		std::wstring namestr = wstr(aiNameStr);
		IGUIStaticText* name = guienv->addStaticText(namestr.c_str(), rect<s32>(txtPos, sliceSize), false, true, dummy);
		setHUDTextSmall(name);
		wingmenNames[i] = name;
		IGUIImage* bg = guienv->addImage(rect<s32>(spPos, sliceSize), dummy);
		scaleAlign(bg);
		bg->setImage(assets->getTexture("assets/hud/bar_health-shields.png"));
		IGUIImage* lazy = guienv->addImage(rect<s32>(zero, sliceSize), bg);
		scaleAlign(lazy);
		lazy->setImage(assets->getTexture("assets/hud/bar_flash.png"));
		IGUIImage* fill = guienv->addImage(rect<s32>(zero, sliceSize), bg);
		scaleAlign(fill);
		fill->setImage(assets->getTexture("assets/hud/bar_fill_shields.png"));
		wingmenSP[i] = new FillBar(bg, fill, BARFILLDIR::RIGHT, namestr, lazy);

		bg = guienv->addImage(rect<s32>(hpPos, sliceSize), dummy);
		scaleAlign(bg);
		bg->setImage(assets->getTexture("assets/hud/bar_health-shields.png"));
		lazy = guienv->addImage(rect<s32>(zero, sliceSize), bg);
		scaleAlign(lazy);
		lazy->setImage(assets->getTexture("assets/hud/bar_flash.png"));
		fill = guienv->addImage(rect<s32>(zero, sliceSize), bg);
		scaleAlign(fill);
		fill->setImage(assets->getTexture("assets/hud/bar_fill_health.png"));
		wingmenHP[i] = new FillBar(bg, fill, BARFILLDIR::RIGHT, namestr, lazy);
	}
}

HUDComms::~HUDComms()
{
	if (panel) panel->remove();
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		if (wingmenNames[i]) wingmenNames[i]->remove();
		if (wingmenHP[i]) delete wingmenHP[i];
		if (wingmenSP[i]) delete wingmenSP[i];
	}
}

void HUDComms::updateElement(flecs::entity playerId)
{
	auto sns = playerId.get<SensorComponent>();
	auto in = playerId.get<InputComponent>();
	auto player = playerId.get_mut<PlayerComponent>();
	if (commState == COMMSTATE::ORDER_SPECIFIC_UNIT && !selectedEntity.is_alive()) {
		commState = COMMSTATE::ROOT;
		selectedEntity = INVALID_ENTITY;
		m_displayContactList(sns);
	}
	if (selectedEntity != INVALID_ENTITY && commState == COMMSTATE::ORDER_SPECIFIC_UNIT) {
		bool found = false;
		for (auto& ctct : sns->nearbyOrderableContacts) { //check if its still in range
			if (ctct.ent == selectedEntity) {
				found = true;
				break;
			}
		}
		if (!found) {
			commState = COMMSTATE::ROOT;
			selectedEntity = INVALID_ENTITY;
			m_displayContactList(sns);
		}
	}

	switch (commState) {
	case COMMSTATE::ROOT: {
		m_handleWingmenHpDisplay(in);
		break;
	}
	case COMMSTATE::DISPLAY_ALL_CONTACTS: {
		m_displayContactList(sns);
		m_handleContactSelect(in, player, sns);
		break;
	}
	case COMMSTATE::ORDER_ALL_UNITS: {
		m_handleAllOrderableUnitsOrder(in, player, sns);
		break;
	}
	case COMMSTATE::ORDER_ALL_WINGMEN: {
		m_handleAllWingmenOrder(in, player, sns);
		break;
	}
	case COMMSTATE::ORDER_SPECIFIC_UNIT: {
		m_handleTargetOrder(in, player, sns);
		break;
	}
	default:
		break;
	}
}

void HUDComms::m_displayOrderList()
{
	std::wstring display = L"";
	display += getKeyDesc(IN_COMMS_1) + L": Attack My Target\n";
	display += getKeyDesc(IN_COMMS_2) + L": Form Up\n";
	display += getKeyDesc(IN_COMMS_3) + L": Halt Movement\n";
	display += getKeyDesc(IN_COMMS_4) + L": Dock With Target\n";
	display += getKeyDesc(IN_COMMS_5) + L": Help Me!\n";
	display += getKeyDesc(IN_COMMS_6) + L": Return To Ship\n";
	display += getKeyDesc(IN_COMMS_7) + L": Cancel\n";
	panel->setText(display.c_str());
}

void HUDComms::m_handleWingmenHpDisplay(const InputComponent* in)
{
	dummy->setVisible(true);
	panel->setVisible(false);
	openCommsReminder->setVisible(true);
	if (in->isKeyDown(IN_OPEN_COMMS)) {
		commState = COMMSTATE::DISPLAY_ALL_CONTACTS;
		audioDriver->playMenuSound("menu_cancel.ogg");
		return;
	}
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		if (!gameController->getWingman(i).is_alive()) {
			if (wingmenHP[i]) wingmenHP[i]->togVis(false);
			if (wingmenSP[i]) wingmenSP[i]->togVis(false);
			if (wingmenNames[i]) wingmenNames[i]->setVisible(false);
			continue;
		}
		auto wingmanHP = gameController->getWingman(i).get<HealthComponent>();
		if (wingmanHP) {
			wingmenHP[i]->updateBar(wingmanHP->health, wingmanHP->maxHealth);
			wingmenSP[i]->updateBar(wingmanHP->shields, wingmanHP->maxShields);
		}
	}
}

void HUDComms::m_displayContactList(const SensorComponent* sns)
{
	panel->setVisible(true);
	dummy->setVisible(false);
	openCommsReminder->setVisible(false);

	std::wstring display = getKeyDesc(IN_COMMS_1) + L": Order All Wingmen\n" + getKeyDesc(IN_COMMS_2) + L": Order All\n";
	for (u32 i = 0; i < sns->nearbyOrderableContacts.size(); ++i) {
		if (i == 5) break;
		display += getKeyDesc((INPUT)(IN_COMMS_3 + i)) + L": ";
		auto& ctct = sns->nearbyOrderableContacts[i];
		auto ai = ctct.ent.get<AIComponent>();
		if (!ai) continue;
		display += wstr(ai->AIName) + L"\n";
	}
	panel->setText(display.c_str());
}

void HUDComms::m_handleContactSelect(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns)
{
	if (!in->commsInput() || player->timeSinceLastOrder < .4f) return;
	if (in->isKeyDown(IN_COMMS_2)) {
		commState = COMMSTATE::ORDER_ALL_UNITS;
	}
	else if (in->isKeyDown(IN_COMMS_1)) {
		commState = COMMSTATE::ORDER_ALL_WINGMEN;
	}
	else {
		commState = COMMSTATE::ORDER_SPECIFIC_UNIT;
		u32 which = in->whichCommsInput();
		which -= IN_COMMS_3; //get a number 0 through 4
		selectedEntity = sns->nearbyOrderableContacts[which].ent;
	}
	audioDriver->playMenuSound("menu_cancel.ogg");
	player->timeSinceLastOrder = 0;
	m_displayOrderList();
}
void HUDComms::m_handleTargetOrder(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns)
{
	if (!in->commsInput() || player->timeSinceLastOrder < .4f) return;
	if (in->whichCommsInput() == IN_COMMS_7) {
		commState = COMMSTATE::ROOT;
		m_displayContactList(sns);
		return;
	}
	auto ai = selectedEntity.get_mut<AIComponent>();
	ORDER_TYPE ord = (ORDER_TYPE)(in->whichCommsInput() - IN_COMMS_1);
	ai->registerOrder(ord, gameController->getPlayer(), (ord == ORDER_TYPE::ORD_HONK_HELP) ? player->beingTargetedBy : sns->targetContact);
	player->timeSinceLastOrder = 0;
	commState = COMMSTATE::ROOT;
	m_displayContactList(sns);
}

void HUDComms::m_handleAllWingmenOrder(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns)
{
	if (!in->commsInput() || player->timeSinceLastOrder < .4f) return;
	if (in->whichCommsInput() == IN_COMMS_7) {
		commState = COMMSTATE::ROOT;
		m_displayContactList(sns);
		return;
	}
	ORDER_TYPE ord = (ORDER_TYPE)(in->whichCommsInput() - IN_COMMS_1);
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		auto man = gameController->getWingman(i);
		if (man == INVALID_ENTITY || !man.is_alive()) continue;
		auto ai = man.get_mut<AIComponent>();
		ai->registerOrder(ord, gameController->getPlayer(), (ord == ORDER_TYPE::ORD_HONK_HELP) ? player->beingTargetedBy : sns->targetContact);
	}
	player->timeSinceLastOrder = 0;
	commState = COMMSTATE::ROOT;
	m_displayContactList(sns);
}
void HUDComms::m_handleAllOrderableUnitsOrder(const InputComponent* in, PlayerComponent* player, const SensorComponent* sns)
{
	if (!in->commsInput() || player->timeSinceLastOrder < .4f) return;
	if (in->whichCommsInput() == IN_COMMS_7) {
		commState = COMMSTATE::ROOT;
		m_displayContactList(sns);

		return;
	}
	ORDER_TYPE ord = (ORDER_TYPE)(in->whichCommsInput() - IN_COMMS_1);

	for (u32 i = 0; i < sns->nearbyOrderableContacts.size(); ++i) {
		auto man = sns->nearbyOrderableContacts[i].ent;
		if (man == INVALID_ENTITY || !man.is_alive()) continue;
		auto ai = man.get_mut<AIComponent>();
		ai->registerOrder(ord, gameController->getPlayer(), (ord == ORDER_TYPE::ORD_HONK_HELP) ? player->beingTargetedBy : sns->targetContact);
	}
	player->timeSinceLastOrder = 0;
	commState = COMMSTATE::ROOT;
	m_displayContactList(sns);


}