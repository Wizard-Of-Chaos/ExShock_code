#include "ShipUpdateSystem.h"
#include "AudioDriver.h"
#include "GameController.h"
#include "GameAssets.h"
#include "Config.h"
#include "btUtils.h"
#include "BulletRigidBodyComponent.h"
#include "ShipMovementUtils.h"
#include "CrashLogger.h"
#include "ShipParticleComponent.h"
#include "PowerComponent.h"
#include "FactionComponent.h"
#include "HealthComponent.h"
#include "Scenario.h"
#include "ThrustComponent.h"
#include "IrrlichtComponent.h"
#include "IrrlichtUtils.h"

static void jetOn(IParticleEmitter* jet)
{
	if (!jet) return;
	jet->setMaxParticlesPerSecond(150);
	jet->setMinParticlesPerSecond(50);
}
static void jetOff(IParticleEmitter* jet)
{
	if (!jet) return;
	jet->setMaxParticlesPerSecond(0);
	jet->setMinParticlesPerSecond(0);
}
static void jetPairOn(IParticleEmitter* jet1, IParticleEmitter* jet2)
{
	jetOn(jet1);
	jetOn(jet2);
}

static void jetPairOff(IParticleEmitter* jet1, IParticleEmitter* jet2)
{
	jetOff(jet1);
	jetOff(jet2);
}

static void setPairDir(IParticleEmitter* jet1, IParticleEmitter* jet2, vector3df dir)
{
	if (!jet1 || !jet2) return;
	jet1->setDirection(dir * .02f);
	jet2->setDirection(dir * .02f);
}

static void afterburnerJetOn(IParticleEmitter* engine, ILightSceneNode* light)
{
	if (!engine || !light) return;
	engine->setMaxParticlesPerSecond(500);
	engine->setMinParticlesPerSecond(450);
	light->setRadius(3.f);
}

static void afterburnerJetOff(IParticleEmitter* engine, ILightSceneNode* light)
{
	if (!engine || !light) return;
	engine->setMaxParticlesPerSecond(300);
	engine->setMinParticlesPerSecond(100);
	light->setRadius(1.3f);
}

static bool velocitySafetyCheck(f32 linVelocity, ThrustComponent* thrust, ShipComponent* ship, btVector3 velDir, btVector3 thrustDir, bool friendly)
{
	if (gameController->currentScenario->environment() == SECTOR_FINALE && friendly) return true;

	if (linVelocity >= thrust->getLinMax()) {
		if (std::abs(velDir.angle(thrustDir)) * RADTODEG >= 90.f) {
			return true;
		}
		return thrust->safetyOverride;
	}
	return true;
}

bool angularSafetyCheck(f32 angVelocity, ThrustComponent* thrust, ShipComponent* ship, btVector3 velDir, btVector3 thrustDir, bool friendly)
{
	if (gameController->currentScenario->environment() == SECTOR_FINALE && friendly) return true;

	f32 vel = std::abs(angVelocity);
	if (vel >= thrust->getAngMax()) {
		if (std::abs(velDir.angle(thrustDir)) * RADTODEG >= 90.f) {
			return true;
		}
		return thrust->safetyOverride;
	}
	return true;
}

void shipUpdateSystem(flecs::iter it, ThrustComponent* thrc, ShipComponent* shpc, BulletRigidBodyComponent* rbcs, IrrlichtComponent* irrc, ShipParticleComponent* prtc, PowerComponent* pwrc)
{
	baedsLogger::logSystem("Ship Update (FX and Safety)");

	for (auto i : it) {
		f32 dt = it.delta_time();
		auto ship = &shpc[i];
		auto irr = &irrc[i];
		auto rbc = &rbcs[i];
		auto particles = &prtc[i];
		auto thrust = &thrc[i];
		auto power = &pwrc[i];

		flecs::entity entityId = it.entity(i);
		try {
			if (!it.entity(i).is_alive()) game_throw("Ship update entity is not alive - " + entDebugStr(it.entity(i)) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}

		btRigidBody* body = rbc->rigidBody;

		f32 linVel = std::abs(body->getLinearVelocity().length());
		f32 angVel = std::abs(body->getAngularVelocity().length());
		btVector3 linDir = body->getLinearVelocity();
		btVector3 angDir = body->getAngularVelocity();

		velocitySafeNormalize(linDir);
		velocitySafeNormalize(angDir);
		//Ugly looking, but all these emitters need adjusting every time the ship moves
		IParticleEmitter* up1 =nullptr, *up2=nullptr, *down1=nullptr, *down2=nullptr, 
			*left1=nullptr, *left2=nullptr, *right1=nullptr, *right2=nullptr, *back1=nullptr, *back2=nullptr;

		if(particles->upJetEmit[0]) up1 = particles->upJetEmit[0]->getEmitter();
		if(particles->upJetEmit[1]) up2 = particles->upJetEmit[1]->getEmitter();
		if(particles->downJetEmit[0]) down1 = particles->downJetEmit[0]->getEmitter();
		if(particles->downJetEmit[1]) down2 = particles->downJetEmit[1]->getEmitter();
		if(particles->leftJetEmit[0]) left1 = particles->leftJetEmit[0]->getEmitter();
		if(particles->leftJetEmit[1]) left2 = particles->leftJetEmit[1]->getEmitter();
		if(particles->rightJetEmit[0]) right1 = particles->rightJetEmit[0]->getEmitter();
		if(particles->rightJetEmit[1]) right2 = particles->rightJetEmit[1]->getEmitter();
		if(particles->reverseJetEmit[0]) back1 = particles->reverseJetEmit[0]->getEmitter();
		if(particles->reverseJetEmit[1]) back2 = particles->reverseJetEmit[1]->getEmitter();

		setPairDir(up1, up2, getNodeUp(irr->node));
		setPairDir(down1, down2, getNodeDown(irr->node));
		setPairDir(left1, left2, getNodeLeft(irr->node));
		setPairDir(right1, right2, getNodeRight(irr->node));
		setPairDir(back1, back2, getNodeForward(irr->node));

		jetPairOff(up1, up2);
		jetPairOff(down1, down2);
		jetPairOff(left1, left2);
		jetPairOff(right1, right2);
		jetPairOff(back1, back2);

		bool friendly = true;
		if (entityId.has<FactionComponent>()) {
			if (entityId.get<FactionComponent>()->type == FACTION_HOSTILE) friendly = false;
		}

		if (thrust->moves[STRAFE_DOWN]) {
			if (velocitySafetyCheck(linVel, thrust, ship, linDir, getRigidBodyDown(body), friendly)) {
				jetPairOn(up1, up2);
			}
			else {
				thrust->moves[STRAFE_DOWN] = false;
			}
		}
		if (thrust->moves[STRAFE_UP]) {
			if (velocitySafetyCheck(linVel, thrust, ship, linDir, getRigidBodyUp(body), friendly)) {
				jetPairOn(down1, down2);
			}
			else {
				thrust->moves[STRAFE_UP] = false;
			}
		}
		if (thrust->moves[STRAFE_LEFT]) {
			if (velocitySafetyCheck(linVel, thrust, ship, linDir, getRigidBodyLeft(body), friendly)) {
				jetPairOn(right1, right2);
			}
			else {
				thrust->moves[STRAFE_LEFT] = false;
			}
		}
		if (thrust->moves[STRAFE_RIGHT]) {
			if (velocitySafetyCheck(linVel, thrust, ship, linDir, getRigidBodyRight(body), friendly)) {
				jetPairOn(left1, left2);
			}
			else {
				thrust->moves[STRAFE_RIGHT] = false;
			}
		}
		if (thrust->moves[THRUST_FORWARD]) {
			if (velocitySafetyCheck(linVel, thrust, ship, linDir, getRigidBodyForward(body), friendly)) {
				//turn on jet pairs, maybe? or turn on the engine light more
			}
			else {
				thrust->moves[THRUST_FORWARD] = false;
			}
		}
		if (thrust->moves[THRUST_BACKWARD]) {
			if (velocitySafetyCheck(linVel, thrust, ship, linDir, getRigidBodyBackward(body), friendly)) {
				jetPairOn(back1, back2);
			}
			else {
				thrust->moves[THRUST_BACKWARD] = false;
			}
		}

		if (thrust->moves[THRUST_BOOST] && thrust->curBoostCd >= thrust->boostCd) {
			if (power->getPower(70.f) > 0.f) {
				audioDriver->playGameSound(entityId, "ship_afterburn.ogg");
				thrust->curBoostCd = 0.f;
			}
			else thrust->moves[THRUST_BOOST] = false;
		}
		else {
			thrust->moves[THRUST_BOOST] = false;
		}

		if (thrust->moves[PITCH_UP]) {
			if (angularSafetyCheck(angVel, thrust, ship, angDir, getRigidBodyLeft(body), friendly)) {
				jetPairOn(down2, up1);
			}
			else {
				thrust->moves[PITCH_UP] = false;
			}
		}
		if (thrust->moves[PITCH_DOWN]) {
			if (angularSafetyCheck(angVel, thrust, ship, angDir, getRigidBodyRight(body), friendly)) {
				jetPairOn(down1, up2);
			}
			else {
				thrust->moves[PITCH_DOWN] = false;
			}
		}
		if (thrust->moves[YAW_LEFT]) {
			if (angularSafetyCheck(angVel, thrust, ship, angDir, getRigidBodyDown(body), friendly)) {
				jetPairOn(right2, left1);
			}
			else {
				thrust->moves[YAW_LEFT] = false;
			}
		}
		if (thrust->moves[YAW_RIGHT]) {
			if (angularSafetyCheck(angVel, thrust, ship, angDir, getRigidBodyUp(body), friendly)) {
				jetPairOn(right1, left2);
			}
			else {
				thrust->moves[YAW_LEFT] = false;
			}
		}
		
		if (thrust->moves[ROLL_LEFT]) {
			if (angularSafetyCheck(angVel, thrust, ship, angDir, getRigidBodyForward(body), friendly)) {

			}
			else {
				thrust->moves[ROLL_LEFT] = false;
			}
		}
		if (thrust->moves[ROLL_RIGHT]) {
			if (angularSafetyCheck(angVel, thrust, ship, angDir, getRigidBodyBackward(body), friendly)) {
			}
			else {
				thrust->moves[ROLL_LEFT] = false;
			}
		}
		
		//adjusts engine light
		f32 zPercent = rbc->rigidBody->getLinearVelocity().length() / thrust->linearMaxVelocity;
		zPercent = std::min(zPercent, 2.5f); // 2.5x max velocity
		for (u32 i = 0; i < ship->engineCount; ++i) {
			IVolumeLightSceneNode* engine = particles->engineJetEmit[i];
			if (engine) engine->setScale(vector3df(std::max(zPercent, 3.5f), std::max(5 * zPercent, .15f), std::max(zPercent, 3.5f)));
		}
		if (particles->shield) {
			for (u32 i = 0; i < particles->shield->getMesh()->getMeshBufferCount(); ++i) {
				auto manip = smgr->getMeshManipulator();
				S3DVertex* verts = (S3DVertex*)particles->shield->getMesh()->getMeshBuffer(i)->getVertices();
				u32 alpha = verts[0].Color.getAlpha();
				if (alpha > 0) alpha = lerp(alpha, 0U, it.delta_time());
				manip->setVertexColorAlpha(particles->shield->getMesh()->getMeshBuffer(i), alpha);
			}
		}
		if (entityId.has<HealthComponent>()) {
			auto hp = entityId.get<HealthComponent>();

			if (particles->shield && hp->wasHitLastFrame) {
				for (u32 i = 0; i < particles->shield->getMesh()->getMeshBufferCount(); ++i) {
					auto manip = smgr->getMeshManipulator();
					manip->setVertexColorAlpha(particles->shield->getMesh()->getMeshBuffer(i), 100);
				}
			}
			f32 percent = hp->health / hp->maxHealth;

			if (percent <= .75f && particles->smokePoints[0]) {
				particles->smokePoints[0]->setVisible(true);
			}
			else if (particles->smokePoints[0]) {
				particles->smokePoints[0]->setVisible(false);
			}
			if (percent <= .7f && particles->smokePoints[1]) {
				particles->smokePoints[1]->setVisible(true);
			}
			else if (particles->smokePoints[1]) {
				particles->smokePoints[1]->setVisible(false);
			}
			if (percent <= .65f && particles->smokePoints[2]) {
				particles->smokePoints[2]->setVisible(true);
			}
			else if (particles->smokePoints[2]) {
				particles->smokePoints[2]->setVisible(false);
			}
			if (percent <= .6f && particles->smokePoints[3]) {
				particles->smokePoints[3]->setVisible(true);
			}
			else if (particles->smokePoints[0]) {
				particles->smokePoints[3]->setVisible(false);
			}

			if (percent <= .5f && particles->flamePoints[0]) {
				particles->flamePoints[0]->setVisible(true);
			}
			else if (particles->flamePoints[0]) {
				particles->flamePoints[0]->setVisible(false);
			}
			if (percent <= .45f && particles->flamePoints[1]) {
				particles->flamePoints[1]->setVisible(true);
			}
			else if (particles->flamePoints[1]) {
				particles->flamePoints[1]->setVisible(false);
			}
			if (percent <= .4f && particles->flamePoints[2]) {
				particles->flamePoints[2]->setVisible(true);
			}
			else if (particles->flamePoints[2]) {
				particles->flamePoints[2]->setVisible(false);
			}
			if (percent <= .35f && particles->flamePoints[3]) {
				particles->flamePoints[3]->setVisible(true);
			}
			else if (particles->flamePoints[0]) {
				particles->flamePoints[3]->setVisible(false);
			}

		}
	}
}