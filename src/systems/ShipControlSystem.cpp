#include "ShipControlSystem.h"
#include "GameController.h"
#include "AudioDriver.h"
#include "WeaponUtils.h"
#include "btUtils.h"
#include "GameController.h"
#include "HUDActiveSelection.h"
#include "SensorComponent.h"
#include "BulletRigidBodyComponent.h"
#include "ThrustComponent.h"
#include "ShipMovementUtils.h"
#include "Config.h"
#include "CrashLogger.h"
#include "HardpointComponent.h"
#include "IrrlichtComponent.h"
#include "PlayerComponent.h"
#include "GameFunctions.h"

static void checkSpaceFriction(ThrustComponent* thrustc, vector3df& throttle, vector3df& rotate)
{
	bool thrust = false;
	bool rot = false;
	for (u32 i = 0; i < 7; ++i) {
		if (thrustc->moves[i]) {
			thrust = true;
			break;
		}
	}
	for (u32 i = 7; i < 13; ++i) {
		if (thrustc->moves[i]) {
			rot = true;
			break;
		}
	}
	if (!thrust && cfg->game.toggles[GTOG_LIN_SPACE_FRICTION]) {
		thrustc->moves[STOP_VELOCITY] = true;
		throttle = vector3df(0, 0, 0);
	}
	if (!rot && cfg->game.toggles[GTOG_ANG_SPACE_FRICTION]) {
		thrustc->moves[STOP_ROTATION] = true;
		rotate = vector3df(0, 0, 0);
	}
}

void setVelocity(ThrustComponent* thrust, f32& local, f32& desired, MOVEMENT pos, MOVEMENT neg)
{
	if (std::abs(local) < std::abs(desired)) {
		if (desired > 0) {
			thrust->moves[pos] = true;
		} if (desired < 0) {
			thrust->moves[neg] = true;
		}
	}
	else if (std::abs(local) > std::abs(desired)) {
		if (desired > 0) {
			thrust->moves[neg] = true;
		} if (desired < 0) {
			thrust->moves[pos] = true;
		}
	}
}

void throttleToShip(ThrustComponent* thrustc, btRigidBody* body, vector3df& thrust, vector3df& rot)
{
	f32 desiredZ = thrust.Z * thrustc->linearMaxVelocity;
	f32 desiredY = thrust.Y * thrustc->linearMaxVelocity;
	f32 desiredX = thrust.X * thrustc->linearMaxVelocity;

	btVector3 ang = body->getAngularVelocity();
	btVector3 lin = body->getLinearVelocity();

	btScalar localZ = lin.dot(getRigidBodyForward(body));
	btScalar localX = lin.dot(getRigidBodyRight(body));
	btScalar localY = lin.dot(getRigidBodyUp(body));


	setVelocity(thrustc, localZ, desiredZ, THRUST_FORWARD, THRUST_BACKWARD);
	setVelocity(thrustc, localY, desiredY, STRAFE_UP, STRAFE_DOWN);
	setVelocity(thrustc, localX, desiredX, STRAFE_RIGHT, STRAFE_LEFT);

	f32 desiredPitch = rot.X * thrustc->angularMaxVelocity;
	f32 desiredYaw = rot.Y * thrustc->angularMaxVelocity;
	f32 desiredRoll = rot.Z * thrustc->angularMaxVelocity;

	f32 localPitch = ang.dot(getRigidBodyRight(body));
	f32 localYaw = ang.dot(getRigidBodyUp(body));
	f32 localRoll = ang.dot(getRigidBodyForward(body));

	//std::cout << localPitch << ", " << desiredPitch << std::endl;
	setVelocity(thrustc, localPitch, desiredPitch, PITCH_DOWN, PITCH_UP);
	setVelocity(thrustc, localYaw, desiredYaw, YAW_RIGHT, YAW_LEFT);
	setVelocity(thrustc, localRoll, desiredRoll, ROLL_RIGHT, ROLL_LEFT);
}
static void setPlayerWep(flecs::entity playerId, InputComponent* input, flecs::entity wep, bool firing=true, bool forceReload=false)
{
	if (!wep.is_alive() || !playerId.is_alive()) return;

	auto player = playerId.get<PlayerComponent>();
	auto sensors = playerId.get<SensorComponent>();
	auto playerIrr = playerId.get<IrrlichtComponent>();
	auto irrComp = wep.get<IrrlichtComponent>();

	if (!wep.has<WeaponInfoComponent>() || !player || !sensors || !playerIrr || !irrComp) return;
	auto wepInfo = wep.get<WeaponInfoComponent>();
	auto wepFire = wep.get_mut<WeaponFiringComponent>();
	wepFire->isFiring = firing;
	if (forceReload) {
		if (wepInfo->usesAmmunition && wepInfo->maxClip != wepFire->clip) {
			wepFire->ammunition += wepFire->clip;
			wepFire->clip = 0;
		}
	}
	if (!firing) return;

	//wepInfo->spawnPosition = getWepSpawnPosition(wepInfo, irrComp);
	vector3df target = input->cameraRay.getMiddle();
	ISceneNode* coll = smgr->getSceneCollisionManager()->getSceneNodeFromRayBB(input->cameraRay, ID_IsSelectable);
	bool mouseOverHUD = false;
	//auto aim for aiming at the current active selection
	if (gameController->selectHUD()->crosshair->getAbsolutePosition().isPointInside(input->mousePixPosition)) {
		target = btVecToIrr(gameController->selectHUD()->crosshairTarget);
	}
	vector3df dir = target - wepFire->spawnPosition;
	dir.normalize();
	wepFire->firingDirection = dir;
}

void shipControlSystem(flecs::iter it,
	InputComponent* inc, HardpointComponent* hardsc, ShipComponent* shpc, ThrustComponent* thrc, PlayerComponent* plyc, BulletRigidBodyComponent* rbcs, IrrlichtComponent* irrc, SensorComponent* snsc)
{
	baedsLogger::logSystem("Ship Controls / Input");
	game_world->defer_suspend();
	for(auto i : it) {
		f32 dt = it.delta_time();
		InputComponent* input = &inc[i];
		ShipComponent* ship = &shpc[i];
		PlayerComponent* player = &plyc[i];
		BulletRigidBodyComponent* rbc = &rbcs[i];
		IrrlichtComponent* irr = &irrc[i];
		SensorComponent* sensors = &snsc[i];
		ThrustComponent* thrust = &thrc[i];
		HardpointComponent* hards = &hardsc[i];

		flecs::entity entityId = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("Ship-control entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		//strafing
		thrust->safetyOverride = input->safetyOverride;

		bool fa = cfg->game.toggles[GTOG_CONST_THRUST];

		vector3df throttle;
		vector3df angularThrottle;

		if(input->isKeyDown(cfg->keys.key[IN_THRUST_FORWARDS])) {
			throttle.Z += dt;
			if (!fa) thrust->moves[THRUST_FORWARD] = true;
		}
		if(input->isKeyDown(cfg->keys.key[IN_STRAFE_BACKWARDS])) {
			throttle.Z -= dt;
			if (!fa) thrust->moves[THRUST_BACKWARD] = true;
		}
		throttle.Z = std::clamp(throttle.Z, -1.f, 1.f);

		if(input->isKeyDown(cfg->keys.key[IN_STRAFE_LEFT])) {
			throttle.X -= dt;
			if (!fa) thrust->moves[STRAFE_LEFT] = true;
		}
		if(input->isKeyDown(cfg->keys.key[IN_STRAFE_RIGHT])) {
			throttle.X += dt;
			if (!fa) thrust->moves[STRAFE_RIGHT] = true;
		}
		throttle.X = std::clamp(throttle.X, -1.f, 1.f);
		if(input->isKeyDown(cfg->keys.key[IN_STRAFE_UP])) {
			throttle.Y += dt;
			if (!fa) thrust->moves[STRAFE_UP] = true;
		}
		if(input->isKeyDown(cfg->keys.key[IN_STRAFE_DOWN])) {
			throttle.Y -= dt;
			if (!fa) thrust->moves[STRAFE_DOWN] = true;
		}
		throttle.Y = std::clamp(throttle.Y, -1.f, 1.f);
		if (input->isKeyDown(cfg->keys.key[IN_AFTERBURNER])) {
			thrust->moves[THRUST_BOOST] = true;
		}

		//rotations
		//when in reverse camera mode, we don't accept rotations
		if (input->isKeyDown(cfg->keys.key[IN_ROLL_RIGHT]) && !input->usingReverseCamera) {
			angularThrottle.Z -= dt;
			if (!fa) thrust->moves[ROLL_RIGHT] = true;
		}
		if(input->isKeyDown(cfg->keys.key[IN_ROLL_LEFT]) && !input->usingReverseCamera) {
			angularThrottle.Z += dt;
			if (!fa) thrust->moves[ROLL_LEFT] = true;
		}
		angularThrottle.Z = std::clamp(angularThrottle.Z, -1.f, 1.f);
		if(input->isKeyDown(cfg->keys.key[IN_PITCH_UP]) && !input->usingReverseCamera) {
			angularThrottle.X -= dt;
			if (!fa) thrust->moves[PITCH_UP] = true;
		}
		if(input->isKeyDown(cfg->keys.key[IN_PITCH_DOWN]) && !input->usingReverseCamera) {
			angularThrottle.X += dt;
			if (!fa) thrust->moves[PITCH_DOWN] = true;
		}
		angularThrottle.X = std::clamp(angularThrottle.X, -1.f, 1.f);
		if (input->isKeyDown(cfg->keys.key[IN_YAW_LEFT]) && !input->usingReverseCamera) {
			angularThrottle.Y -= dt;
			if (!fa) thrust->moves[YAW_LEFT] = true;
		}
		if(input->isKeyDown(cfg->keys.key[IN_YAW_RIGHT]) && !input->usingReverseCamera) {
			angularThrottle.Y += dt;
			if (!fa) thrust->moves[YAW_RIGHT] = true;
		}
		angularThrottle.Y = std::clamp(angularThrottle.Y, -1.f, 1.f);

		//STOOOOOOOOOOOOOOOOOOOP
		if (input->isKeyDown(cfg->keys.key[IN_STOP_LIN_MOVEMENT])) {
			thrust->moves[STOP_VELOCITY] = true;
			throttle = vector3df(0, 0, 0);
		}
		if (input->isKeyDown(cfg->keys.key[IN_STOP_ANG_MOVEMENT])) {
			thrust->moves[STOP_ROTATION] = true;
			angularThrottle = vector3df(0, 0, 0);
		}

		if (fa) {
			throttleToShip(thrust, rbc->rigidBody, throttle, angularThrottle);
		}
		checkSpaceFriction(thrust, throttle, angularThrottle);

		input->cameraRay = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(input->mousePixPosition, player->camera);
		if (input->usingReverseCamera) smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(input->mousePixPosition, player->reverseCamera);

		ISceneNode* coll = smgr->getSceneCollisionManager()->getSceneNodeFromRayBB(input->cameraRay, ID_IsSelectable);

		if (input->mouseControlEnabled) {
			vector3df viewpoint = input->cameraRay.getMiddle();
			viewpoint = viewpoint - btVecToIrr(rbc->rigidBody->getCenterOfMassPosition());
			viewpoint.normalize();

			btScalar angle = getRigidBodyForward(rbc->rigidBody).angle(irrVecToBt(viewpoint));
			btVector3 ang = rbc->rigidBody->getAngularVelocity();
			if (angle * RADTODEG >= .8f) {
				turnToDirection(rbc->rigidBody, thrust, irrVecToBt(viewpoint));
				//smoothTurnToDirection(rbc->rigidBody, thrust, irrVecToBt(viewpoint));
			}
			else {
				if(!thrust->moves[ROLL_LEFT] && !thrust->moves[ROLL_RIGHT]) thrust->moves[STOP_ROTATION] = true;
			}

		}
		for (unsigned int i = 0; i < hards->hardpointCount; ++i) {
			flecs::entity wep = hards->weapons[i];
			if (!wep.is_alive()) continue;
			if (input->usingReverseCamera) continue;
			setPlayerWep(entityId, input, wep, input->isKeyDown(cfg->keys.key[IN_FIRE_REGULAR]), input->isKeyDown(cfg->keys.key[IN_RELOAD]));
		}
		if(!input->usingReverseCamera) setPlayerWep(entityId, input, hards->physWeapon, input->isKeyDown(cfg->keys.key[IN_FIRE_PHYS]));
		if(!input->usingReverseCamera) setPlayerWep(entityId, input, hards->heavyWeapon, input->isKeyDown(cfg->keys.key[IN_FIRE_HEAVY]), input->isKeyDown(cfg->keys.key[IN_RELOAD]));
	}
	game_world->defer_resume();
}