#include "DialogueEffects.h"
#include "Campaign.h"
#include "AudioDriver.h"
#include "GameFunctions.h"
#include "IrrlichtUtils.h"
#include "GuiController.h"
#include "GuiDialogueMenu.h"
#include "AttributeReaders.h"

static void _injureWingman(dataId which, s32 turnsInjured=0)
{
	auto man = campaign->getWingman(which);
	man->injured = true;
	man->turnsInjured = turnsInjured;
	if(turnsInjured >= 0) ++man->totalInjuries;
	for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
		if (campaign->getAssignedWingman(i) == man) {
			campaign->removeAssignedWingman(i);
			break;
		}
	}
}

void smallSupply()
{
	campaign->addSupplies(random.frange(1.f, 12.f));
}
void mediumSupply()
{
	campaign->addSupplies(random.frange(10.f, 40.f));
}
void largeSupply()
{
	campaign->addSupplies(random.frange(20.f, 55.f));
}

void smallAmmo()
{
	campaign->addAmmo(random.srange(1, 6));
}
void mediumAmmo()
{
	campaign->addAmmo(random.srange(4, 14));
}
void largeAmmo()
{
	campaign->addAmmo(random.srange(10,30));
}

void randEventAblative()
{
	campaign->increaseCarrierUpgradeTier("Alien Ablative Armor");
}
void randEventFreeTux()
{
	auto tux = campaign->createNewShipInstance(0);
	tux->hp.health = 10.f;
	campaign->addShipInstanceToHangar(tux);
}

WingmanInstance* _getCurWingman()
{
	auto menu = (GuiDialogueMenu*)guiController->getDialogueByType(GUI_DIALOGUE_MENU);
	if (!menu) return nullptr;
	auto spkr = menu->getCurSpeaker();

	for (auto man : campaign->wingmen()) {
		if (wstr(man->name) == spkr) return man;
	}

	return nullptr;
}

void aimGain()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.aim += .1f;
	wingman->ai.aim = std::clamp(wingman->ai.aim, 0.1f, 10.f);
	campaign->saveCampaign("saves/autosave.xml");
}
void aimLoss()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.aim -= .1f;
	wingman->ai.aim = std::clamp(wingman->ai.aim, 0.1f, 10.f);
	campaign->saveCampaign("saves/autosave.xml");
}
void resolveGain()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.resolve += .05f;
	wingman->ai.resolve = std::clamp(wingman->ai.resolve, 0.001f, .9f);
	campaign->saveCampaign("saves/autosave.xml");
}
void resolveLoss()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.resolve -= .05f;
	wingman->ai.resolve = std::clamp(wingman->ai.resolve, 0.001f, .9f);
	campaign->saveCampaign("saves/autosave.xml");
}
void reactionGain()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.reactionSpeed += .1f;
	wingman->ai.reactionSpeed = std::clamp(wingman->ai.reactionSpeed, .1f, 4.f);
	campaign->saveCampaign("saves/autosave.xml");
}
void reactionLoss()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.reactionSpeed -= .1f;
	wingman->ai.reactionSpeed = std::clamp(wingman->ai.reactionSpeed, .1f, 4.f);
	campaign->saveCampaign("saves/autosave.xml");
}
void aggroGain()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.aggressiveness += .1f;
	wingman->ai.aggressiveness = std::clamp(wingman->ai.aggressiveness, .2f, 4.f);
	campaign->saveCampaign("saves/autosave.xml");
}
void aggroLoss()
{
	auto wingman = _getCurWingman();
	if (!wingman) return;

	wingman->ai.aggressiveness -= .1f;
	wingman->ai.aggressiveness = std::clamp(wingman->ai.aggressiveness, .2f, 4.f);
	campaign->saveCampaign("saves/autosave.xml");
}

void randEventSeanCache()
{
	for (u32 i = 0; i < 3; ++i) {
		campaign->createNewWeaponInstance(weaponData[1]->wepComp);
	}
	_injureWingman(1);

	campaign->getWingman(1)->totalKills += random.urange(10, 16);
}

void randEventTheodBuff()
{
	u32 roll = random.d4();
	switch (roll) {
	case 1:
		aimGain();
		break;
	case 2:
		resolveLoss();
		break;
	case 3:
		reactionLoss();
		break;
	case 4:
		aggroGain();
		break;
	default:
		break;
	}
}

void recruitArthur()
{
	//WingmanInstance* arthur = new WingmanInstance;
	auto arthur = loadWingman(wingMarkers[5], true);
	campaign->addWingman(arthur);
	auto ship = campaign->createNewShipInstance(5);
	campaign->addShipInstanceToHangar(ship);
	campaign->assignWingmanToShip(arthur, ship);
}

void leeBuff()
{
	campaign->createNewWeaponInstance(heavyWeaponData[8]->wepComp);
}

void kateProject()
{
	//nothing for now
}

void shipAfterburn()
{
	audioDriver->playMenuSound("../game/ship_afterburn.ogg");
}

void arthurActivate()
{
	audioDriver->playMenuSound("../game/weapon_hum_impulse.ogg");
}

void dialogueWhistle()
{
	audioDriver->playMenuSound("../game/incoming_message.ogg");
}

void arnoldFloorHit()
{
	_injureWingman(3, 1);
	audioDriver->playMenuSound("../game/weapon_hit_impulse.ogg");
}

void noDialogueEffect()
{
	//NOTHING!
}

void addBFG()
{
	campaign->addNewShipUpgradeInstance(shipUpgradeData[1], 100);
}

void tauranTKO()
{
	aggroGain();
	auto lee = campaign->getWingman(9);
	auto tauran = campaign->getWingman(4);
	if (lee && tauran) {
		lee->injured = true;
		lee->turnsInjured = 1;
		++lee->totalInjuries;
		++tauran->totalKills;
		for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
			if (campaign->getAssignedWingman(i) == lee) {
				campaign->removeAssignedWingman(i);
				break;
			}
		}
	}
}

void theodChomp()
{
	auto lee = campaign->getWingman(9);
	auto theod = campaign->getWingman(10);
	if (lee && theod) {
		lee->injured = true;
		lee->turnsInjured = 0;
		++lee->totalInjuries;
		++theod->totalKills;
		for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
			if (campaign->getAssignedWingman(i) == lee) {
				campaign->removeAssignedWingman(i);
				break;
			}
		}
	}
}

void arnoldMission()
{
	_injureWingman(3, -50);
	campaign->setFlag(L"ARNOLD_MISSION_AVAILABLE");
}

void leeMission()
{
	_injureWingman(9, -50);
	campaign->setFlag(L"LEE_MISSION_AVAILABLE");
}

void tauranMission()
{
	_injureWingman(4, -50);
	campaign->setFlag(L"TAURAN_MISSION_AVAILABLE");
}

void leeGrounded()
{
	_injureWingman(9, -999);
}

void leeDebriefed()
{
	_injureWingman(9, 1);
}

void arthurLeft()
{
	_injureWingman(5, -999);
}

void exitDialogue()
{
	//saves
	campaign->saveCampaign("saves/autosave.xml");
	//kill the current dialogue
}