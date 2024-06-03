#include "SoundSystem.h"
#include "GameController.h"
#include "AudioDriver.h"
#include "PlayerComponent.h"
#include "BulletRigidBodyComponent.h"
#include "IrrlichtComponent.h"
#include "IrrlichtUtils.h"
#include "CrashLogger.h"

void soundSystem()
{
	baedsLogger::logSystem("Audio");

	auto it = audioDriver->curGameSounds.begin();
	while(it != audioDriver->curGameSounds.end()) {
		if (it->src->isFinished()) { //if the sound is finished we're done here
			delete it->src;
			it = audioDriver->curGameSounds.erase(it);
			continue;
		}
		if (it->id.is_alive()) { //if the entity is still alive we need to update the sound accordingly
			auto irr = it->id.get<IrrlichtComponent>(); //we use the irrlicht position...
			auto rbc = it->id.get<BulletRigidBodyComponent>(); //... and the rigid body velocity
			if (irr) { //if we have the position, great - update
				it->src->setPos(irr->node->getAbsolutePosition());
			}
			if (rbc) { //if we have the rigid body, update that too
				it->src->setVel(rbc->rigidBody->getLinearVelocity());
			}
		}
		else { //if it's not alive, we need to waste anything that's looping still, but if it's a regular effect just let it play out
			if (it->src->isLooping()) {
				it->src->setLoop(false); //this will make it finished on the next iteration
			}
		}
		++it;
	}

	//lastly we need to set the listener position and the music / menu positions accordingly
	auto ent = gameController->getPlayer();
	if (!ent.is_alive()) return;
	auto player = ent.get<PlayerComponent>();
	auto rbc = ent.get<BulletRigidBodyComponent>();
	if (!player || !rbc ) return;

	vector3df pos = player->camera->getAbsolutePosition();
	btVector3 vel = rbc->rigidBody->getLinearVelocity();
	audioDriver->setListenerPosition(pos, getNodeUp(player->camera), getNodeForward(player->camera), vel);
}