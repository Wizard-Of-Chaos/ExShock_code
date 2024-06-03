#include "Campaign.h"
#include "GameStateController.h"
#include "WeaponUpgrades.h"
#include "ShipUpgrades.h"
#include "LoadoutData.h"
#include "GuiController.h"
#include "AudioDriver.h"
#include "AttributeReaders.h"
#include <filesystem>
#include "CrashLogger.h"

const s32 STARTER_WEP = 3;
const u32 MAX_ENCOUNTERS = 8;
const u32 START_AMMO = 20;
const f32 START_SUPPLIES = 225.f;

void Campaign::addAmmo(s32 amt)
{
	ammunition += amt;
}
s32 Campaign::removeAmmo(s32 amt)
{
	ammunition -= amt;
	if (ammunition < 0) {
		u32 ret = amt + ammunition;
		ammunition = 0;
		return ret;
	}
	return amt;
}
void Campaign::addSupplies(f32 amt)
{
	supplies += amt;
}
f32 Campaign::removeSupplies(f32 amt)
{
	supplies -= amt;
	if (supplies < 0) {
		f32 ret = amt + supplies;
		supplies = 0;
		return ret;
	}
	return amt;
}

ShipInstance* Campaign::createNewShipInstance(u32 id, bool templateShip)
{
	ShipInstance* ship = new ShipInstance;
	ship->ship = shipData[id]->ship;
	ship->hp = shipData[id]->hp;
	ship->hards = shipData[id]->hards;

	if (!templateShip) {
		++shipCount;
		ship->id = shipCount;
	}

	for (u32 i = 0; i < MAX_HARDPOINTS; ++i) {
		ship->weps[i] = -1;
	}
	ship->physWep = -1;
	return ship;
}

WeaponInstance* Campaign::createNewWeaponInstance(WeaponInfoComponent wep, bool templateWep)
{
	if(wep.wepDataId != 0 && !templateWep) ++wepCount;
	WeaponInstance* ret = new WeaponInstance;
	if (wep.wepDataId == 0 || templateWep) {
		ret->id = 0;
	}
	else {
		ret->id = wepCount;
	}
	ret->wep = wep;
	ret->fire.ammunition = wep.maxAmmunition;
	if (templateWep) return ret;
	//if its not a template then add it to inventory
	if (wep.hrdtype == HRDP_PHYSICS) m_physWeapons.push_back(ret);
	else if (wep.hrdtype == HRDP_HEAVY) m_heavyWeapons.push_back(ret);
	else m_weapons.push_back(ret);
	return ret;
}

WeaponInstance* Campaign::createRandomWeaponInstance()
{
	u32 id = random.unumEx((unsigned int)weaponData.size());
	WeaponData* dat = weaponData[id];
	while (!dat->canLoot) {
		id = random.unumEx((unsigned int)weaponData.size());
		dat = weaponData[id];
	}
	if (weaponData[id]) return createNewWeaponInstance(weaponData[id]->wepComp);

	return createNewWeaponInstance(weaponData[3]->wepComp);
}

ShipInstance* Campaign::buildShipInstanceFromData(ShipData* data)
{
	auto inst = createNewShipInstance(data->id);
	inst->ship = data->ship;
	inst->hards = data->hards;
	return inst;
}

ShipInstance* Campaign::buildStarterShip()
{
	auto inst = buildShipInstanceFromData(shipData[0]);
	inst->weps[0] = createNewWeaponInstance(weaponData[STARTER_WEP]->wepComp)->id;
	inst->weps[1] = createNewWeaponInstance(weaponData[STARTER_WEP]->wepComp)->id;
	inst->physWep = createNewWeaponInstance(physWeaponData[2]->wepComp)->id;
	inst->heavyWep = createNewWeaponInstance(heavyWeaponData[1]->wepComp)->id;

	getWeapon(inst->weps[0])->usedBy = inst->id;
	getWeapon(inst->weps[1])->usedBy = inst->id;
	getPhysWeapon(inst->physWep)->usedBy = inst->id;
	getHeavyWeapon(inst->heavyWep)->usedBy = inst->id;

	return inst;
}

ShipInstance* Campaign::createRandomShipInstance()
{
	u32 shipnum = 0;
	ShipData* ship=nullptr;
	while (!ship) {
		shipnum = random.unumEx((unsigned int)shipData.size());
		ship = shipData[shipnum];
	}
	while (!ship->canLoot) {
		shipnum = random.unumEx((unsigned int)shipData.size());
		ship = shipData[shipnum];
	}
	auto inst = createNewShipInstance(shipnum);
	buildShipInstanceFromData(ship);
	return inst;
}

bool Campaign::addShipInstanceToHangar(ShipInstance* inst)
{
	if (getShip(inst->id)) return false;
	m_ships.push_back(inst);
	return true;
}

bool Campaign::addWeapon(WeaponInstance* inst)
{
	if (inst->wep.hrdtype == HRDP_PHYSICS) {
		if (getPhysWeapon(inst->id)) return false;
		m_physWeapons.push_back(inst);
	}
	else if (inst->wep.hrdtype == HRDP_HEAVY) {
		if (getHeavyWeapon(inst->id)) return false;
		m_heavyWeapons.push_back(inst);
	}
	else{
		if (getWeapon(inst->id)) return false;
		m_weapons.push_back(inst);
	}
	return true;
}

bool Campaign::removeShipInstance(ShipInstance* inst)
{
	auto it = m_ships.begin();
	while (it != m_ships.end()) {
		if (inst->id == (*it)->id) {
			it = m_ships.erase(it);
			return true;
		}
		++it;
	}
	return false;
}

bool Campaign::destroyShip(ShipInstance* inst)
{
	if (!inst) return true;

	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		if (assignedShips[i] == inst->id) assignedShips[i] = -1;
	}

	if (inst->inUseBy >= 0) {
		auto man = getWingman(inst->inUseBy);
		man->assigned = false;
		man->assignedShip = -1;
	}

	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		if (inst->upgrades[i] >= 0) {
			auto sup = getShipUpgrade(inst->upgrades[i]);
			removeShipUpgrade(sup);
			delete sup;
		}
	}

	for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
		if (inst->weps[i] >= 0) {
			auto wep = getWeapon(inst->weps[i]);
			removeWeapon(wep);
			for (u32 j = 0; j < MAX_WEP_UPGRADES; ++j) {
				if (wep->upgrades[j] >= 0) {
					auto wup = getWeaponUpgrade(wep->upgrades[j]);
					removeWeaponUpgrade(wup);
					delete wup;
				}
			}
			delete wep;
		}
	}
	if (inst->heavyWep >= 0) {
		auto wep = getHeavyWeapon(inst->heavyWep);
		removeWeapon(wep);
		for (u32 j = 0; j < MAX_WEP_UPGRADES; ++j) {
			if (wep->upgrades[j] >= 0) {
				auto wup = getWeaponUpgrade(wep->upgrades[j]);
				removeWeaponUpgrade(wup);
				delete wup;
			}
		}
		delete wep;
	}
	if (inst->physWep >= 0) {
		auto wep = getPhysWeapon(inst->physWep);
		removeWeapon(wep);
		for (u32 j = 0; j < MAX_WEP_UPGRADES; ++j) {
			if (wep->upgrades[j] >= 0) {
				auto wup = getWeaponUpgrade(wep->upgrades[j]);
				removeWeaponUpgrade(wup);
				delete wup;
			}
		}
		delete wep;
	}
	removeShipInstance(inst);
	delete inst;
	return true;
}

bool Campaign::removeWeapon(WeaponInstance* inst)
{
	if (!inst) return true;
	if (inst->wep.hrdtype == HRDP_PHYSICS) {
		auto it = m_physWeapons.begin();
		while (it != m_physWeapons.end()) {
			if (inst->id == (*it)->id) {
				it = m_physWeapons.erase(it);
				return true;
			}
			++it;
		}
	}
	else if (inst->wep.hrdtype == HRDP_HEAVY) {
		auto it = m_heavyWeapons.begin();
		while (it != m_heavyWeapons.end()) {
			if (inst->id == (*it)->id) {
				it = m_heavyWeapons.erase(it);
				return true;
			}
			++it;
		}
	}
	else {
		auto it = m_weapons.begin();
		while (it != m_weapons.end()) {
			if (inst->id == (*it)->id) {
				it = m_weapons.erase(it);
				return true;
			}
			++it;
		}
	}
	return false;
}

ShipInstance* Campaign::getShip(instId id)
{
	for (auto val : m_ships) {
		if (val->id == id) return val;
	}
	return nullptr;
}

WingmanInstance* Campaign::getWingman(instId id)
{
	for (auto val : m_wingmen) {
		if (val->id == id) return val;
	}
	return nullptr;
}

WeaponInstance* Campaign::getWeapon(instId id)
{
	for (auto val : m_weapons) {
		if (val->id == id) return val;
	}
	for (auto val : m_physWeapons) {
		if (val->id == id) return val;
	}
	for (auto val : m_heavyWeapons) {
		if (val->id == id) return val;
	}
	return nullptr;
}
WeaponInstance* Campaign::getPhysWeapon(instId id)
{
	for (auto val : m_physWeapons) {
		if (val->id == id) return val;
	}
	return nullptr;
}
WeaponInstance* Campaign::getHeavyWeapon(instId id)
{
	for (auto val : m_heavyWeapons) {
		if (val->id == id) return val;
	}
	return nullptr;
}

WingmanInstance* Campaign::getAssignedWingman(u32 pos)
{
	if (pos == -1) return getWingman(0);
	return getWingman(assignedWing[pos]);
}
void Campaign::setAssignedWingman(WingmanInstance* man, u32 pos)
{
	if (pos == -1) return; //don't remove the player
	if (getAssignedWingman(pos)) removeAssignedWingman(pos);

	assignedWing[pos] = man->id;
	man->assigned = true;
	if (man->id == 5) {
		assignedShips[pos] = man->assignedShip;
	}
}
void Campaign::removeAssignedWingman(u32 pos)
{
	if (pos == -1) return; //don't remove the player
	auto man = getWingman(assignedWing[pos]);

	if (assignedWing[pos] > -1) {
		if(man->id !=5) man->assignedShip = -1;
		man->assigned = false;
	}
	assignedWing[pos] = -1;
	if (man->id != 5) {
		if (assignedShips[pos] > -1) {
			getShip(assignedShips[pos])->inUseBy = -1;
		}
	}
	assignedShips[pos] = -1;
}
ShipInstance* Campaign::getAssignedShip(u32 pos)
{
	if (pos == -1) return getShip(playerShip);
	return getShip(assignedShips[pos]);
}
void Campaign::setAssignedShip(ShipInstance* inst, u32 pos)
{
	if (getAssignedShip(pos)) removeAssignedShip(pos);
	if (pos == -1) {
		playerShip = inst->id;
		inst->inUseBy = 0;
		return;
	}
	assignedShips[pos] = inst->id;
	inst->inUseBy = getAssignedWingman(pos)->id;
}
void Campaign::removeAssignedShip(u32 pos)
{
	if (pos == -1) {
		if (playerShip > -1) getShip(playerShip)->inUseBy = -1;
		playerShip = -1;
		return;
	}
	if (assignedShips[pos] > -1) {
		getShip(assignedShips[pos])->inUseBy = -1;
	}
	assignedShips[pos] = -1;
}

bool Campaign::assignWingmanToShip(WingmanInstance* wingman, ShipInstance* ship)
{
	if (!wingman || !ship) return false;
	if (wingman->assignedShip >= 0) {
		getShip(wingman->assignedShip)->inUseBy = -1;
		wingman->assignedShip = -1;
	}
	ship->inUseBy = wingman->id;
	wingman->assignedShip = ship->id;
	if(wingman->id != 5) wingman->assigned = true;

	return true;
}

void Campaign::addWingman(WingmanInstance* inst)
{
	m_wingmen.push_back(inst);
	m_allSpeakers.push_back(wstr(inst->name));
}

void Campaign::stripShip(ShipInstance* inst)
{
	if (!inst) return;
	if (inst->physWep >= 0) campaign->getWeapon(inst->physWep)->usedBy = -1;
	inst->physWep = -1;
	if (inst->heavyWep >= 0) campaign->getWeapon(inst->heavyWep)->usedBy = -1;
	inst->heavyWep = -1;
	for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
		if (inst->weps[i] > -1) campaign->getWeapon(inst->weps[i])->usedBy = -1;
		inst->weps[i] = -1;
	}
	for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
		if (inst->upgrades[i] > -1) campaign->getShipUpgrade(inst->upgrades[i])->usedBy = -1;
		inst->upgrades[i] = -1;
	}
}

void Campaign::stripWeapon(WeaponInstance* inst)
{
	if (!inst) return;
	for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
		auto id = inst->upgrades[i];
		auto upgrade = campaign->getWeaponUpgrade(id);
		if (upgrade) upgrade->usedBy = INVALID_DATA_ID;
		inst->upgrades[i] = INVALID_DATA_ID;
	}
}

void Campaign::stripAllUnassignedShips()
{
	for (auto& ship : m_ships) {
		if (ship->inUseBy != INVALID_DATA_ID) continue; //don't strip things in use
		stripShip(ship);
	}
}

void Campaign::stripAllUnassignedWeapons()
{
	for (auto& wep : m_weapons) {
		if (wep->usedBy != INVALID_DATA_ID) continue;
		stripWeapon(wep);
	}
}

void Campaign::m_buildNextSector()
{
	SECTOR_TYPE curType = currentSector->getType();
	if(currentSector) delete currentSector;
	u32 next = (u32)curType;
	++next;
	m_buildSector((SECTOR_TYPE)next);
	currentSector->buildScenarios();
}

void Campaign::m_buildSector(SECTOR_TYPE type)
{
	switch (type) {
	case SECTOR_DEBRIS:
		currentSector = new DebrisSector;
		break;
	case SECTOR_ASTEROID:
		currentSector = new AsteroidSector;
		break;
	case SECTOR_GAS:
		currentSector = new GasSector;
		break;
	case SECTOR_SUPPLY_DEPOT:
		currentSector = new SupplyDepotSector;
		break;
	case SECTOR_RINGS:
		currentSector = new RingSector;
		break;
	case SECTOR_FLEET_GROUP:
		currentSector = new FleetSector;
		break;
	case SECTOR_FINALE:
		currentSector = new CometSector;
		break;
	default:
		currentSector = new DebrisSector;
		break;
	}
}

void Campaign::returnToCampaign()
{
	if (getSector()->sectorComplete()) {
		m_buildNextSector();
	}
	++currentDifficulty;
	getRandomCharacterDialogues();
	currentSector->deselectCurrentScenario();
	audioDriver->playMusic(getSector()->menuMusic);
	audioDriver->setMusicGain(0, 1.f);
	audioDriver->stopMusic(1);
}

bool Campaign::isDialogueAvailable(DialogueTree& tree)
{
	if (tree.isUsed()) return false; //if its been used throw it out
	if (tree.minSector() > currentSector->getType()) return false; //if it's too advanced throw it out
	bool found = false;
	for (auto& speaker : tree.speakers()) {
		found = false;
		for (auto& s : m_allSpeakers) {
			if (s == speaker) {
				found = true;
				break;
			}
		}
		if (!found) break;
	}
	if (!found) return false; //if all the speakers are not present throw it out
	bool prereqsDone = true;
	for (auto& tree : tree.requiredTrees()) {
		if (!isTreeUsed(tree)) {
			prereqsDone = false;
			break;
		}
	}
	for (auto& flag : tree.requiredFlags()) {
		if (!getFlag(flag)) {
			prereqsDone = false;
			break;
		}
	}
	if (!prereqsDone) return false; //if it doesnt have the prereqs throw it out

	return true;
}

void Campaign::getRandomCharacterDialogues()
{
	m_availableCharacterDialogues.clear();
	u32 count = 0;
	auto it = m_allCharacterDialogues.begin();
	std::vector<std::wstring> usedSpeakers;
	usedSpeakers.clear();
	std::vector<std::wstring> possibleDialogues;
	possibleDialogues.clear();
	u32 maxMisses = 150;
	u32 misses = 0;
	for (auto& [id, tree] : m_allCharacterDialogues) {
		if (!isDialogueAvailable(tree)) continue;
		bool speakersUsed = false;
		for (auto& spkr : usedSpeakers) {
			for (auto& treespkr : tree.speakers()) {
				if (spkr == treespkr) {
					speakersUsed = true;
					break;
				}
			}
			if (speakersUsed) break;
		}
		if (speakersUsed) continue;
		//speakers are available, dialogue is available
		possibleDialogues.push_back(id);
		for (auto& spkr : tree.speakers()) usedSpeakers.push_back(spkr);
	}
	//now have a list of possible dialogues
	if (possibleDialogues.empty()) {
		baedsLogger::log("No dialogues available.\n");
		return;
	}

	if (possibleDialogues.size() == 1) {
		baedsLogger::log("Single dialogue available:" + wstrToStr(possibleDialogues.front()) + "\n");
		m_availableCharacterDialogues.push_back(m_allCharacterDialogues[possibleDialogues.front()]);
		return;
	}

	if (possibleDialogues.size() == 2) {
		baedsLogger::log("Two dialogues available:" + wstrToStr(possibleDialogues.front()) + ", " + wstrToStr(possibleDialogues.back()) + "\n");

		m_availableCharacterDialogues.push_back(m_allCharacterDialogues[possibleDialogues.front()]);
		m_availableCharacterDialogues.push_back(m_allCharacterDialogues[possibleDialogues.back()]);
		return;
	}
	baedsLogger::log(std::to_string(possibleDialogues.size()) + " dialogues available. Rolls: ");
	u32 roll = random.unumEx((unsigned int)possibleDialogues.size());
	std::wstring first = possibleDialogues[roll];
	possibleDialogues.erase(possibleDialogues.begin() + roll);
	roll = random.unumEx((unsigned int)possibleDialogues.size());
	std::wstring second = possibleDialogues[roll];

	baedsLogger::log(wstrToStr(first) + ", " + wstrToStr(second) + "\n");

	m_availableCharacterDialogues.push_back(m_allCharacterDialogues[first]);
	m_availableCharacterDialogues.push_back(m_allCharacterDialogues[second]);

}

bool Campaign::isTreeUsed(std::wstring id)
{
	if (m_allCharacterDialogues.find(id) == m_allCharacterDialogues.end()) return true;

	return m_allCharacterDialogues[id].isUsed();
}

const std::vector<DialogueTree>& Campaign::getCurAvailableCharacterDialogues()
{
	return m_availableCharacterDialogues;
}

void Campaign::setDialogueTreeUsed(std::wstring id, bool used)
{
	if (m_allCharacterDialogues.find(id) == m_allCharacterDialogues.end()) return;
	m_allCharacterDialogues[id].setUsed();
}

DialogueTree Campaign::getCharacterDialogue(std::wstring id)
{
	return DialogueTree("assets/dialogue/" + wstrToStr(id) + ".xml");
}

bool Campaign::isBanterUsed(Banter& banter)
{
	return banter.used();
}

bool Campaign::isBanterAvailable(Banter& banter)
{
	if (banter.used()) return false; //if its been used throw it out
	if (banter.minSector() > currentSector->getType()) return false; //if it's too advanced throw it out
	bool found = false;
	for (auto& speaker : banter.speakers()) {
		found = false;
		for (auto& s : m_allSpeakers) {
			if (s == speaker) {
				found = true;
				break;
			}
		}
		if (!found) break;
	}
	if (!found) return false; //if all the speakers are not present throw it out
	bool prereqsDone = true;
	for (auto& tree : banter.requiredTrees()) {
		if (!isTreeUsed(tree)) {
			prereqsDone = false;
			break;
		}
	}
	for (auto& flag : banter.requiredFlags()) {
		if (!getFlag(flag)) {
			prereqsDone = false;
			break;
		}
	}
	for (auto& bantz : banter.requiredBanter()) {
		if (!isBanterUsed(banter)) {
			prereqsDone = false;
			break;
		}
	}
	if (!prereqsDone) return false; //if it doesnt have the prereqs throw it out

	return true;

}

const std::vector<Banter>& Campaign::getAvailableBanter()
{
	return m_availableBanter;
}

void Campaign::setBanterUsed(std::wstring id, bool used)
{
	if (m_allBanter.find(id) != m_allBanter.end()) {
		m_allBanter.at(id).setUsed(used);
	}
}

void Campaign::loadRandomBanter()
{
	m_availableBanter.clear();
	u32 count = 0;
	std::vector<std::wstring> bantz;
	bantz.clear();
	u32 maxMisses = 150;
	u32 misses = 0;
	for (auto& [id, banter] : m_allBanter) {
		if (!isBanterAvailable(banter)) continue;
		bool onWing = true;
		for (auto& spkr : banter.speakers()) {
			bool found = false;
			if (spkr == L"Steven Mitchell" || spkr == L"Kate Dietric" || spkr == L"Martin Hally") {
				found = true;
				continue;
			}
			for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
				if (!getAssignedWingman(i)) continue;
				if (spkr == wstr(getAssignedWingman(i)->name)) {
					found = true;
					break;
				}
			}
			if (!found) {
				onWing = false;
				break;
			}
		}
		if (!onWing) continue;

		bantz.push_back(id);
	}
	if (bantz.empty()) {
		baedsLogger::log("No banter available.\n");
		return;
	}

	if (bantz.size() == 1) {
		baedsLogger::log("One banter available: " + wstrToStr(bantz.front()) + "\n");
		m_availableBanter.push_back(m_allBanter[bantz.front()]);
		return;
	}

	if (bantz.size() == 2) {
		baedsLogger::log("Two banters available: " + wstrToStr(bantz.front()) + ", " + wstrToStr(bantz.back()) + "\n");
		m_availableBanter.push_back(m_allBanter[bantz.front()]);
		m_availableBanter.push_back(m_allBanter[bantz.back()]);
		return;
	}

	baedsLogger::log(std::to_string(bantz.size()) + " banters available. Rolls: ");

	u32 roll = random.unumEx((unsigned int)bantz.size());
	std::wstring first = bantz[roll];
	bantz.erase(bantz.begin() + roll);
	roll = random.unumEx((unsigned int)bantz.size());
	std::wstring second = bantz[roll];
	bantz.erase(bantz.begin() + roll);

	baedsLogger::log(wstrToStr(first) + ", " + wstrToStr(second) + "\n");

	m_availableBanter.push_back(m_allBanter[first]);
	m_availableBanter.push_back(m_allBanter[second]);
}

void Campaign::m_loadAllDialogue() //todo: thread this 
{
	for (const auto& file : std::filesystem::directory_iterator("assets/dialogue/")) {
		DialogueTree tree(file.path().string());
		m_allCharacterDialogues[tree.dialogueId()] = tree;
	}

	for (const auto& file : std::filesystem::directory_iterator("assets/events/random_events/")) {
		DialogueTree tree(file.path().string());
		m_randomEvents[tree.dialogueId()] = tree;
	}
	for (const auto& file : std::filesystem::directory_iterator("assets/events/plot_events/")) {
		DialogueTree tree(file.path().string());
		m_plotEvents[tree.dialogueId()] = tree;
	}
	for (const auto& file : std::filesystem::directory_iterator("assets/banter/")) {
		Banter banter;
		banter.load(file.path().string());
		m_allBanter[banter.id()] = banter;
	}
}

bool Campaign::advance()
{
	bool bossFight = currentSector->advance();
	return bossFight;
}

WeaponData* Campaign::getWepDataFromInstance(WeaponInstance* inst)
{
	if (!inst) return nullptr;
	HARDPOINT_TYPE type = inst->wep.hrdtype;
	s32 id = inst->wep.wepDataId;
	if (type == HRDP_HEAVY) return heavyWeaponData[id];
	else if (type == HRDP_PHYSICS) return physWeaponData[id];
	else return weaponData[id];
}

ShipUpgradeInstance* Campaign::addNewShipUpgradeInstance(ShipUpgradeData* dat, f32 value)
{
	if (!dat) return nullptr;
	ShipUpgradeInstance* inst = nullptr;
	switch (dat->type) {
		case SUP_HEALTH:
			inst = new SUIHealth;
			break;
		case SUP_SHIELD:
			inst = new SUIShield;
			break;
		case SUP_ACCELERATION:
			inst = new SUIAccel;
			break;
		case SUP_DECELERATION:
			inst = new SUIDecel;
			break;
		case SUP_BOOST:
			inst = new SUIBoost;
			break;
		case SUP_TURNING:
			inst = new SUITurn;
			break;
		case SUP_BFG:
			inst = new SUIFinale;
			break;
		default:
			break;
	}
	if (!inst) return nullptr;
	inst->dataId = dat->id;
	inst->value = value;
	++shipUpgradeCount;
	inst->id = shipUpgradeCount;
	m_shipUpgrades.push_back(inst);
	return inst;
}
WeaponUpgradeInstance* Campaign::addNewWeaponUpgradeInstance(WeaponUpgradeData* dat, f32 value)
{
	if (!dat) return nullptr;
	WeaponUpgradeInstance* inst = nullptr;
	switch (dat->type) {
	case WUP_DAMAGE:
		inst = new WUIDamage;
		break;
	case WUP_FIRERATE:
		inst = new WUIFirerate;
		break;
	case WUP_CLIPSIZE:
		inst = new WUIClipSize;
		break;
	case WUP_MAXAMMO:
		inst = new WUIMaxAmmo;
		break;
	case WUP_VELOCITY:
		inst = new WUIVelocity;
		break;
	default:
		break;
	}
	if (!inst) return nullptr;
	inst->dataId = dat->id;
	inst->value = value;
	++wepUpgradeCount;
	inst->id = wepUpgradeCount;
	m_weaponUpgrades.push_back(inst);
	return inst;
}
ShipUpgradeInstance* Campaign::addRandomShipUpgrade()
{
	s32 roll = random.unumEx((unsigned int)shipUpgradeData.size());
	auto dat = shipUpgradeData[roll];
	f32 value = dat->baseValue;
	u32 increment = random.unum((u32)((dat->maxValue - dat->baseValue) / dat->scaleValue));
	value += (f32)(increment * dat->scaleValue);
	return addNewShipUpgradeInstance(dat, value);
}
WeaponUpgradeInstance* Campaign::addRandomWeaponUpgrade()
{
	s32 roll = random.unumEx((unsigned int)weaponUpgradeData.size());
	auto dat = weaponUpgradeData[roll];
	f32 value = dat->baseValue;
	u32 increment = random.unum((u32)((dat->maxValue - dat->baseValue) / dat->scaleValue));
	value += (f32)(increment * dat->scaleValue);
	return addNewWeaponUpgradeInstance(dat, value);
}
bool Campaign::removeShipUpgrade(ShipUpgradeInstance* inst)
{
	if (!inst) return false; //what?
	auto it = m_shipUpgrades.begin();
	while (it != m_shipUpgrades.end()) {
		if (inst->id == (*it)->id) {
			it = m_shipUpgrades.erase(it);
			return true;
		}
		++it;
	}
	return false;
}
bool Campaign::removeWeaponUpgrade(WeaponUpgradeInstance* inst)
{
	if (!inst) return false;
	auto it = m_weaponUpgrades.begin();
	while (it != m_weaponUpgrades.end()) {
		if (inst->id == (*it)->id) {
			it = m_weaponUpgrades.erase(it);
			return true;
		}
		++it;
	}
	return false;
}

ShipUpgradeInstance* Campaign::getShipUpgrade(instId uniqueId)
{
	for (auto val : m_shipUpgrades) {
		if (val->id == uniqueId) return val;
	}
	return nullptr;
}
WeaponUpgradeInstance* Campaign::getWeaponUpgrade(instId uniqueId)
{
	for (auto val : m_weaponUpgrades) {
		if (val->id == uniqueId) return val;
	}
	return nullptr;
}

void Campaign::assignShipUpgrade(ShipInstance* inst, u32 slot, ShipUpgradeInstance* upgrade)
{
	if (!inst) return;
	if (inst->upgrades[slot] >= 0) getShipUpgrade(inst->upgrades[slot])->usedBy = -1;
	if (upgrade) {
		inst->upgrades[slot] = upgrade->id;
		upgrade->usedBy = inst->id;
	}
	else {
		inst->upgrades[slot] = -1;
	}
}
void Campaign::assignWepUpgrade(WeaponInstance* inst, u32 slot, WeaponUpgradeInstance* upgrade)
{
	if (!inst) return;
	if (inst->upgrades[slot] >= 0) getWeaponUpgrade(inst->upgrades[slot])->usedBy = -1;
	if (upgrade) {
		inst->upgrades[slot] = upgrade->id;
		upgrade->usedBy = inst->id;
	}
	else {
		inst->upgrades[slot] = -1;
	}
}

void Campaign::increaseCarrierUpgradeTier(std::string which)
{
	if (m_carrierUpgrades.find(which) == m_carrierUpgrades.end()) return;
	CarrierUpgrade& up = m_carrierUpgrades[which];
	if (up.maxtier == up.tier) return;
	++up.tier;
	up.upgrade(up);
}

std::vector<const WingmanMarker*> Campaign::getAvailableWingmen()
{
	std::vector<const WingmanMarker*> marks;
	marks.clear();
	for (auto& [id, marker] : wingMarkers) {
		if (std::find_if(m_wingmen.begin(), m_wingmen.end(), [&](const WingmanInstance* i) {return i->id == id; }) != m_wingmen.end()) continue; //already have him
		if (getSector()->getType() < marker->minSector) continue; //not his time
		marks.push_back(marker);
	}
	return marks;
}

void Campaign::exitCampaign()
{
	baedsLogger::log("Cleaning out old campaign data on exit - ");

	if(campaign->getSector()) campaign->getSector()->deselectCurrentScenario();
	GuiCampaignMenu* menu = (GuiCampaignMenu*)guiController->getDialogueByType(GUI_CAMPAIGN_MENU);
	menu->showing = -1;

	for (auto val : m_wingmen) {
		if (val) delete val;
	}
	for (auto val : m_ships) {
		if (val) delete val;
	}
	for (auto val : m_weapons) {
		if (val) delete val;
	}
	for (auto val : m_physWeapons) {
		if (val) delete val;
	}
	for (auto val : m_heavyWeapons) {
		if (val) delete val;
	}
	for (auto val : m_shipUpgrades) {
		if (val) delete val;
	}
	for (auto val : m_weaponUpgrades) {
		if (val) delete val;
	}
	if (currentSector) {
		currentSector->clearScenarios();
		delete currentSector;
	}
	currentSector = nullptr;

	m_wingmen.clear();
	m_ships.clear();
	m_weapons.clear();
	m_physWeapons.clear();
	m_heavyWeapons.clear();
	m_shipUpgrades.clear();
	m_weaponUpgrades.clear();
	m_allSpeakers.clear();

	shipCount = 0;
	wepCount = 0;
	shipUpgradeCount = 0;
	wepUpgradeCount = 0;

	currentDifficulty = 1;

	baedsLogger::log("Done.\n");
}
void Campaign::newCampaign()
{
	baedsLogger::log("Creating new campaign... ");
	exitCampaign();
	ammunition = START_AMMO;
	supplies = START_SUPPLIES;
	currentSector = new DebrisSector();

	currentSector->buildScenarios();

	auto playerShip = buildStarterShip();
	addShipInstanceToHangar(playerShip);

	auto anubis = buildShipInstanceFromData(shipData[2]);
	anubis->weps[0] = createNewWeaponInstance(weaponData[STARTER_WEP]->wepComp)->id;
	anubis->weps[1] = createNewWeaponInstance(weaponData[STARTER_WEP]->wepComp)->id;
	getWeapon(anubis->weps[0])->usedBy = anubis->id;
	getWeapon(anubis->weps[1])->usedBy = anubis->id;
	addShipInstanceToHangar(anubis);

	//gougar
	auto gougar = buildShipInstanceFromData(shipData[18]);
	gougar->weps[0] = createNewWeaponInstance(weaponData[7]->wepComp)->id;
	getWeapon(gougar->weps[0])->usedBy = gougar->id;
	addShipInstanceToHangar(gougar);

	addNewShipUpgradeInstance(shipUpgradeData[0], 30.f);
	addNewWeaponUpgradeInstance(weaponUpgradeData[0], .20f);
	addNewWeaponUpgradeInstance(weaponUpgradeData[0], .20f);

	m_loadAllDialogue();
	m_loadAllFlags();
	m_loadAllCarrierUpgrades();
	m_freshCarrierStats();

	timeSpentInCampaign = 0;
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		assignedWing[i] = -1;
		assignedShips[i] = -1;
	}

	auto player = loadWingman(wingMarkers[0], true); //player
	auto sean = loadWingman(wingMarkers[1], true); //sean
	auto cat = loadWingman(wingMarkers[2], true); //cat
	m_wingmen.push_back(player);
	addWingman(sean);
	addWingman(cat);

	setAssignedWingman(sean, 0);
	setAssignedWingman(cat, 1);
	setAssignedShip(gougar, 0);
	assignWingmanToShip(sean, gougar);
	setAssignedShip(anubis, 1);
	assignWingmanToShip(cat, anubis);

	m_allSpeakers.push_back(L"Martin Hally");
	m_allSpeakers.push_back(L"Steven Mitchell");
	m_allSpeakers.push_back(L"Kate Dietric");

	assignWingmanToShip(getWingman(0), playerShip);

	getRandomCharacterDialogues();
	guiController->setDialogueTree(campaign->getCharacterDialogue(L"steven_newgame"));
	guiController->setEventDialoguePopup();
	baedsLogger::log("Done.\n");
}

void Campaign::m_freshCarrierStats()
{
	m_carrierStats.hp.health = 25000.f;
	m_carrierStats.hp.maxHealth = 25000.f;
	m_carrierStats.hp.healthResistances[EXPLOSIVE] = 1.f;
	m_carrierStats.hp.healthResistances[IMPACT] = .75f;
	m_carrierStats.hp.healthResistances[VELOCITY] = 1.f;
	m_carrierStats.hp.shields = 0.f;
	m_carrierStats.hp.maxShields = 0.f;
	m_carrierStats.thrst = shipData[CHAOS_THEORY_ID]->thrust;
	m_carrierStats.turretId = 0;
	m_carrierStats.turretWepId = 3;
}

void Campaign::m_loadAllCarrierUpgrades()
{
	gvReader in;
	for (const auto& file : std::filesystem::directory_iterator("assets/attributes/upgrades/carrier/")) {
		baedsLogger::log("Reading in carrier upgrade from path " + file.path().string() + " - ");
		in.read(file.path().string());
		if (in.lines.empty()) {
			baedsLogger::log(" could not read!!\n");
			continue;
		}
		in.readLinesToValues();

		CarrierUpgrade up;
		up.canBuild = (in.getString("canBuild") == "yes") ? true : false;
		up.name = in.getString("name");
		up.desc = in.getString("desc");
		up.cost = in.getFloat("cost");
		up.maxtier = in.getUint("maxTier");
		up.tier = 0;
		up.upgrade = carrUpgradeCallbacks.at(up.name);
		m_carrierUpgrades[up.name] = up;
		std::cout << "Done.\n";
		in.clear();
	}
}

void Campaign::m_loadAllFlags()
{
	gvReader in;
	baedsLogger::log("Loading all campaign flags...\n");
	in.read("assets/attributes/campaign_flags.flg");
	for (u32 i = 0; i < in.lines.size(); ++i) {
		m_flags[wstr(in.lines[i])] = false;
	}
	in.clear();
}

void Campaign::rollForEvent()
{
	campaign->setFlag(L"EVENT_AVAILABLE", false);
	campaign->setFlag(L"CUR_EVENT_IS_PLOT", false);

	if (!m_plotEvents.at(L"arthur_intro").isUsed() && !campaign->getFlag(L"ARTHUR_RECRUITED")) { //always trigger this immediately
		setFlag(L"CUR_EVENT_IS_PLOT");
		m_currentEvent = m_plotEvents.at(L"arthur_intro");
		m_plotEvents.at(L"arthur_intro").setUsed();
		return;
	}

	for (auto& [id, tree] : m_plotEvents) {
		if (!isDialogueAvailable(tree)) continue;
		m_currentEvent = tree;
		setFlag(L"CUR_EVENT_IS_PLOT");
		return;
	}
	s32 roll = random.unumEx((unsigned int)m_randomEvents.size());
	auto it = m_randomEvents.begin();
	std::advance(it, roll);
	m_currentEvent = it->second;
}

bool Campaign::saveCampaign(std::string fname)
{
	IXMLWriter* out = device->getFileSystem()->createXMLWriter(fname.c_str());
	if (!out) return false;
	out->writeXMLHeader();
	array<stringw> names;
	array<stringw> vals;
	/*
	* This has to be some of the more tedious code I've ever written - file formats are just *awful.*
	* For anyone in the future who ends up reading this - I was entirely zoned out while writing this, and
	* listening to The Venture Bros soundtrack for the whole experience.
	* 
	* I shall not, with luck, ever have to look at this damn code again.
	*/
	names.push_back("time");
	vals.push_back(stringw(timeSpentInCampaign));
	names.push_back("difficulty");
	vals.push_back(stringw(currentDifficulty));
	names.push_back("ammo");
	vals.push_back(stringw(ammunition));
	names.push_back("supplies");
	vals.push_back(stringw(supplies));
	names.push_back("sector_type");
	vals.push_back(stringw(getSector()->getType()));
	names.push_back("moved");
	std::string str = getSector()->hasMoved() ? "yes" : "no";
	vals.push_back(stringw(str.c_str()));
	names.push_back("wingmen_recruited");
	vals.push_back(stringw(getSector()->wingmenRecruitedThisSector));
	names.push_back("encounter");
	vals.push_back(stringw(getSector()->getEncounterNum()));
	names.push_back("ship_count");
	vals.push_back(stringw(shipCount));
	names.push_back("wep_count");
	vals.push_back(stringw(wepCount));
	names.push_back("ship_up_count");
	vals.push_back(stringw(shipUpgradeCount));
	names.push_back("wep_up_count");
	vals.push_back(stringw(wepUpgradeCount));

	out->writeElement(L"campaign", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();

	names.push_back("playerShip");
	vals.push_back(stringw(playerShip));
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		names.push_back(stringw("wingman") + stringw(i));
		vals.push_back(stringw(assignedWing[i]));
	}
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		names.push_back(stringw("ship") + stringw(i));
		vals.push_back(stringw(assignedShips[i]));
	}
	out->writeElement(L"assignments", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();
	for (auto& [id, val] : m_flags) {
		names.push_back(id.c_str());
		stringw value = val ? L"yes" : L"no";
		vals.push_back(value);
	}
	out->writeElement(L"flags", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();
	for (auto& [id, tree] : m_allCharacterDialogues) {
		names.push_back(id.c_str());
		stringw value = tree.isUsed() ? L"yes" : L"no";
		vals.push_back(value);
	}
	out->writeElement(L"character_dialogues", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();
	for (auto& [id, bantz] : m_allBanter) {
		names.push_back(id.c_str());
		stringw value = bantz.used() ? L"yes" : L"no";
		vals.push_back(value);
	}
	out->writeElement(L"banter", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();

	for (auto& [id, tree] : m_plotEvents) {
		names.push_back(id.c_str());
		stringw value = tree.isUsed() ? L"yes" : L"no";
		vals.push_back(value);
	}
	out->writeElement(L"plot_events", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();

	names.push_back("id");
	stringw curEvent = (m_currentEvent.dialogueId() == L"") ? L"none" : m_currentEvent.dialogueId().c_str();
	vals.push_back(curEvent);
	out->writeElement(L"current_event", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();

	for (auto& [id, up] : m_carrierUpgrades) {
		std::string name = id;
		std::replace(name.begin(), name.end(), ' ', '_');
		names.push_back(name.c_str());
		vals.push_back(stringw(up.tier));
	}
	out->writeElement(L"carrier_upgrades", true, names, vals);

	names.clear();
	vals.clear();
	out->writeLineBreak();

	for (auto man : m_wingmen) {
		names.push_back("name");
		vals.push_back(man->name.c_str());
		names.push_back("id");
		vals.push_back(stringw(man->id));
		names.push_back("assigned_ship");
		vals.push_back(stringw(man->assignedShip));
		names.push_back("injured");
		stringw stat = man->injured ? "yes" : "no";
		vals.push_back(stat);
		names.push_back("turns_injured");
		vals.push_back(stringw(man->turnsInjured));
		names.push_back("total_injuries");
		vals.push_back(stringw(man->totalInjuries));
		names.push_back("kills");
		vals.push_back(stringw(man->totalKills));
		names.push_back("aggressiveness");
		vals.push_back(stringw(man->ai.aggressiveness));
		names.push_back("reactionSpeed");
		vals.push_back(stringw(man->ai.reactionSpeed));
		names.push_back("resolve");
		vals.push_back(stringw(man->ai.resolve));
		names.push_back("aim");
		vals.push_back(stringw(man->ai.aim));
		names.push_back("desc");
		vals.push_back(man->description.c_str());

		out->writeElement(L"wingman", true, names, vals);
		names.clear();
		vals.clear();
		out->writeLineBreak();
	}
	for (auto ship : m_ships) {
		names.push_back("inst_id");
		vals.push_back(stringw(ship->id));
		names.push_back("ship_id");
		vals.push_back(stringw(ship->ship.shipDataId));
		names.push_back("assigned_man");
		vals.push_back(stringw(ship->inUseBy));
		names.push_back("hp");
		vals.push_back(stringw(ship->hp.health));
		names.push_back("hardpoint_count");
		vals.push_back(stringw(ship->hards.hardpointCount));
		for (u32 i = 0; i < ship->hards.hardpointCount; ++i) {
			names.push_back(stringw("wep_inst_id") + stringw(i));
			vals.push_back(stringw(ship->weps[i]));
		}
		names.push_back("phys_id");
		vals.push_back(stringw(ship->physWep));
		names.push_back("hvy_id");
		vals.push_back(stringw(ship->heavyWep));
		for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
			names.push_back(stringw("ship_up_id") + stringw(i));
			vals.push_back(stringw(ship->upgrades[i]));
		}
		out->writeElement(L"ship_inst", true, names, vals);
		names.clear();
		vals.clear();
		out->writeLineBreak();
	}
	std::list<WeaponInstance*> weps = m_weapons;
	weps.insert(weps.end(), m_heavyWeapons.begin(), m_heavyWeapons.end());
	weps.insert(weps.end(), m_physWeapons.begin(), m_physWeapons.end());

	for (auto wep : weps) {
		names.push_back("inst_id");
		vals.push_back(stringw(wep->id));
		names.push_back("wep_id");
		vals.push_back(stringw(wep->wep.wepDataId));
		names.push_back("wep_type");
		vals.push_back(stringw(wep->wep.hrdtype));
		names.push_back("ammo");
		vals.push_back(stringw(wep->fire.ammunition)); //this is unused on non-ammo weps
		names.push_back("used_by");
		vals.push_back(stringw(wep->usedBy));
		for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
			names.push_back(stringw("wep_up_id") + stringw(i));
			vals.push_back(stringw(wep->upgrades[i]));
		}
		out->writeElement(L"wep_inst", true, names, vals);
		names.clear();
		vals.clear();
		out->writeLineBreak();
	}
	for (auto sup : m_shipUpgrades) {
		names.push_back("inst_id");
		vals.push_back(stringw(sup->id));
		names.push_back("up_id");
		vals.push_back(stringw(sup->dataId));
		names.push_back("used_by");
		vals.push_back(stringw(sup->usedBy));
		names.push_back("value");
		vals.push_back(stringw(sup->value));
		out->writeElement(L"ship_up_inst", true, names, vals);
		names.clear();
		vals.clear();
		out->writeLineBreak();
	}
	for (auto sup : m_weaponUpgrades) {
		names.push_back("inst_id");
		vals.push_back(stringw(sup->id));
		names.push_back("up_id");
		vals.push_back(stringw(sup->dataId));
		names.push_back("used_by");
		vals.push_back(stringw(sup->usedBy));
		names.push_back("value");
		vals.push_back(stringw(sup->value));
		out->writeElement(L"wep_up_inst", true, names, vals);
		names.clear();
		vals.clear();
		out->writeLineBreak();
	}

	out->drop();
	return true;
}
bool Campaign::loadCampaign(std::string fname)
{
	baedsLogger::log("Loading campaign from file " + fname + "... ");
	IrrXMLReader* xml = createIrrXMLReader(fname.c_str());
	if (!xml) {
		baedsLogger::log("Could not read file!\n");
		return false;
	}
	exitCampaign();

	m_allSpeakers.push_back(L"Martin Hally");
	m_allSpeakers.push_back(L"Steven Mitchell");
	m_allSpeakers.push_back(L"Kate Dietric");

	m_loadAllDialogue();
	m_loadAllCarrierUpgrades();
	m_loadAllFlags();
	m_freshCarrierStats();

	while (xml->read()) {
		const stringw type = xml->getNodeName();
		if (type == L"campaign") {
			timeSpentInCampaign = xml->getAttributeValueAsInt("time");
			currentDifficulty = xml->getAttributeValueAsInt("difficulty");
			ammunition = xml->getAttributeValueAsInt("ammo");
			supplies = xml->getAttributeValueAsFloat("supplies");
			shipCount = xml->getAttributeValueAsInt("ship_count");
			wepCount = xml->getAttributeValueAsInt("wep_count");
			shipUpgradeCount = xml->getAttributeValueAsInt("ship_up_count");
			wepUpgradeCount = xml->getAttributeValueAsInt("wep_up_count");

			SECTOR_TYPE type = (SECTOR_TYPE)xml->getAttributeValueAsInt("sector_type");
			std::string movedstr = xml->getAttributeValueSafe("moved");
			bool moved = (movedstr == "yes") ? true : false;
			u32 encounter = xml->getAttributeValueAsInt("encounter");
			m_buildSector(type);
			currentSector->initSectorFromSettings(moved, encounter);
			currentSector->wingmenRecruitedThisSector = xml->getAttributeValueAsInt("wingmen_recruited");
		}
		if (type == L"assignments") {
			playerShip = xml->getAttributeValueAsInt("playerShip");
			for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
				assignedWing[i] = xml->getAttributeValueAsInt(std::string("wingman" + std::to_string(i)).c_str());
				assignedShips[i] = xml->getAttributeValueAsInt(std::string("ship" + std::to_string(i)).c_str());
			}
		}
		if (type == L"flags") {
			for (auto& [flag, val] : m_flags) {
				std::string str = xml->getAttributeValueSafe(wstrToStr(flag).c_str());

				m_flags[flag] = (str == "yes") ? true : false;
			}
		}
		if (type == L"character_dialogues") {
			for (auto& [id, tree] : m_allCharacterDialogues) {
				std::string val = xml->getAttributeValueSafe(wstrToStr(id).c_str());
				bool used = (val == "yes") ? true : false;
				m_allCharacterDialogues[id].setUsed(used);
			}
		}
		if (type == L"banter") {
			for (auto& [id, bantz] : m_allBanter) {
				std::string val = xml->getAttributeValueSafe(wstrToStr(id).c_str());
				bool used = (val == "yes") ? true : false;
				m_allBanter[id].setUsed(used);
			}
		}

		if (type == L"plot_events") {
			for (auto& [id, tree] : m_plotEvents) {
				std::string val = xml->getAttributeValueSafe(wstrToStr(id).c_str());
				bool used = (val == "yes") ? true : false;
				m_plotEvents[id].setUsed(used);
			}
		}
		if (type == L"current_event") {
			std::wstring id = wstr(xml->getAttributeValueSafe("id"));
			if (m_plotEvents.find(id) != m_plotEvents.end()) m_currentEvent = m_plotEvents.at(id);
			else if (m_randomEvents.find(id) != m_randomEvents.end()) m_currentEvent = m_randomEvents.at(id);
		}
		if (type == L"carrier_upgrades") {
			for (auto& [id, up] : m_carrierUpgrades) {
				std::string xmlname = id;
				std::replace(xmlname.begin(), xmlname.end(), ' ', '_');
				u32 tier = xml->getAttributeValueAsInt(xmlname.c_str());
				if (tier > 0) {
					m_carrierUpgrades[id].tier = tier;
					m_carrierUpgrades[id].upgrade(m_carrierUpgrades[id]);
				}
			}
		}
		if (type == L"wingman") {
			//auto man = new WingmanInstance;
			auto marker = wingMarkers[xml->getAttributeValueAsInt("id")];
			auto man = loadWingman(marker, true);
			m_allSpeakers.push_back(wstr(man->name));
			man->assignedShip = xml->getAttributeValueAsInt("assigned_ship");
			if (man->assignedShip > -1 && man->id != 5) man->assigned = true;
			else if (man->id == 5) {
				for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
					if (campaign->assignedWing[i] == man->id) man->assigned = true;
				}
			}
			else man->assigned = false;
			std::string injury = xml->getAttributeValueSafe("injured");
			man->injured = (injury == "yes") ? true : false;
			man->turnsInjured = xml->getAttributeValueAsInt("turns_injured");
			man->totalInjuries = xml->getAttributeValueAsInt("total_injuries");
			man->totalKills = xml->getAttributeValueAsInt("kills");

			man->ai.aggressiveness = xml->getAttributeValueAsFloat("aggressiveness");
			man->ai.reactionSpeed = xml->getAttributeValueAsFloat("reactionSpeed");
			man->ai.resolve = xml->getAttributeValueAsFloat("resolve");
			man->ai.aim = xml->getAttributeValueAsFloat("aim");

			addWingman(man);
		}
		if (type == L"ship_inst") {
			ShipInstance* inst = new ShipInstance;
			s32 dataId = xml->getAttributeValueAsInt("ship_id");
			inst->id = xml->getAttributeValueAsInt("inst_id");
			inst->ship = shipData[dataId]->ship;
			inst->hards = shipData[dataId]->hards;
			inst->hp = shipData[dataId]->hp;
			inst->hp.health = xml->getAttributeValueAsFloat("hp");
			inst->inUseBy = xml->getAttributeValueAsInt("assigned_man");
			for (u32 i = 0; i < inst->hards.hardpointCount; ++i) {
				std::string idVal = "wep_inst_id" + std::to_string(i);
				inst->weps[i] = xml->getAttributeValueAsInt(idVal.c_str());
			}
			inst->physWep = xml->getAttributeValueAsInt("phys_id");
			inst->heavyWep = xml->getAttributeValueAsInt("hvy_id");
			for (u32 i = 0; i < MAX_SHIP_UPGRADES; ++i) {
				std::string idVal = "ship_up_id" + std::to_string(i);
				inst->upgrades[i] = xml->getAttributeValueAsInt(idVal.c_str());
			}
			addShipInstanceToHangar(inst);
		}
		if (type == L"wep_inst") {
			WeaponInstance* wep = new WeaponInstance;
			wep->id = xml->getAttributeValueAsInt("inst_id");
			s32 dataId = xml->getAttributeValueAsInt("wep_id");
			HARDPOINT_TYPE type = (HARDPOINT_TYPE)xml->getAttributeValueAsInt("wep_type");
			if (type == HRDP_HEAVY) wep->wep = heavyWeaponData[dataId]->wepComp;
			else if (type == HRDP_PHYSICS) wep->wep = physWeaponData[dataId]->wepComp;
			else wep->wep = weaponData[dataId]->wepComp;
			wep->fire.ammunition = xml->getAttributeValueAsInt("ammo");
			wep->usedBy = xml->getAttributeValueAsInt("used_by");
			for (u32 i = 0; i < MAX_WEP_UPGRADES; ++i) {
				std::string idVal = "wep_up_id" + std::to_string(i);
				wep->upgrades[i] = xml->getAttributeValueAsInt(idVal.c_str());
			}
			addWeapon(wep);
		}
		if (type == L"ship_up_inst") {
			s32 id = xml->getAttributeValueAsInt("up_id");
			f32 val = xml->getAttributeValueAsFloat("value");
			auto inst = addNewShipUpgradeInstance(shipUpgradeData[id], val);
			inst->id = xml->getAttributeValueAsInt("inst_id");
			inst->usedBy = xml->getAttributeValueAsInt("used_by");
			--shipUpgradeCount;
		}
		if (type == L"wep_up_inst") {
			s32 id = xml->getAttributeValueAsInt("up_id");
			f32 val = xml->getAttributeValueAsFloat("value");
			auto inst = addNewWeaponUpgradeInstance(weaponUpgradeData[id], val);
			inst->id = xml->getAttributeValueAsInt("inst_id");
			inst->usedBy = xml->getAttributeValueAsInt("used_by");
			--wepUpgradeCount;
		}
	}
	if (currentSector->getEncounterNum() >= MAX_ENCOUNTERS) currentSector->buildBossScenario();
	else currentSector->buildScenarios();
	getRandomCharacterDialogues();
	delete xml;
	baedsLogger::log("Done.\n");
	return true;
}

