#include "MissionUtils.h"
#include "GameController.h"
#include "ShipUtils.h"
#include "AttributeReaders.h"
#include "Campaign.h"
#include "IrrlichtUtils.h"
#include "GameFunctions.h"
#include "StatusEffects.h"

static const s32 _getLiveWingmanID()
{
	s32 ret = 7; //defaults to Steven
	u32 iters = 0;
	while (iters < 12) {
		u32 which = random.unumEx(MAX_WINGMEN_ON_WING);
		if (gameController->getWingman(which).is_alive()) {
			ret = campaign->getAssignedWingman(which)->id;
			break;
		}
		++iters;
	}
	return ret;
}

void onTotalBustCb()
{
	s32 talker = _getLiveWingmanID();
	auto inst = loadWingman(wingMarkers[talker], false);
	gameController->addLargePopup(inst->bustTotalLine, inst->name);
	delete inst;
}

void onSalvageBustCb()
{
	s32 talker = _getLiveWingmanID();
	auto inst = loadWingman(wingMarkers[talker], false);
	gameController->addLargePopup(inst->bustSalvageLine, inst->name);
	delete inst;
}

void onAmbushBustCb()
{
	s32 talker = _getLiveWingmanID();
	auto inst = loadWingman(wingMarkers[talker], false);
	gameController->addLargePopup(inst->bustAmbushLine, inst->name);

	s32 ace, reg, acewep, regwep, turr, turrwep;
	gameController->setSpawns(ace, reg, acewep, regwep, turr, turrwep);
	
	auto playerNode = gameController->getPlayer().get<IrrlichtComponent>()->node;

	spawnScenarioWing(playerNode->getPosition() + getNodeForward(playerNode) * 650.f, randomRotationVector(), reg, regwep, ace, acewep, turr, turrwep);

	delete inst;
}

void onDistressBeaconSuccessCb()
{
	if (gameController->currentScenario->environment() == SECTOR_FLEET_GROUP) {
		gameController->addLargePopup(L"Excellent work, commander. Life support signs are still there. Hold tight while we send a re-activation signal, see if they can get underway." L"Rear Admiral Davis");
		return;
	}
	gameController->addLargePopup(L"Sir, it seems like the frigate could still be operational. We can try and re-activate it, but it'll take time. We'll see what we can do." L"Steven Mitchell");
}

void onDistressBeaconFailCb()
{
	if (gameController->currentScenario->environment() == SECTOR_FLEET_GROUP) {
		gameController->addLargePopup(L"Damn! The frigate's inoperative and we've got no life support signals. You'll need to scuttle it, commander." L"Rear Admiral Davis");
		return;
	}
	gameController->addLargePopup(L"There's no life signs aboard, sir. It would be best to scuttle the frigate." L"Steven Mitchell");

}

void onFinaleHoldoutCb()
{
	gameController->addLargePopup(L"Reading you at the rendezvous, sir. Hold out there while we move to extract you.", L"Steven Mitchell");
}

void arnoldCollectedCb()
{
	gameController->addLargePopup(L"Reading several enemy signatures closing on your position, sir. Sending you a rendezvous point.", L"Steven Mitchell");
}