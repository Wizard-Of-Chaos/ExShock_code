#include "btUtils.h"
#include "GameStateController.h"
#include "GameController.h"
#include "Networking.h"
#include "NetworkingComponent.h"
#include "BulletRigidBodyComponent.h"
#include "GameFunctions.h"
#include "IrrlichtComponent.h"
#include "CrashLogger.h"

void QuaternionToEuler(const btQuaternion& TQuat, btVector3& TEuler) {
	btScalar x, y, z;

	TQuat.getEulerZYX(z, y, x);
	TEuler.setValue(x, y, z);
	TEuler *= RADTODEG;
}

btVector3 velocitySafeNormalize(btVector3& velocity)
{
	btVector3 retval(velocity);
	if (retval.length2() <= 0) return btVector3(0, 0, 0);
	if (retval.length2() <= DEGENERATE_VECTOR_LENGTH) return retval;
	retval.normalize();
	return retval;
}

bool initBtRBC(flecs::entity id, btCollisionShape* shape, btVector3& scale, f32 mass, f32 startVel, f32 startRotVel, NetworkId networkId)
{
	if (!id.has<IrrlichtComponent>()) return false;

	auto objIrr = id.get<IrrlichtComponent>();
	BulletRigidBodyComponent rbc;
	rbc.shape = shape;
	rbc.shape->setLocalScaling(scale);
	btTransform transform = btTransform();
	transform.setIdentity();
	transform.setOrigin(irrVecToBt(objIrr->node->getPosition()));
	btQuaternion q;
	vector3df rot = objIrr->node->getRotation() * DEGTORAD;
	q.setEulerZYX(rot.Z, rot.Y, rot.X);
	transform.setRotation(q);
	auto motionState = new btDefaultMotionState(transform);

	btVector3 localInertia;
	rbc.shape->calculateLocalInertia(mass, localInertia);
	rbc.rigidBody = new btRigidBody(mass, motionState, rbc.shape, localInertia);
	if (startVel > 0) rbc.rigidBody->setLinearVelocity(getRigidBodyForward(rbc.rigidBody) * startVel);
	if (startRotVel > 0) rbc.rigidBody->setAngularVelocity(irrVecToBt(randomRotationVector()).normalize() * startRotVel);
	if(startVel == 0 && startRotVel==0) rbc.rigidBody->setSleepingThresholds(0, 0);
	setIdOnBtObject(rbc.rigidBody, id);
	bWorld->addRigidBody(rbc.rigidBody);
	id.set<BulletRigidBodyComponent>(rbc);
	
	if (!id.has<NetworkingComponent>() && gameController->isNetworked()) {
		initializeNetworkingComponent(id, 1, networkId);
	}
	id.add<SpawnInvulnerable>();

	return true;
}

bool initializeBtConvexHull(flecs::entity entityId, btConvexHullShape shape, btVector3& scale, f32 mass, f32 startVel, f32 startRotVel, NetworkId networkId)
{
	auto newShape = new btConvexHullShape(shape);
	return initBtRBC(entityId, newShape, scale, mass, startVel, startRotVel, networkId);
}

flecs::entity getIdFromBt(const btCollisionObject* object)
{
	uint32_t bottom = object->getUserIndex();
	uint32_t top = object->getUserIndex2();

	flecs::entity_t id = ((flecs::entity_t)top << 32) | ((flecs::entity_t)bottom);

	flecs::entity ent(game_world->get_world(), id);
	return ent;
}

void setIdOnBtObject(btCollisionObject* object, flecs::entity id)
{
	size_t mask = ((1 << 32) - 1);
	uint32_t bottom = id.id() & mask;
	uint32_t top = (id.id() >> 32) & mask;
	object->setUserIndex(bottom);
	object->setUserIndex2(top);
}

btVector3 getRigidBodyForward(btRigidBody* body)
{
	btVector3 forward(0, 0, 1);
	btQuaternion transRot = body->getCenterOfMassTransform().getRotation();
	return forward.rotate(transRot.getAxis(), transRot.getAngle());
}
btVector3 getRigidBodyBackward(btRigidBody* body)
{
	return -getRigidBodyForward(body);
}
btVector3 getRigidBodyRight(btRigidBody* body)
{
	btVector3 right(1, 0, 0);
	btQuaternion transRot = body->getCenterOfMassTransform().getRotation();
	return right.rotate(transRot.getAxis(), transRot.getAngle());
}
btVector3 getRigidBodyLeft(btRigidBody* body)
{
	return -getRigidBodyRight(body);
}
btVector3 getRigidBodyUp(btRigidBody* body)
{
	btVector3 up(0, 1, 0);
	btQuaternion transRot = body->getCenterOfMassTransform().getRotation();
	return up.rotate(transRot.getAxis(), transRot.getAngle());
}
btVector3 getRigidBodyDown(btRigidBody* body)
{
	return -getRigidBodyUp(body);
}

btVector3 getLocalAngularVelocity(btRigidBody* body)
{
	return body->getAngularVelocity() * body->getWorldTransform().getBasis().transpose();
}

btVector3 getLocalLinearVelocity(btRigidBody* body)
{
	return body->getLinearVelocity() * body->getWorldTransform().getBasis().transpose();
}

#include <iostream> 

void BulletPhysicsWorld::registerConstraint(btTypedConstraint* constraint, flecs::entity first, flecs::entity second, f32 duration)
{
	if (!constraint || first == INVALID_ENTITY) {
		//both of these mean it'd be invalid
#ifdef _DEBUG
		baedsLogger::errLog("Invalid constraint registration!\n");
#endif // _DEBUG
		return;
	}
	_RegisteredConstraint reg(constraint, first, second, duration);
	ECS_activeConstraints.push_back(reg);
	addConstraint(constraint);
#ifdef _DEBUG
	baedsLogger::log("Constraint registered between " + entDebugStr(first) + " and " + entDebugStr(second) + ". Total: " + std::to_string(ECS_activeConstraints.size()) + "\n");
#endif
}

bool BulletPhysicsWorld::bodyLoaded(btRigidBody* body)
{
	int index = m_nonStaticRigidBodies.findLinearSearch(body);
	if (index == -1) return false;
	return true;

}

void BulletPhysicsWorld::checkConstraints(f32 dt)
{
	auto it = ECS_activeConstraints.begin();
	while (it != ECS_activeConstraints.end()) {
		if (!it->entity1.is_alive()) {
			removeConstraint(it->constraint);
			delete it->constraint;
			it = ECS_activeConstraints.erase(it);
			continue;
		}
		if (!it->entity2.is_alive() && !it->singleEntity) {
			removeConstraint(it->constraint);
			delete it->constraint;
			it = ECS_activeConstraints.erase(it);
			continue;
		}
		if (it->duration > 0.f) { //if the duration is 0 this is a perma
			it->currentLifetime += dt;
			if (it->currentLifetime >= it->duration) {
				removeConstraint(it->constraint);
				delete it->constraint;
				it = ECS_activeConstraints.erase(it);
				continue;
			}
		}
		++it;
	}
}

void BulletPhysicsWorld::removeEntityConstraints(flecs::entity ent)
{
	auto it = ECS_activeConstraints.begin();
	while (it != ECS_activeConstraints.end()) {
		if (it->entity1 == ent || it->entity2 == ent) {
			removeConstraint(it->constraint);
			delete it->constraint;
			it = ECS_activeConstraints.erase(it);
			continue;
		}
		++it;
	}
}

void BulletPhysicsWorld::removeConstraintById(u32 id)
{
	auto it = ECS_activeConstraints.begin();
	while (it != ECS_activeConstraints.end()) {
		if (it->id == id) {
			removeConstraint(it->constraint);
			delete it->constraint;
			it = ECS_activeConstraints.erase(it);
			continue;
		}
		++it;
	}
}

#if _DEBUG
void btDebugRenderer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	stateController->addDebugLine(line3df(btVecToIrr(from), btVecToIrr(to)));
}
#endif 