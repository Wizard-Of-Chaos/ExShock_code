#include "MissionUtils.h"
#include "GameController.h"
#include "ShipUtils.h"
#include "AttributeReaders.h"
#include "Campaign.h"
#include "IrrlichtUtils.h"
#include "GameFunctions.h"
#include "StatusEffects.h"
#include "LargeShipUtils.h"

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

void _jumpByWing()
{
	s32 ace, reg, acewep, regwep, turr, turrwep;
	gameController->setSpawns(ace, reg, acewep, regwep, turr, turrwep);

	auto playerNode = gameController->getPlayer().get<IrrlichtComponent>()->node;
	spawnScenarioWing(playerNode->getPosition() + getNodeForward(playerNode) * 650.f, randomRotationVector(), reg, regwep, ace, acewep, turr, turrwep);
}

void onAmbushBustCb()
{
	s32 talker = _getLiveWingmanID();
	auto inst = loadWingman(wingMarkers[talker], false);
	gameController->addLargePopup(inst->bustAmbushLine, inst->name);
	_jumpByWing();
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

void theodCollectCb()
{
	auto inst = campaign->createNewShipInstance(24);
	campaign->addShipInstanceToHangar(inst);
	_jumpByWing();
	gameController->addLargePopup("It *is* in working order! I knew it! HHhrrrraaaaaaahhh!", "Theod Tantrus", true);
	gameController->addLargePopup("Bring me my canvas! Bring the prey! I will tear you apart!", "Theod Tantrus", true);
	campaign->setFlag(L"THEOD_MISSION_COMPLETED");
	campaign->setFlag(L"EVENT_AVAILABLE");
}

void leeFinishCb()
{
	campaign->setFlag(L"LEE_MISSION_COMPLETED");
	campaign->setFlag(L"EVENT_AVAILABLE");
}

void arthur1Cb()
{
	gameController->addLargePopup(L"Correct identification. This is indeed the weapon we are looking for. Uploading additional coordinates.", L"ARTHUR");
}
void arthur2Cb()
{
	_jumpByWing();
	gameController->addLargePopup(L"We were anticipated here. Suggest moving quickly toward the next coordinates.", L"ARTHUR");
}
void arthur3Cb()
{
	_jumpByWing();
	gameController->addLargePopup(L"All four weapons have been retrieved. Eliminate opposition and return them to the Chaos Theory for installation.", L"ARTHUR");
	campaign->setFlag(L"ARTHUR_MISSION_COMPLETED");
	campaign->setFlag(L"EVENT_AVAILABLE");
}

void sean1Cb()
{
	_jumpByWing();
	gameController->addLargePopup(L"Damn it. Nothing here. Let's head to the next most likely one.", L"Sean Cooper");
}

void sean2Cb()
{
	gameController->addLargePopup(L"Receiving a transponder signal from nearby debris... it's faint, but it looks to be the Pendulum.", L"Sean Cooper");
	gameController->addLargePopup(L"Sir, it's black box data. The Pendulum ejected logs while it was crashing and burning, it looks like.", L"Sean Cooper");
	gameController->addLargePopup(L"I've got an idea of its velocity. Let's keep moving.");
}

void sean3Cb()
{
	_jumpByWing();
	gameController->addLargePopup(L"Damn, they were waiting for us! That's the Pendulum, alright! Take 'em out!", L"Sean Cooper");
	gameController->addLargePopup(L"I'm reading no power signatures from the ship at all, but it's missing a fighter complement... one last signal.", L"Sean Cooper");
}

void sean4Cb()
{
	_jumpByWing();
	gameController->addLargePopup(L"That crazy son of a... he ejected from his craft? Without a pod?" L"Sean Cooper");
	gameController->addLargePopup(L"We'll have to investigate the data when we get back to the Chaos Theory, sir. We're out of time.", L"Sean Cooper");
	campaign->setFlag(L"SEAN_MISSION_COMPLETED");
	campaign->setFlag(L"EVENT_AVAILABLE");
}

void jamesCb()
{
	gameController->addLargePopup(L"YEEEEEEEESSSSSSSSSSSSS! I consign this monstrous creature to *hell*! Hahahahaha!", L"James Lavovar");
	campaign->setFlag(L"JAMES_MISSION_COMPLETED");
	campaign->setFlag(L"EVENT_AVAILABLE");
}

void tauranCb()
{
	campaign->setFlag(L"TAURAN_MISSION_COMPLETED");
	campaign->setFlag(L"EVENT_AVAILABLE");
}

void catCb()
{
	_jumpByWing();
	gameController->addLargePopup(L"What the..?! It was a trap! Oh no! Commander, take cover as quick as you can! I'm so sorry!", L"Cat Cheadle");
	gameController->triggerShipSpawn([]() {
		auto playerNode = gameController->getPlayer().get<IrrlichtComponent>()->node;
		auto ent = createAlienCarrier(14, playerNode->getPosition() + getNodeForward(playerNode) * 1100.f, randomRotationVector(), 24, 0, 25);
		auto irr = ent.get_mut<IrrlichtComponent>();
		decloakEffect(playerNode->getPosition() + getNodeForward(playerNode) * 1050.f, 400.f);
		});
	campaign->setFlag(L"CAT_MISSION_COMPLETED");
	campaign->setFlag(L"EVENT_AVAILABLE");
}