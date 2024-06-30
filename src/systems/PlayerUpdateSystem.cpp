#include "PlayerUpdateSystem.h"
#include "GameController.h"
#include "BulletRigidBodyComponent.h"
#include "FactionComponent.h"
#include "SensorComponent.h"
#include "HUDHeader.h"
#include "HUDPopup.h"
#include "CrashLogger.h"
#include "IrrlichtComponent.h"
#include "PlayerComponent.h"
#include "IrrlichtUtils.h"
#include "GameFunctions.h"
#include <cmath>

//Updates the camera. This is done by having an empty scene node maintain the exact position of the player. The empty node
//(the "target" node in playerComponent) does not match the rotation exactly. Instead, it slerps between its current rotation
//and the player's actual rotation, so the camera is able to to actually rotate. This gives the effect of smooth turning as well as
//visibly showing when you're moving or not. Also updates with velocity.
void cameraUpdate(PlayerComponent* player, ISceneNode* playerShip, btRigidBody* body)
{
	ISceneNode* targetnode = player->target;
	ICameraSceneNode* camera = player->camera;

	if(!gameController->finishAnim())
		targetnode->setPosition(playerShip->getPosition());

	vector3df targetForward = getNodeForward(targetnode);
	vector3df shipForward = getNodeForward(playerShip);

	f32 angle = irrVecToBt(targetForward).angle(irrVecToBt(shipForward));
	f32 rollAngle = irrVecToBt(getNodeRight(targetnode)).angle(irrVecToBt(getNodeRight(playerShip)));
	vector3df axis = shipForward.crossProduct(targetForward);

	quaternion nodeRot(playerShip->getRotation() * DEGTORAD);
	quaternion targetRot(targetnode->getRotation() * DEGTORAD);

	f32 slerp = (abs(angle) + abs(rollAngle));
	quaternion desiredRot;
	vector3df targetRotVec(0, 0, 0);
	desiredRot.slerp(targetRot, nodeRot, player->slerpFactor * (slerp * 5.f)); //remove the magic number here later
	desiredRot.toEuler(targetRotVec);
	targetRotVec *= RADTODEG;

	targetnode->setRotation(targetRotVec);

	vector3df targetUp = getNodeUp(targetnode);
	camera->setUpVector(targetUp);
	vector3df vel = btVecToIrr(body->getLinearVelocity());
	f32 len = std::min(vel.getLength(), 650.f);
	vel.setLength(len);
	vector3df target;
	if (!gameController->finishAnim() && !gameController->startAnim())
		target = playerShip->getPosition() + (getNodeUp(playerShip) * 5.f) + vel * player->velocityFactor;
	else 
		target = playerShip->getPosition();
	camera->setTarget(target);

	player->reverseCamera->setUpVector(targetUp);
	f32 dist = (target - camera->getAbsolutePosition()).getLength();
	vector3df targNorm = target;
	targNorm.normalize();
	vector3df reverseTarget = playerShip->getPosition() + (getNodeUp(playerShip) * 5.f) + vel * player->velocityFactor;
	//the reverse target here is just a phantom target at the same distance and vector
	player->reverseCamera->setTarget(reverseTarget);

	f32 defaultFOV = PI / 2.5f;

	f32 targetFOV = defaultFOV * 1 + (.0005f * len);
	camera->setFOV(lerp(camera->getFOV(), targetFOV, .025f));
}


void playerUpdateSystem(flecs::entity player, IrrlichtComponent& irr, PlayerComponent& ply, BulletRigidBodyComponent& rbc)
{
	baedsLogger::logSystem("Player Update / HUD");
	try {
		if (!player.is_alive()) game_throw("Player entity is not alive, wtf? " + entDebugStr(player) + "\n");
	}
	catch (gameException e) {
		baedsLogger::errLog(e.what());
		return;
	}
	ply.timeSinceLastOrder += gameController->getDt();
	ply.inputTimeDelay += gameController->getDt();
	//camera work
	cameraUpdate(&ply, irr.node, rbc.rigidBody);
}