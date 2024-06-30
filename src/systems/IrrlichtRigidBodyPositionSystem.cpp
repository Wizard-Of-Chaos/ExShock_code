#include "IrrlichtRigidBodyPositionSystem.h"
#include "CrashLogger.h"
#include "BulletRigidBodyComponent.h"
#include "IrrlichtComponent.h"
#include "GameFunctions.h"
#include "GameController.h"
#include "GameAssets.h"
#include "Shaders.h"
#include "Config.h"

static void downscale(IrrlichtComponent& irr)
{
	if (irr.downscaled) return;
	if (irr.node->getType() == ESNT_SKY_BOX) return;
	if (irr.node->getType() == ESNT_MESH && irr.botMesh != "") {
		auto node = (IMeshSceneNode*)irr.node;
		bool firstLoad;
		node->setMesh(assets->getMesh(irr.botMesh, firstLoad));
	}
	if (irr.node->getType() == ESNT_PARTICLE_SYSTEM) {
		auto node = (IParticleSystemSceneNode*)irr.node;
		auto emit = node->getEmitter();
		emit->setMinParticlesPerSecond(4);
		emit->setMaxParticlesPerSecond(4);
	}
	if (irr.baseMat != EMT_SOLID && irr.baseMat != EMT_TRANSPARENT_ALPHA_CHANNEL && irr.baseMat != EMT_TRANSPARENT_ADD_COLOR && irr.baseMat != EMT_TRANSPARENT_VERTEX_ALPHA && cfg->vid.toggles[TOG_BUMP]) {
		for (u32 i = 0; i < irr.node->getMaterialCount(); ++i) {
			irr.serializedMats[i] = driver->createAttributesFromMaterial(irr.node->getMaterial(i));
		}
		irr.node->setMaterialType(shaders->getShaderMaterial(SHADE_2LIGHT_NORM)); //this is a solid object and we should downscale it
		for (u32 i = 0; i < irr.node->getMaterialCount(); ++i) {
			SMaterial& mat = irr.node->getMaterial(i);
			//baedsLogger::log(std::to_string(irr.textures[i].size()));
			for (u32 j = 0; j < irr.textures[i].size(); ++j) {
				if (irr.textures[i][j]) {
					//baedsLogger::log("setting tex " + std::to_string(i) + "," + std::to_string(j));
					mat.setTexture(j, irr.textures[i][j]);
				}
			}
		}
	}
	irr.downscaled = true;
}

static void upscale(IrrlichtComponent& irr)
{
	if (!irr.downscaled) return;
	if (irr.node->getType() == ESNT_SKY_BOX) return;
	if (irr.node->getType() == ESNT_MESH && irr.topMesh != "") {
		auto node = (IMeshSceneNode*)irr.node;
		bool firstLoad;
		node->setMesh(assets->getMesh(irr.topMesh, firstLoad));
	}
	if (irr.node->getType() == ESNT_PARTICLE_SYSTEM) {
		auto node = (IParticleSystemSceneNode*)irr.node;
		auto emit = node->getEmitter();
		emit->setMinParticlesPerSecond(15);
		emit->setMaxParticlesPerSecond(45);
	}
	if (irr.baseMat != EMT_SOLID && irr.baseMat != EMT_TRANSPARENT_ALPHA_CHANNEL && irr.baseMat != EMT_TRANSPARENT_ADD_COLOR && irr.baseMat != EMT_TRANSPARENT_VERTEX_ALPHA && cfg->vid.toggles[TOG_BUMP]) {
		for (u32 i = 0; i < irr.node->getMaterialCount(); ++i) {
			driver->fillMaterialStructureFromAttributes(irr.node->getMaterial(i), irr.serializedMats[i]);
			irr.serializedMats[i]->drop();
			irr.serializedMats[i] = nullptr;
		}
		irr.node->setMaterialType(irr.baseMat); //this is a solid object and we should downscale it
		for (u32 i = 0; i < irr.node->getMaterialCount(); ++i) {
			SMaterial& mat = irr.node->getMaterial(i);
			for (u32 j = 0; j < irr.textures[i].size(); ++j) {
				if (irr.textures[i][j]) {
					mat.setTexture(j, irr.textures[i][j]);
				}
			}
		}
	}
	irr.downscaled = false;
}

void irrlichtRigidBodyPositionSystem(flecs::entity e, BulletRigidBodyComponent& rbc, IrrlichtComponent& irr)
{
	baedsLogger::logSystem("Bullet-Irrlicht Interface");
	try {
		if (!e.is_alive()) game_throw("Irr-RBC entity is not alive - " + entDebugStr(e) + "\n");
	}
	catch (gameException e) {
		baedsLogger::errLog(e.what());

	}
	if (!rbc.rigidBody || !e.has<BulletRigidBodyComponent>()) {
		baedsLogger::errLog("No rigid body to update on entity " + entDebugStr(e) + "!\n");
		return;
	}
	if (!irr.node || !e.has<IrrlichtComponent>()) {
		baedsLogger::errLog("No node to update on entity " + entDebugStr(e) + "!\n");
		return;
	}
	if (rbc.timeAlive < gameController->getDt())
		return;

	btTransform motionStateTransform;
	btMotionState* motionState = rbc.rigidBody->getMotionState();
	motionState->getWorldTransform(motionStateTransform);
	irr.node->setPosition(btVecToIrr(motionStateTransform.getOrigin()));
	btVector3 eulerOrientation;
	QuaternionToEuler(motionStateTransform.getRotation(), eulerOrientation);
	irr.node->setRotation(btVecToIrr(eulerOrientation));

	auto player = gameController->getPlayer();
	if (player.is_alive()) {
		f32 dist = irr.node->getAbsolutePosition().getDistanceFromSQ(player.get<IrrlichtComponent>()->node->getAbsolutePosition());
		f32 farDist = (smgr->getActiveCamera()->getFarValue() * smgr->getActiveCamera()->getFarValue()) + (2900*2900);
		if (dist < 16777216) //4km
			upscale(irr);
		else
			downscale(irr);

		if (dist > farDist)
			irr.node->setVisible(false);
		else
			irr.node->setVisible(true);
	}
	//TODO: Re-add the speed check LATER when turrets get debugged properly.
	/*
	if ((rbc->rigidBody->getLinearVelocity().length2() > 4000000 || rbc->rigidBody->getAngularVelocity().length2() > 4096) && !ent.has<ProjectileInfoComponent>()) {
		baedsLogger::errLog("Object " + entDebugStr(ent) + " achieving ludicrous speeds. LIN: " 
			+ std::to_string(rbc->rigidBody->getLinearVelocity().length()) + ", ANG:" + std::to_string(rbc->rigidBody->getAngularVelocity().length()) + "\n");
		destroyObject(ent);
	}
	*/

}