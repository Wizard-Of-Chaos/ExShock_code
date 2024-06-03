#include "SensorUpdateSystem.h"
#include "SensorComponent.h"
#include "SensorCallbackComponent.h"
#include "btUtils.h"
#include "GameController.h"
#include "CrashLogger.h"
#include "BulletGhostComponent.h"
#include "WeaponInfoComponent.h"
#include "AIComponent.h"
#include "IrrlichtComponent.h"
#include "GameFunctions.h"

static void getContacts(const flecs::entity ent, SensorComponent* sens, const FactionComponent* fac)
{
	if (!sens) return; //wtf?
	sens->contacts.clear();
	sens->nearbyOrderableContacts.clear();

	btVector3 pos(0, 0, 0);
	btCollisionObject* sensorObject = nullptr;

	if (ent.has<BulletRigidBodyComponent>()) {
		auto rbc = ent.get<BulletRigidBodyComponent>();
		pos = rbc->rigidBody->getCenterOfMassPosition();
		sensorObject = rbc->rigidBody;
	}
	else if (ent.has<BulletGhostComponent>()) {
		auto ghostobj = ent.get<BulletGhostComponent>();
		sensorObject = ghostobj->ghost;
		pos = ghostobj->ghost->getWorldTransform().getOrigin();
	}
	else if (ent.has<IrrlichtComponent>()) pos = irrVecToBt(ent.get<IrrlichtComponent>()->node->getAbsolutePosition());

	btSphereShape shape(sens->detectionRadius);
	btPairCachingGhostObject ghost; ///ooooooooooooooooooooooooo!
	btTransform transform;
	transform.setOrigin(pos);
	ghost.setCollisionShape(&shape);
	ghost.setWorldTransform(transform);
	bWorld->addCollisionObject(&ghost);


	btVector3 closeHostileDist(0, 0, 0);
	btVector3 closeFriendlyDist(0, 0, 0);
	btVector3 closeDist(0, 0, 0);
	for (s32 i = 0; i < ghost.getNumOverlappingObjects(); ++i) {
		btCollisionObject* obj = ghost.getOverlappingObject(i);
		if (obj == sensorObject) continue;

		flecs::entity objId = getIdFromBt(obj);
		if (!objId.is_alive()) continue;
		if (objId.has<SpawnInvulnerable>()) continue; //just don't even register it if it's invisible
		//todo: rework this to allow for ghost components
		if (!objId.has<BulletRigidBodyComponent>() || !objId.has<FactionComponent>()) continue; //throw out anything without a rigid body comp and a faction comp
		if (objId.has<ProjectileInfoComponent>()) continue;
		if (sens->onlyDetectShips && !objId.has<ShipComponent>()) continue;

		auto objRBC = objId.get<BulletRigidBodyComponent>();
		auto objFac = objId.get<FactionComponent>();

		btVector3 dist = objRBC->rigidBody->getCenterOfMassPosition() - pos;
		btScalar len = dist.length2();

		//create a contact info and throw it in
		ContactInfo info = { objId, objRBC, objFac, dist.length()};
		sens->contacts.push_back(info);

		//update closest hostile, friendly, and general contacts
		if (len > closeDist.length2()) {
			closeDist = dist;
			sens->closestContact = objId;
		}
		if (!fac) continue;
		//if we don't have a faction component, continue
		//otherwise check for hostilities
		if (len > closeHostileDist.length2() && fac->isHostile(objFac)) {
			closeHostileDist = dist;
			sens->closestHostileContact = objId;
			if (ent == gameController->getPlayer()) {
				gameController->triggerCombatMusic();
			}
		}
		if (len > closeFriendlyDist.length2() && fac->isFriendly(objFac)) {
			closeFriendlyDist = dist;
			sens->closestFriendlyContact = objId;
		}

		if (objFac && fac) {
			if (!objFac->isFriendly(fac)) continue;
			if (!objId.has<AIComponent>()) continue; //if it's a friendly and has an AI component that can be ordered, keep track of it
			if (objId.get<AIComponent>()->hasBehavior(AIBHVR_CAN_BE_ORDERED)) sens->nearbyOrderableContacts.push_back(info);
		}
	}

	bWorld->removeCollisionObject(&ghost); //get rid of the sphere used to check
}

void sensorSystem(flecs::iter it, SensorComponent* sns)
{
	baedsLogger::logSystem("Sensors");
	for (auto i : it) {
		auto sens = &sns[i];
		auto entity = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("Sensor entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
			//tier list for sensors:
			//bullet rigid body, bullet ghost body, irrlicht component
			//if it has none of those, this thing should not have sensors and we should just continue
			if (!entity.has<BulletRigidBodyComponent>() && !entity.has<BulletGhostComponent>() && !entity.has<IrrlichtComponent>()) game_throw("Sensor entity missing required components - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		const FactionComponent* fac = nullptr;
		if (entity.has<FactionComponent>()) fac = entity.get<FactionComponent>();

		//scrub the contact list for if something died
		//also todo: check the hostile / friendly contacts for if something is dead
		auto ctctIt = sens->contacts.begin();
		while (ctctIt != sens->contacts.end()) {
			if (!ctctIt->ent.is_alive()) ctctIt = sens->contacts.erase(ctctIt);
			if (ctctIt != sens->contacts.end()) ++ctctIt;
		}

		sens->timeSinceLastUpdate += it.delta_time();
		if (sens->timeSinceLastUpdate >= sens->updateInterval) {
			getContacts(entity, sens, fac);
			sens->timeSinceLastUpdate = 0;
		}
		if (sens->targetContact != INVALID_ENTITY) {
			sens->timeSelected += it.delta_time();
		}
		else {
			sens->timeSelected = 0;
		}
	}
}