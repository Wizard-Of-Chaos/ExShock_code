#pragma once
#ifndef BTUTILS_H
#define BTUTILS_H
#include "BaseHeader.h"

struct ObstacleDoesNotCollide {};
const f32 DEGENERATE_VECTOR_LENGTH = 0.0000001f;

//Convenience function to get the ID from a bullet collision object.
flecs::entity getIdFromBt(const btCollisionObject* object);
void setIdOnBtObject(btCollisionObject* object, flecs::entity id);

//Converts a Bullet quaternion to Euler angles (in degrees).
void QuaternionToEuler(const btQuaternion& TQuat, btVector3& TEuler);

//Safe normalization for a velocity that dodges degenerate cases.
btVector3 velocitySafeNormalize(btVector3& vel);

/* DIRECTIONAL VECTORS */

//Forward directional vector for a rigid body.
btVector3 getRigidBodyForward(btRigidBody* body);
//Backward directional vector for a rigid body.
btVector3 getRigidBodyBackward(btRigidBody* body);
//Right directional vector for a rigid body.
btVector3 getRigidBodyRight(btRigidBody* body);
//Left directional vector for a rigid body.
btVector3 getRigidBodyLeft(btRigidBody* body);
//Up directional vector for a rigid body.
btVector3 getRigidBodyUp(btRigidBody* body);
//Down directional vector for a rigid body.
btVector3 getRigidBodyDown(btRigidBody* body);

//Returns the angular velocity of the body transformed into local coordinates (X is pitch, Y is yaw, Z is roll).
//NOTE: What the fuck did I write these for? They're not used anywhere...
btVector3 getLocalAngularVelocity(btRigidBody* body);
//Returns the linear velocity of the body transformed into local coordinates (Z is forward/backward, X is left/right, Y is up/down).
//NOTE: What the fuck did I write these for? They're not used anywhere...
btVector3 getLocalLinearVelocity(btRigidBody* body);

//Adds a bullet rigid body to the given entity.
bool initializeBtConvexHull(flecs::entity entityId, btConvexHullShape shape, btVector3& scale, f32 mass, f32 startVel=0, f32 startRotVel=0, NetworkId id=0);
bool initBtRBC(flecs::entity id, btCollisionShape* shape, btVector3& scale, f32 mass, f32 startVel=0, f32 startRotVel=0, NetworkId networkId = 0);

//An extension of the bullet physics world with a helpful function that effectively deletes the world. Otherwise, it's the same.
class BulletPhysicsWorld : public btDiscreteDynamicsWorldMt
{
public:
	BulletPhysicsWorld(btDispatcher* dispatcher, btBroadphaseInterface* broadphasePairCache, btConstraintSolverPoolMt* solverPool, btSequentialImpulseConstraintSolver* solver, btCollisionConfiguration* collisionConfiguration) :
		btDiscreteDynamicsWorldMt(dispatcher, broadphasePairCache, solverPool, solver, collisionConfiguration)
	{
		constraintNum = 0;
	}
	bool bodyLoaded(btRigidBody* body);
	void clearObjects()
	{
		//getBroadphase()->~btBroadphaseInterface();

		m_collisionObjects.clear();
		m_nonStaticRigidBodies.clear();
		m_sortedConstraints.clear();
		m_constraints.clear();
		m_actions.clear();
		m_predictiveManifolds.clear();
	}
	inline static u32 constraintNum; //I'm just going to bank on the assumption I will not have 2 billion constraints in one scenario

	struct _RegisteredConstraint
	{
		_RegisteredConstraint() {
			id = ++BulletPhysicsWorld::constraintNum;
		}
		_RegisteredConstraint(btTypedConstraint* constraint, flecs::entity first, flecs::entity second = INVALID_ENTITY, f32 duration = 0.f)
			: constraint(constraint), entity1(first), entity2(second), currentLifetime(0), duration(duration) {
			if (second == INVALID_ENTITY) singleEntity = true;
			id = ++BulletPhysicsWorld::constraintNum;
		}
		u32 id = 0;
		btTypedConstraint* constraint = nullptr;
		bool singleEntity = false;
		flecs::entity entity1 = INVALID_ENTITY;
		flecs::entity entity2 = INVALID_ENTITY;
		f32 duration = 0.f;
		f32 currentLifetime = 0.f;
	};

	void registerConstraint(btTypedConstraint* constraint, flecs::entity first, flecs::entity second=INVALID_ENTITY, f32 duration=0.f);
	void checkConstraints(f32 dt);
	void removeEntityConstraints(flecs::entity ent);
	void removeConstraintById(u32 id);
	std::list<_RegisteredConstraint> ECS_activeConstraints;
};

#if _DEBUG
/*
* This class is SUPPOSED to be able to make it so that bullet bodies use Irrlicht to draw themselves, but I have yet to get it functional.
*/
class btDebugRenderer : public btIDebugDraw
{
public:
	void setController(GameStateController* ctrl) { controller = ctrl; }
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);

	virtual void drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar dist, int lifeTime, const btVector3& color)
	{

	}
	virtual void reportErrorWarning(const char* warning) {
		printf(warning);
	}
	virtual void draw3dText(const btVector3& location, const char* text) {

	}
	virtual void setDebugMode(int nmode) { mode = nmode; }
	virtual int getDebugMode() const { return mode; }
private:
	GameStateController* controller;
	int mode;
};
#endif

//Used for sphere sweeps with collision avoidance.
//This code ripped directly from Bullet's own source. It's declared in a .cpp file. I don't know why
//it isn't in a damned header, but, y'know, whatever works. Original declaration in btDiscreteDynamicsWorld.cpp
class btClosestNotMeConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
	btCollisionObject* m_me;
	btScalar m_allowedPenetration;
	btOverlappingPairCache* m_pairCache;
	btDispatcher* m_dispatcher;

public:
	btClosestNotMeConvexResultCallback(btCollisionObject* me, const btVector3& fromA, const btVector3& toA, btOverlappingPairCache* pairCache, btDispatcher* dispatcher) : btCollisionWorld::ClosestConvexResultCallback(fromA, toA),
		m_me(me),
		m_allowedPenetration(0.0f),
		m_pairCache(pairCache),
		m_dispatcher(dispatcher)
	{
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		if (convexResult.m_hitCollisionObject == m_me)
			return 1.0f;

		//ignore result if there is no contact response
		if (!convexResult.m_hitCollisionObject->hasContactResponse())
			return 1.0f;

		btVector3 linVelA, linVelB;
		linVelA = m_convexToWorld - m_convexFromWorld;
		linVelB = btVector3(0, 0, 0);  //toB.getOrigin()-fromB.getOrigin();

		btVector3 relativeVelocity = (linVelA - linVelB);
		//don't report time of impact for motion away from the contact normal (or causes minor penetration)
		if (convexResult.m_hitNormalLocal.dot(relativeVelocity) >= -m_allowedPenetration)
			return 1.f;

		return ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
	}

	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		//don't collide with itself
		if (proxy0->m_clientObject == m_me)
			return false;

		///don't do CCD when the collision filters are not matching
		if (!ClosestConvexResultCallback::needsCollision(proxy0))
			return false;
		if (m_pairCache->getOverlapFilterCallback()) {
			btBroadphaseProxy* proxy1 = m_me->getBroadphaseHandle();
			bool collides = m_pairCache->needsBroadphaseCollision(proxy0, proxy1);
			if (!collides)
			{
				return false;
			}
		}

		btCollisionObject* otherObj = (btCollisionObject*)proxy0->m_clientObject;

		if (!m_dispatcher->needsCollision(m_me, otherObj))
			return false;

		//call needsResponse, see http://code.google.com/p/bullet/issues/detail?id=179
		if (m_dispatcher->needsResponse(m_me, otherObj))
		{
#if 0
			///don't do CCD when there are already contact points (touching contact/penetration)
			btAlignedObjectArray<btPersistentManifold*> manifoldArray;
			btBroadphasePair* collisionPair = m_pairCache->findPair(m_me->getBroadphaseHandle(), proxy0);
			if (collisionPair)
			{
				if (collisionPair->m_algorithm)
				{
					manifoldArray.resize(0);
					collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);
					for (int j = 0; j < manifoldArray.size(); j++)
					{
						btPersistentManifold* manifold = manifoldArray[j];
						if (manifold->getNumContacts() > 0)
							return false;
					}
				}
			}
#endif
			return true;
		}

		return false;
	}
};

class btClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
	btClosestNotMeRayResultCallback(btCollisionObject* me, const btVector3& from, const btVector3& to) : btCollisionWorld::ClosestRayResultCallback(from, to)
	{
		m_me = me;
	}

	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if (rayResult.m_collisionObject == m_me)
			return 1.0;

		return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
	}

protected:
	btCollisionObject* m_me;
};

#endif 