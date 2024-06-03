#include "ShipMovementUtils.h"
#include "GameController.h"
#include "GameStateController.h"
#include "ThrustComponent.h"

btVector3 getForceForward(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyForward(body) * thrust->getForward();
}
btVector3 getForceBoost(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyForward(body) * thrust->getBoost() * 100.f;
}
btVector3 getForceBackward(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyBackward(body) * thrust->getBrake();
}
btVector3 getForceRight(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyRight(body) * thrust->getStrafe();
}
btVector3 getForceLeft(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyLeft(body) * thrust->getStrafe();
}
btVector3 getForceUp(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyUp(body) * thrust->getStrafe();
}
btVector3 getForceDown(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyDown(body) * thrust->getStrafe();
}
btVector3 getTorquePitchUp(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyLeft(body) * thrust->getPitch();
}
btVector3 getTorquePitchDown(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyRight(body) * thrust->getPitch();
}
btVector3 getTorqueYawRight(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyUp(body) * thrust->getYaw();
}
btVector3 getTorqueYawLeft(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyDown(body) * thrust->getYaw();
}
btVector3 getTorqueRollRight(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyBackward(body) * thrust->getRoll();
}
btVector3 getTorqueRollLeft(btRigidBody* body, ThrustComponent* thrust)
{
	return getRigidBodyForward(body) * thrust->getRoll();
}

btVector3 getTorqueToStopAngularVelocity(btRigidBody* body, ThrustComponent* thrust)
{
	btVector3 ang = body->getAngularVelocity();
	ang = velocitySafeNormalize(ang);
	if (ang.length2() <= DEGENERATE_VECTOR_LENGTH) return -ang;
	return -ang * ((thrust->getPitch() + thrust->getYaw() + thrust->getRoll()) / 3.f);
}
btVector3 getForceToStopLinearVelocity(btRigidBody* body, ThrustComponent* thrust)
{
	btVector3 lin = body->getLinearVelocity();
	lin = velocitySafeNormalize(lin);
	if (lin.length2() <= DEGENERATE_VECTOR_LENGTH) return -lin;
	return -lin * (thrust->getStrafe() + thrust->getBrake());
}

void turnToDirection(btRigidBody* body, ThrustComponent* thrust, btVector3 dir)
{
	btVector3 right = getRigidBodyRight(body);
	btVector3 left = getRigidBodyLeft(body);
	btVector3 up = getRigidBodyUp(body);
	btVector3 down = getRigidBodyDown(body);
	if (right.dot(dir) > left.dot(dir)) {
		thrust->moves[YAW_RIGHT] = true;
	} else {
		thrust->moves[YAW_LEFT] = true;
	}
	if (up.dot(dir) > down.dot(dir)) {
		thrust->moves[PITCH_UP] = true;
	} else {
		thrust->moves[PITCH_DOWN] = true;
	}
}

void smoothTurnToDirection(btRigidBody* body, ThrustComponent* thrust, btVector3 dir, bool print)
{
	btScalar angle = getRigidBodyForward(body).angle(dir);
	btVector3 ang = body->getAngularVelocity();
	if (angle > ang.length()) {
		turnToDirection(body, thrust, dir);
	} else {
		thrust->moves[STOP_ROTATION] = true;
	}
}

void drunkTurnToDirection(btRigidBody* body, ThrustComponent* thrust, btVector3 dir)
{
	btScalar angle = getRigidBodyForward(body).angle(dir) * RADTODEG;
	if (angle > 30.f) turnToDirection(body, thrust, dir);
	else thrust->moves[STOP_ROTATION] = true;
}

void goToPointPrecise(btRigidBody* body, ThrustComponent* thrust, btVector3 dest)
{
	btVector3 shipPos = body->getCenterOfMassPosition();
	btVector3 path = dest - shipPos;
	btVector3 dir = path.normalized();
	btScalar angle = getRigidBodyForward(body).angle(dir) * RADTODEG;
	smoothTurnToDirection(body, thrust, dir);
	if (angle < 20.f) {
		btScalar brakePower = thrust->brake + thrust->strafe;
		btVector3 velocity = body->getLinearVelocity();
		btScalar timeToStop = velocity.length() / brakePower; //time required to stop in seconds
		btScalar timeToArrive = path.length() / velocity.length(); //time to arrive based on the current path
		if (timeToStop >= timeToArrive) { //You ever just write something so simple that you don't understand why it was such a PITA to get correct?

			thrust->moves[STOP_VELOCITY] = true;
		} else {
			thrust->moves[THRUST_FORWARD] = true;
		}
	}
	else {
		thrust->moves[STOP_VELOCITY] = true;
	}
}

void goToPointVague(btRigidBody* body, ThrustComponent* thrust, btVector3 dest)
{
	btVector3 shipPos = body->getCenterOfMassPosition();
	btVector3 path = dest - shipPos;
	btVector3 dir = path.normalized();
	btScalar angle = getRigidBodyForward(body).angle(dir) * RADTODEG;
	drunkTurnToDirection(body, thrust, dir);

	if (path.length() < 45.f) thrust->moves[STOP_VELOCITY] = true; 
	if (angle < 50.f) thrust->moves[THRUST_FORWARD] = true;
}

void goToPointPilot(btRigidBody* body, ThrustComponent* thrust, btVector3 dest, f32 linPrecision, f32 fastAngle, f32 slowAngle, bool accountForVelocity)
{
	btVector3 shipPos = body->getCenterOfMassPosition();
	btVector3 path = dest - shipPos;
	btVector3 dir = path;
	dir.safeNormalize();
	btScalar angle = getRigidBodyForward(body).angle(dir) * RADTODEG;
	//need to turn towards the direction - it's ok if we overshoot a little, just so long as it's fast
	if (angle > fastAngle) {
		turnToDirection(body, thrust, dir);
	}
	else if (angle > slowAngle) {
		smoothTurnToDirection(body, thrust, dir);
	}
	else {
		thrust->moves[STOP_ROTATION] = true;
	}
	//smooth turn on smaller angles, on large angles just hit the jets and we'll correct shortly
	btVector3 linearVelocity = body->getLinearVelocity();
	f32 length = linearVelocity.length();
	//we use the same logic as drunk turn but with a twist

	if (!accountForVelocity) length = 0;

	if (path.length() < linPrecision + (length * .85f)) thrust->moves[STOP_VELOCITY] = true;
	if (angle >= 80.f) thrust->moves[STOP_VELOCITY] = true;
	if (angle < 40.f) thrust->moves[THRUST_FORWARD] = true;

	//so now we're thrusting forward (or not) but we need to break the velocity down to adjust it with strafe values
	//this breaks the velocity down to x, y, and z
	btVector3 velocity = body->getWorldTransform().getBasis().transpose() * linearVelocity;
	//velocity is now in local coords
	//need to find out how much we're off by
	btVector3 right = getRigidBodyRight(body);
	btVector3 left = getRigidBodyLeft(body);
	btVector3 up = getRigidBodyUp(body);
	btVector3 down = getRigidBodyDown(body);
	bool isRight = false;
	bool isUp = false;
	if (right.dot(dir) > left.dot(dir)) {
		isRight = true;
	}
	if (up.dot(dir) > down.dot(dir)) {
		isUp = true;
	}
	//know which directions we NEED to move, now adjust velocty if it's AGAINST that movement
	if (isRight) { //if it's right and we're moving left, correct that
		if (velocity.x() < -5.f) thrust->moves[STRAFE_RIGHT] = true;
	}
	else { //ditto inverse
		if (velocity.x() > 5.f) thrust->moves[STRAFE_LEFT] = true;
	}

	if (isUp) { //if it's up and we're moving down, correct that
		if (velocity.y() < -5.f) thrust->moves[STRAFE_UP] = true;
	}
	else { //ditto inverse
		if (velocity.y() > 5.f) thrust->moves[STRAFE_DOWN] = true;
	}
}

const btCollisionObject* obstacleOnVelocityVector(btRigidBody* rb)
{
	if (!bWorld || !rb) return nullptr;
	
	btVector3 velocity = rb->getLinearVelocity();
	const btVector3& pos = rb->getCenterOfMassPosition();
	btVector3 futurePos = pos + (velocity * 5.f); //where will it be five seconds from now

	return obstacleOnPath(rb, futurePos);
}

const btCollisionObject* obstacleOnPath(btRigidBody* rb, const btVector3& target)
{
	if (!bWorld || !rb) return nullptr;

	const btVector3& pos = rb->getCenterOfMassPosition();
	btClosestNotMeConvexResultCallback cb = btClosestNotMeConvexResultCallback(rb, pos, target, bWorld->getPairCache(), bWorld->getDispatcher());

	btSphereShape shape = btSphereShape(rb->getCollisionShape()->getLocalScaling().x() + 5.f);

	btTransform from(rb->getOrientation(), pos);
	btTransform to(rb->getOrientation(), target);
	to.setOrigin(target);
	bWorld->convexSweepTest(&shape, from, to, cb);

	if (cb.hasHit()) return cb.m_hitCollisionObject;
	return nullptr;
}

const btVector3 pathAroundObstacle(const btRigidBody* body, const btCollisionObject* obstacle, const btVector3& target)
{
	if (!body || !obstacle) return target; //wtf?

	btVector3 boxMin, boxMax, bodyPos, obstaclePos;
	bodyPos = body->getCenterOfMassPosition();
	obstaclePos = obstacle->getWorldTransform().getOrigin();

	obstacle->getCollisionShape()->getAabb(obstacle->getWorldTransform(), boxMin, boxMax);

	btVector3& closer = boxMin;
	//we don't care about details, we just want to know which is closer
	if (bodyPos.distance2(boxMin) > bodyPos.distance2(boxMax)) closer = boxMax;

	btScalar multVal = body->getCollisionShape()->getLocalScaling().x() * 5.f;
	btVector3 directionAway = (obstaclePos - closer).normalized();
	btVector3 pathPoint = closer + (directionAway * (multVal + 15.f));

	return pathPoint;
}