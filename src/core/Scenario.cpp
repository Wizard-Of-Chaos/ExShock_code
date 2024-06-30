#include "Scenario.h"
#include "GameController.h"
#include "Campaign.h"
#include "LoadoutData.h"
#include "AttributeReaders.h"
#include "CrashLogger.h"

static const bool _wingMissionNotShown(SCENARIO_TYPE which)
{
	for (u32 i = 0; i < NUM_SCENARIO_OPTIONS; ++i) {
		auto scen = campaign->getSector()->getScenario(i);
		if (!scen)
			continue;
		if (scen->type() == which)
			return false;
	}
	return true;
}

static const SCENARIO_TYPE _checkForWingMissions()
{
	if (campaign->getFlag(L"THEOD_MISSION_AVAILABLE") && _wingMissionNotShown(SCENARIO_THEOD))
		return SCENARIO_THEOD;
	if (campaign->getFlag(L"LEE_MISSION_AVAILABLE") && _wingMissionNotShown(SCENARIO_MI_CHA))
		return SCENARIO_MI_CHA;
	if (campaign->getFlag(L"ARTHUR_MISSION_AVAILABLE") && _wingMissionNotShown(SCENARIO_ARTHUR))
		return SCENARIO_ARTHUR;
	if (campaign->getFlag(L"SEAN_MISSION_AVAILABLE") && _wingMissionNotShown(SCENARIO_SEAN))
		return SCENARIO_SEAN;
	if (campaign->getFlag(L"JAMES_MISSION_AVAILABLE") && _wingMissionNotShown(SCENARIO_JAMES))
		return SCENARIO_JAMES;
	if (campaign->getFlag(L"TAURAN_MISSION_AVAILABLE") && _wingMissionNotShown(SCENARIO_TAURAN))
		return SCENARIO_TAURAN;
	if (campaign->getFlag(L"CAT_MISSION_AVAILABLE") && _wingMissionNotShown(SCENARIO_CAT))
		return SCENARIO_CAT;

	return SCENARIO_NOT_LOADED;
}

void Scenario::m_loadScenData(u32 difficulty, SCENARIO_TYPE ntype)
{
	SCENARIO_TYPE type = ntype;
	if (type == SCENARIO_NOT_LOADED) {
		gvReader secIn;
		secIn.read("assets/attributes/scenarios/environments/" + environmentTypeStrings.at(m_environment) + ".gdat");
		secIn.readLinesToValues();

		for (u32 i = 0; i < SCENARIO_MAX_TYPES; ++i) {
			auto which = (SCENARIO_TYPE)i;
			if (which == SCENARIO_RETRIEVE_POD && ((campaign->getSector()->wingmenRecruitedThisSector >= WINGMEN_RECRUITED_PER_SECTOR || campaign->wingmen().size() > WINGMEN_PER_CAMPAIGN)
				|| campaign->getAvailableWingmen().size() == 0)) {
				baedsLogger::log("No wingmen. Reason: ");
				if (campaign->getSector()->wingmenRecruitedThisSector >= WINGMEN_RECRUITED_PER_SECTOR) baedsLogger::log("Max per sector ");
				if (campaign->wingmen().size() >= WINGMEN_PER_CAMPAIGN) baedsLogger::log("Max per campaign ");
				if (campaign->getAvailableWingmen().size() == 0) baedsLogger::log("None available");
				baedsLogger::log("\n");
				continue; //too many wingmen
			}
			if (secIn.hasVal(objectiveTypeStrings.at(which))) {
				_typeWeight val = { which, secIn.getUint(objectiveTypeStrings.at(which)) };
				availableScenarioWeights.push_back(val);
			}
		}
		u32 total = 0;
		for (auto& which : availableScenarioWeights) {
			total += which.weight;
		}
		u32 roll = random.urange(0, total);
		u32 pos = 0;
		for (auto& val : availableScenarioWeights) {
			if (roll <= (pos + val.weight) && roll >= pos) {
				type = val.which;
				break;
			}
			pos += val.weight;
		}
	}
	SCENARIO_TYPE wingCheck = _checkForWingMissions();
	if (wingCheck != SCENARIO_NOT_LOADED)
		type = wingCheck;

	gvReader in;
	in.read("assets/attributes/scenarios/environments/" + environmentTypeStrings.at(m_environment) + ".gdat");
	in.readLinesToValues();

	m_type = type;
	std::string location = environmentTypeStrings.at(m_environment);
	std::string description = in.getString("description");
	description += "\n\n";
	//description += in.values[objectiveTypeStrings.at(type)];

	in.clear();
	std::string path = "assets/attributes/scenarios/objectives/" + objectiveTypeStrings.at(type) + ".gdat";
	in.read(path);
	in.readLinesToValues();
	u32 extraVal = 1 + (u32)m_environment;
	u32 baseAmmo = in.getUint("ammoRecovered");
	f32 baseSupplies = in.getFloat("resourcesRecovered");
	description += in.getString("description");
	m_location = location;

	m_ammoRecovered = baseAmmo + random.unum(extraVal * baseAmmo); //add in additional based off the basic value
	m_suppliesRecovered = baseSupplies + random.frange(0.f, baseSupplies * (f32)extraVal);

	m_wepsRecovered = random.srange(0, in.getInt("maxWeaponsRecovered"));
	m_shipsRecovered = random.srange(0, in.getInt("maxShipsRecovered"));

	path = "assets/attributes/scenarios/environments/" + location + ".gdat";
	in.read(path);
	in.readLinesToValues();
	m_detectionChance = in.getInt("detectionChance") + (2 * random.unum(extraVal));
	if (m_detectionChance > 50) m_detectionChance = 50;
	if (m_environment == SECTOR_RINGS) m_detectionChance = 0;

	if (type == SCENARIO_BOSSFIGHT || type == SCENARIO_SCRAMBLE || type >= SCENARIO_MAX_TYPES) m_detectionChance = 0; //The man has already got you.

	m_description = description;
}

void Scenario::setRandom(u32 difficulty)
{
	m_loadScenData(difficulty);
}
void Scenario::setScramble(u32 difficulty)
{
	m_loadScenData(difficulty, SCENARIO_SCRAMBLE);
}

void Scenario::setType(u32 difficulty, SCENARIO_TYPE type)
{
	m_loadScenData(difficulty, type);
}

void Scenario::print()
{
	baedsLogger::log("Current scenario: " + std::to_string(m_type) + ", " + std::to_string(m_environment) + ".\n");
	baedsLogger::log("Ammo / Supplies recovered: " + std::to_string(m_ammoRecovered) + ", " + std::to_string(m_suppliesRecovered) + ".\n");
	baedsLogger::log("Ships / Weapons recovered: " + std::to_string(m_shipsRecovered) + ", " + std::to_string(m_wepsRecovered) + ".\n");
}

void Scenario::getLoot()
{
	m_recoveredShips.clear();
	m_recoveredWeps.clear();
	m_recoveredWingman = nullptr;

	if (m_type == SCENARIO_RETRIEVE_POD) { //make sure we're not maxed out, somehow
		auto list = campaign->getAvailableWingmen();
		if (list.size() > 0) {
			if (!campaign->getFlag(L"ARNOLD_RECRUITED")) { //arnold is the first every time
				campaign->addWingman(loadWingman(wingMarkers[3], true));
			}
			else {
				const u32 which = random.unumEx((unsigned int)list.size());
				campaign->addWingman(loadWingman(list[which], true));
			}
			++campaign->getSector()->wingmenRecruitedThisSector;
		}
	}
	for (u32 i = 0; i < numWepsRecovered(); ++i) {
		auto wepinst = campaign->createRandomWeaponInstance();
		m_recoveredWeps.push_back(wepinst->wep);
		campaign->addWeapon(wepinst);
	}
	for (u32 i = 0; i < numShipsRecovered(); ++i) {
		auto shipinst = campaign->createRandomShipInstance();
		m_recoveredShips.push_back(shipinst);
		campaign->addShipInstanceToHangar(shipinst);
	}
	campaign->addAmmo(numAddedAmmo());
	campaign->addSupplies(numAddedSupplies());

}
