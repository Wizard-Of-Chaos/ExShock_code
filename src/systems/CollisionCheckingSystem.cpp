#include "CollisionCheckingSystem.h"
#include "GameController.h"
#include "GameStateController.h"
#include "WeaponUtils.h"
#include "btUtils.h"
#include "BulletRigidBodyComponent.h"
#include "Config.h"
#include "CrashLogger.h"
#include "ObstacleComponent.h"
#include "FactionComponent.h"
#include "HangarComponent.h"
#include "GameFunctions.h"

static void projectileCollider(flecs::entity projectile, flecs::entity impacted)
{
	handleProjectileHit(projectile, impacted);
	destroyObject(projectile);
}

static void collisionDamage(flecs::entity A, flecs::entity B, btRigidBody* RBA, btRigidBody* RBB)
{

	if (A.has(gameController->doNotCollideWith(), B) || B.has(gameController->doNotCollideWith(), A)) {
		return;
	}
	btVector3 velDiff = RBA->getLinearVelocity() - RBB->getLinearVelocity();
	btScalar velDiffSquared = velDiff.length2();
	btScalar velA = RBA->getLinearVelocity().length2();
	btScalar velB = RBB->getLinearVelocity().length2();
	btScalar kinetic = 0;
	btScalar minimumVel = 50.f;
	if (velDiffSquared < minimumVel) return;

	//Any "static" object would have a mass of 0, and would have 0 velocity, so in both cases the static
	//object should never be applying the damage.

	//Edit: You know, come to think of it, this means that slamming someone into a large asteroid or station would in fact not deal *any* significant damage.
	//Ship mass isn't high enough to kill itself.
	//This also needs to do math to see if the velocities are aligned. A love tap from a rock shouldn't deal damage if you're going the same direction,
	//but if you're going OPPOSITE the rock you should get crushed into powder.
	if (velA >= velB) kinetic = (velA * RBA->getMass()) / 2;
	else kinetic = (velB * RBB->getMass()) / 2;

	//balancing for later
	kinetic = kinetic / 24000.f;
	if (kinetic < .5f) return;
	if (A.has<HealthComponent>()) {
		A.get_mut<HealthComponent>()->registerDamageInstance(DamageInstance(B, A, DAMAGE_TYPE::IMPACT, kinetic / 2, device->getTimer()->getTime()));
	}
	if (B.has<HealthComponent>()) {
		B.get_mut<HealthComponent>()->registerDamageInstance(DamageInstance(A, B, DAMAGE_TYPE::IMPACT, kinetic / 2, device->getTimer()->getTime()));
	}
}

void collisionCheckingSystem()
{
	baedsLogger::logSystem("Collision");
	int numManifolds = bWorld->getDispatcher()->getNumManifolds();
	btDispatcher* dispatch = bWorld->getDispatcher();
	for (int i = 0; i < numManifolds; ++i) {
		//for some reason it's possible for this to get changed midway through so we're manually checking
		if (i >= dispatch->getNumManifolds()) {
			continue;
		}
		btPersistentManifold* contact = dispatch->getManifoldByIndexInternal(i);
		int numContacts = contact->getNumContacts();
		btCollisionObject* objA = const_cast<btCollisionObject*>(contact->getBody0());
		btCollisionObject* objB = const_cast<btCollisionObject*>(contact->getBody1());
		
		btRigidBody* rbcA = (btRigidBody*)objA;
		btRigidBody* rbcB = (btRigidBody*)objB;

		flecs::entity idA = getIdFromBt(objA);
		flecs::entity idB = getIdFromBt(objB);
		if (!idA.is_alive() || !idB.is_alive()) return;

		bool projA = idA.has<ProjectileInfoComponent>();
		bool projB = idB.has<ProjectileInfoComponent>();

		if (projA && !projB) {
			if (idB.has<ObstacleComponent>() || idB.has<HangarComponent>()) {
				for (s32 j = 0; j < numContacts; ++j) {
					if (contact->getContactPoint(j).getDistance() <= 0.f) {
						projectileCollider(idA, idB);
						break;
					}
				}
			}
			else {
				projectileCollider(idA, idB);
			}
			continue;
		}
		else if (projB && !projA) {
			if (idA.has<ObstacleComponent>() || idA.has<HangarComponent>()) {
				for (s32 j = 0; j < numContacts; ++j) {
					if (contact->getContactPoint(j).getDistance() <= 0.f) {
						projectileCollider(idB, idA);
						break;
					}
				}
			}
			else {
				projectileCollider(idB, idA);
			}
			continue;
		}
		else if (projB && projA) {
			destroyObject(idA);
			destroyObject(idB);
			continue;
		}
		for (int j = 0; j < numContacts; ++j) {
			if (contact->getContactPoint(j).getDistance() >= 0.f) continue;

			if (idA.is_alive() && idB.is_alive()) {
				//projectile cases have been handled. now for impact damage
				collisionDamage(idA, idB, rbcA, rbcB);
			}
		}
	}
}

bool broadCallback::needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const
{
	btCollisionObject* a = static_cast<btCollisionObject*>(proxy0->m_clientObject);
	btCollisionObject* b = static_cast<btCollisionObject*>(proxy1->m_clientObject);

	auto entityA = getIdFromBt(a);
	auto entityB = getIdFromBt(b);
	if (!entityA.is_valid() || !entityB.is_valid()) return true; //something probably went wrong if either of these hits
	//need to check who "owns" these entities

	//ditch collisions between things fired by the same object
	auto idFiredA = entityA.target(gameController->firedBy());
	auto idFiredB = entityB.target(gameController->firedBy());
	if (idFiredA != INVALID_ENTITY_ID && idFiredB != INVALID_ENTITY_ID) {
		if (idFiredA == idFiredB) return false;
	}

	//ditch collisions that are specific to not collide with (turrets on owners)
	if (entityA.has(gameController->doNotCollideWith(), entityB) || entityB.has(gameController->doNotCollideWith(), entityA)) {
		return false;
	}
	//ditch collisions with gas clouds entirely except for when shot
	if (entityA.has<ObstacleDoesNotCollide>() && !entityB.has<ProjectileInfoComponent>()) return false;
	if (entityB.has<ObstacleDoesNotCollide>() && !entityA.has<ProjectileInfoComponent>()) return false;

	//ditch collisions between projectiles
	if (entityA.has<ProjectileInfoComponent>() && entityB.has<ProjectileInfoComponent>()) { 
		return false;
	}

	//ditch collisions between friendlies if that setting is enabled
	if (!cfg->game.toggles[GTOG_FRIENDLY_FIRE]) {
		if (entityA.has<FactionComponent>() && entityB.has<FactionComponent>()) {
			auto facA = entityA.get<FactionComponent>();
			auto facB = entityB.get<FactionComponent>();
			if (facA->type & FACTION_PLAYER && facB->type & FACTION_PLAYER) return false;
		}
	}

	//in all other scenarios return true, we need the collision
	return true;
}