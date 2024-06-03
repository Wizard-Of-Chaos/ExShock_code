#include "ThrustSystem.h"
#include "ShipMovementUtils.h"
#include "HealthComponent.h"
#include "CrashLogger.h"
#include "GameController.h"
#include "Scenario.h"
#include "ThrustComponent.h"
#include "BulletRigidBodyComponent.h"
#include "IrrlichtComponent.h"
#include "IrrlichtUtils.h"

void thrustSystem(flecs::iter it, ThrustComponent* thrc, HealthComponent* hpc, BulletRigidBodyComponent* rbcs, IrrlichtComponent* irrc)
{
	baedsLogger::logSystem("Thrust");

	for (auto i : it) {
		flecs::entity entityId = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("Thrust entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		auto thrust = &thrc[i];
		auto hp = &hpc[i];
		auto rbc = &rbcs[i];
		auto irr = &irrc[i];

		f32 dt = it.delta_time();
		btRigidBody* body = rbc->rigidBody;

		btVector3 torque(0, 0, 0);
		btVector3 force(0, 0, 0);

		if (thrust->moves[THRUST_FORWARD]) {
			force += getForceForward(body, thrust);
		}
		if (thrust->moves[THRUST_BACKWARD]) {
			force += getForceBackward(body, thrust);
		}
		if (thrust->moves[THRUST_BOOST]) {
			force += getForceBoost(body, thrust);
		}
		if (thrust->moves[STRAFE_LEFT]) {
			force += getForceLeft(body, thrust);
		}
		if (thrust->moves[STRAFE_RIGHT]) {
			force += getForceRight(body, thrust);
		}
		if (thrust->moves[STRAFE_UP]) {
			force += getForceUp(body, thrust);
		}
		if (thrust->moves[STRAFE_DOWN]) {
			force += getForceDown(body, thrust);
		}

		if (thrust->moves[PITCH_UP]) {
			torque += getTorquePitchUp(body, thrust);
		}
		if (thrust->moves[PITCH_DOWN]) {
			torque += getTorquePitchDown(body, thrust);
		}
		if (thrust->moves[YAW_LEFT]) {
			torque += getTorqueYawLeft(body, thrust);
		}
		if (thrust->moves[YAW_RIGHT]) {
			torque += getTorqueYawRight(body, thrust);
		}
		if (thrust->moves[ROLL_LEFT]) {
			torque += getTorqueRollLeft(body, thrust);
		}
		if (thrust->moves[ROLL_RIGHT]) {
			torque += getTorqueRollRight(body, thrust);
		}

		if (thrust->moves[STOP_ROTATION]) {
			torque += getTorqueToStopAngularVelocity(body, thrust);
		}
		if (thrust->moves[STOP_VELOCITY]) {
			force += getForceToStopLinearVelocity(body, thrust);
		}

		//note: riiiight about here it should handle particle effects for entities. no reason to have a separate system, why loop twice?

		if (!thrust->nonstopThrust) {
			for (u32 i = 0; i < MAX_MOVEMENTS; ++i) {
				thrust->moves[i] = false;
			}
		}

		rbc->rigidBody->applyTorqueImpulse(torque * dt);
		rbc->rigidBody->applyCentralImpulse(force * dt);
		thrust->curBoostCd += dt;
		if (thrust->curBoostCd >= thrust->boostCd) thrust->curBoostCd = thrust->boostCd;
		//over-speed damage here
		f32 linVel = body->getLinearVelocity().length();
		f32 angVel = body->getAngularVelocity().length();

		if (linVel > thrust->getLinMax() + 1.f && gameController->currentScenario->environment() != SECTOR_FINALE) {
			f32 over = linVel - thrust->linearMaxVelocity;
			f32 dmg = (over * thrust->velocityTolerance * dt);
			hp->registerDamageInstance(DamageInstance(INVALID_ENTITY, entityId, DAMAGE_TYPE::VELOCITY, dmg, device->getTimer()->getTime()));
		}
		if (angVel > thrust->getAngMax() + .2f && gameController->currentScenario->environment() != SECTOR_FINALE) {
			f32 over = angVel - thrust->angularMaxVelocity;
			f32 dmg = (over * thrust->velocityTolerance * dt);
			hp->registerDamageInstance(DamageInstance(INVALID_ENTITY, entityId, DAMAGE_TYPE::VELOCITY, dmg, device->getTimer()->getTime()));

		}
	}
}