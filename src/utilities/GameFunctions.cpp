#include "GameFunctions.h"
#include "GameController.h"
#include "TurretHardpointComponent.h"
#include "StatTrackerComponent.h"
#include "BulletRigidBodyComponent.h"
#include "PlayerComponent.h"
#include "AITypes.h"
#include "IrrlichtUtils.h"
#include "StationModuleUtils.h"
#include "NodeAnimators.h"
#include "btUtils.h"
#include "ShipUtils.h"
#include "Config.h"
#include "BulletGhostComponent.h"
#include "IrrlichtComponent.h"
#include "AudioDriver.h"
#include "NetworkingComponent.h"
#include "Networking.h"
#include "CrashLogger.h"
#include "GameStateController.h"

//Convenience functions to swap back and forth between irrlicht and bullet vectors.
vector3df btVecToIrr(btVector3 vec)
{
	return vector3df(vec.x(), vec.y(), vec.z());
}

btVector3 irrVecToBt(vector3df vec)
{
	return btVector3(vec.X, vec.Y, vec.Z);
}

//Convenience functions that create a (sorta) random vector in Irrlicht space.
vector3df randomRotationVector()
{
	return vector3df(random.frange(-180.f, 180.f), random.frange(-180.f, 180.f), random.frange(-180.f, 180.f));
}

vector3df randomDirectionalVector()
{
	return vector3df(random.frange(-500.f, 500.f), random.frange(-500.f, 500.f), random.frange(-500.f, 500.f)).normalize();
}

vector3df randomPositionalVector(f32 max)
{
	return vector3df(random.frange(-max, max), random.frange(-max, max), random.frange(-max, max));
}

vector3df getPointInSphere(vector3df center, f32 radius)
{
	f32 dist = 1.5f; // init to greater than 1
	f32 x, y, z;
	do {
		f32 low = -1.f;
		f32 high = 1.f;
		x = random.frange(low, high);
		y = random.frange(low, high);
		z = random.frange(low, high);
		dist = (x * x) + (y* y) + (z* z);
	} while (dist > 1.f);
	vector3df pos(x * radius, y * radius, z * radius);
	return center + pos;
}

vector3df getPointInTorus(vector3df center, f32 innerRadius, f32 outerRadius, vector3df up)
{
	f32 rad = (outerRadius - innerRadius)/2.f;
	f32 radiusToTorusCenter = innerRadius + rad;

	auto orthog = up.rotationToDirection(up);

	orthog *= radiusToTorusCenter;
	orthog += center;
	return getPointInSphere(orthog, rad);
}

vector3df getPointInShell(vector3df center, f32 innerRadius, f32 outerRadius)
{
	if (outerRadius <= innerRadius) return center;
	vector3df point = getPointInSphere(center, outerRadius);
	while (isPointInSphere(point, center, innerRadius)) point = getPointInSphere(center, outerRadius);
	return point;
}

vector3df getPointInCapsule(vector3df start, vector3df end, f32 radius)
{
	vector3df direction = (end - start);
	f32 length = direction.getLength();
	direction = direction.normalize();
	f32 pos = random.frange(0.f, 1.f);
	return getPointInSphere(start + (direction * (pos * length)), radius);
}

vector3df getPointInDisc(vector3df center, f32 radius, vector3df up)
{
	f32 rad = random.frange(0, 1.f) * radius;
	f32 angle = random.frange(0, 6.28f);
	quaternion q;
	q.fromAngleAxis(angle, up);
	vector3df rot;
	q.toEuler(rot);
	rot *= RADTODEG;
	return center + (rot.rotationToDirection() * rad); //thanks, quaternions!
}

vector3df getPointInBoundedPlane(vector3df topLeft, vector3df topRight, vector3df botLeft)
{
	f32 LRdist = topLeft.getDistanceFrom(topRight);
	f32 UDdist = botLeft.getDistanceFrom(topLeft);
	vector3df LRdir = (topRight - topLeft).normalize();
	vector3df UDdir = (botLeft - topLeft).normalize();
	f32 posLR = random.frange(0.f, 1.f) * LRdist;
	f32 posUD = random.frange(0.f, 1.f) * UDdist;

	return topLeft + (LRdir * LRdist) + (UDdir * UDdist);
}

bool isPointInSphere(vector3df& point, vector3df center, f32 radius)
{
	return (pow(point.X - center.X, 2.f) + pow(point.Y - center.Y, 2.f) + pow(point.Z - center.Z, 2.f) < pow(radius, 2));
}

bool isPointCloseToList(vector3df& point, std::vector<vector3df>& points, f32 radius)
{
	for (auto& vec : points) {
		if (isPointInSphere(point, vec, radius)) return true;
	}
	return false;
}

void destroyObject(flecs::entity id)
{
	gameController->markForDeath(id);
}

void destroyObject_real(flecs::entity id, bool finalDelete)
{
	if (!id.is_alive()) return; //huh?
	bool out = false;
	if (!id.has<ProjectileInfoComponent>()) out = true;
	else out = id.has<OutputDeathInfo>(); //only track projectile deaths if specifically said
	/*
	else {
		std::string projlogstr = "Projectile destroyed: " + entDebugStr(id) + " from " + entDebugStr(id.target(flecs::ChildOf));
		if (id.get<ProjectileInfoComponent>()->hit) projlogstr += " [HIT]\n";
		else projlogstr += " [DESPAWN]\n";
		baedsLogger::log(projlogstr);
	}
	*/
	if (out) baedsLogger::log("Destroying " + entDebugStr(id) + "\n");
	if (gameController->hasDeathCallback(id)) {
		if (out) baedsLogger::log("Death callback triggered, ");
		gameController->deathCallbacks[id](id);
	}
	

	else if (id.has<IrrlichtComponent>() && id.has<BulletRigidBodyComponent>()) {
		if (out) baedsLogger::log("No death callback -- default explosion, ");
		auto irrComp = id.get<IrrlichtComponent>();
		auto pos = irrComp->node->getAbsolutePosition();
		auto& scale = irrComp->node->getScale();
		auto avgScale = (scale.X + scale.Y + scale.Z) / 6.f;
		auto rad = irrComp->node->getBoundingBox().getExtent().getLength() * avgScale;
		explode(pos, 3.f, avgScale, rad, 50.f, 800.f);
	}

	if (id.has<StationModuleOwnerComponent>()) {
		if (out) baedsLogger::log("Neutralizing Station|");
		id.get_mut<StationModuleOwnerComponent>()->neutralizeStation();
	}

	if (id.has<StationModuleComponent>()) {
		if(out)  baedsLogger::log("Station Modules|");
		snapOffStationBranch(id);
	}

	if (id.has<HardpointComponent>()) {
		if (out)  baedsLogger::log("Hardpoints|");
		auto hards = id.get_mut<HardpointComponent>();
		for (unsigned int i = 0; i < MAX_HARDPOINTS; ++i) {
			if (hards->weapons->is_alive()) {
				if (finalDelete) destroyObject_real(hards->weapons[i], finalDelete);
				else destroyObject(hards->weapons[i]);
			}
		}
	}

	if (id.has<TurretHardpointComponent>()) {
		if (out)  baedsLogger::log("Neutralizing Turrets|");
		auto turr = id.get_mut<TurretHardpointComponent>();
		for (u32 i = 0; i < MAX_TURRET_HARDPOINTS; ++i) {
			if (turr->turrets[i].is_alive()) {
				if (turr->turrets[i].has<FactionComponent>()) setFaction(turr->turrets[i].get_mut<FactionComponent>(), FACTION_NEUTRAL, 0, 0, false);
			}
		}
	}

	if (id.has<IrrlichtComponent>() && (!id.has<ShipComponent>() || id.has<HangarComponent>())) {
		auto irr = id.get_mut<IrrlichtComponent>();
		if (out)  baedsLogger::log("Node|");
		if (irr) {
			if (irr->node) {
				irr->node->removeAll();
				irr->node->remove();
			}
		}
	}

	if (out)  baedsLogger::log("Constraints|");
	bWorld->removeEntityConstraints(id);

	if (id.has<BulletRigidBodyComponent>()) {
		if (out)  baedsLogger::log("Rigid Body|");
		auto rbc = id.get_mut<BulletRigidBodyComponent>();
		if (rbc->rigidBody) bWorld->removeRigidBody(rbc->rigidBody);
		if (rbc->rigidBody) delete rbc->rigidBody;
		if (rbc->shape) delete rbc->shape;
	}

	if (id.has<BulletGhostComponent>()) {
		if (out)  baedsLogger::log("Ghost Body|");
		auto ghost = id.get_mut<BulletGhostComponent>();
		bWorld->removeCollisionObject(ghost->ghost);
		if (ghost->ghost) delete ghost->ghost;
		if (ghost->shape) delete ghost->shape;
	}

	if (id.has<AIComponent>()) {
		for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
			if (id == gameController->getWingman(i)) {
				auto ai = id.get<AIComponent>();
				gameController->regPopup(ai->AIName, ai->deathLine);
			}
		}
	}
	if (id.has<NetworkingComponent>()) {
		auto net = id.get<NetworkingComponent>();
		destroyNetworkId(id.get<NetworkingComponent>()->networkedId);
		auto it = gameController->priorityAccumulator.begin();
		while (it != gameController->priorityAccumulator.end()) {
			if (it->get()->id == net->networkedId) {
				it = gameController->priorityAccumulator.erase(it);
				continue;
			}
			++it;
		}
		stateController->networkToEntityDict.erase(net->networkedId);
	}

	if (out) baedsLogger::log("Done.\nDestroying id - ");
	id.destruct();
	if (out) baedsLogger::log("destroyed.\n");
}

f32 getCurAiAim(s32 adjust)
{
	s32 lvl = (s32)cfg->game.difficulties[DIF_AIM];
	lvl += adjust;
	if (lvl <= 0) lvl = 1;
	if (lvl > 6) lvl = 6;
	return aiAimDifficulties.at((DIFFICULTY_LEVEL)lvl);
}
u32 getCurAiNum(s32 adjust)
{
	s32 lvl = (s32)cfg->game.difficulties[DIF_WINGNUM];
	lvl += adjust;
	if (lvl <= 0) lvl = 1;
	if (lvl > 6) lvl = 6;
	return aiNumDifficulties.at((DIFFICULTY_LEVEL)lvl);
}
u32 getCurAiBehaviors(s32 adjust)
{
	s32 lvl = (s32)cfg->game.difficulties[DIF_AI_INT];
	lvl += adjust;
	if (lvl <= 0) lvl = 1;
	if (lvl > 6) lvl = 6;
	return aiBehaviorDifficulties.at((DIFFICULTY_LEVEL)lvl);
}
f32 getCurAiResolve(s32 adjust)
{
	s32 lvl = (s32)cfg->game.difficulties[DIF_RESOLVE];
	lvl += adjust;
	if (lvl <= 0) lvl = 1;
	if (lvl > 6) lvl = 6;
	return aiResolveDifficulties.at((DIFFICULTY_LEVEL)lvl);
}

bool initializeDefaultPlayer(flecs::entity shipId)
{
	if (!shipId.has<IrrlichtComponent>()) return false;
	auto shipIrr = shipId.get<IrrlichtComponent>();
	shipId.set_doc_name("Player");

	ISceneNode* target = smgr->addEmptySceneNode(0);
	target->setPosition(shipIrr->node->getPosition());
	vector3df scale = shipIrr->node->getScale();
	ICameraSceneNode* camera = smgr->addCameraSceneNode(target, vector3df(0.f, 5.f, -20.f) * scale, shipIrr->node->getPosition(), ID_IsNotSelectable, true);
	camera->setFarValue(3000.f + ((f32)cfg->vid.renderDist * 1500.f));
	auto camera2 = smgr->addCameraSceneNode(target, vector3df(0.f, 5.f, 25.f) * scale, shipIrr->node->getPosition(), ID_IsNotSelectable, false);
	camera2->setFarValue(3000.f + ((f32)cfg->vid.renderDist * 1500.f));

	shipId.add<InputComponent>();
	InputComponent input;
	for (u32 i = 0; i < KEY_KEY_CODES_COUNT; ++i) {
		input.keysDown[i] = false;
	}
	input.mouseControlEnabled = true;
	input.safetyOverride = false;

	PlayerComponent player;
	player.camera = camera;
	player.reverseCamera = camera2;
	player.target = target;

	shipId.set<InputComponent>(input);
	shipId.set<PlayerComponent>(player);
	gameController->setPlayer(shipId);
	if (gameController->isNetworked() && shipId.has<NetworkingComponent>()) {
		shipId.get_mut<NetworkingComponent>()->priority = 1000;
	}
	return true;
}

void initializeHealth(flecs::entity id, f32 healthpool)
{
	HealthComponent hp;
	hp.health = healthpool;
	hp.maxHealth = healthpool;
	id.set<HealthComponent>(hp);
}
void initializeDefaultHealth(flecs::entity objectId)
{
	initializeHealth(objectId, DEFAULT_MAX_HEALTH);
}

flecs::entity createRadioMarker(vector3df pos, std::string name)
{
	flecs::entity marker = game_world->entity();
	IrrlichtComponent irr;
	marker.set_doc_name(name.c_str());
	irr.node = smgr->addEmptySceneNode(0, ID_IsSelectable);
	irr.node->setPosition(pos);
	irr.node->setName(idToStr(marker).c_str());
	marker.set<IrrlichtComponent>(irr);
	gameController->addContact(marker, false, true);

	return marker;
}

void initializeAI(flecs::entity id, f32 react, f32 coward, f32 aggro, f32 aim, u32 behaviors, AIType* type)
{
	AIComponent ai;

	if (type) ai.aiControls = std::shared_ptr<AIType>(type);
	else ai.aiControls = std::shared_ptr<AIType>(new AIType);

	ai.aggressiveness = aggro;
	ai.aim = aim;
	ai.reactionSpeed = react;
	ai.resolve = coward;
	ai.behaviorFlags = behaviors;
	ai.timeSinceLastStateCheck = random.frange(0.f, 10.f); //This is designed to make it so that the AI don't all check at the same time for framerate purposes
	id.set<AIComponent>(ai);
}

void initializeDefaultAI(flecs::entity id, AIType* type)
{
	initializeAI(id, AI_DEFAULT_REACTION_TIME, getCurAiResolve(), AI_DEFAULT_AGGRESSIVENESS, getCurAiAim(), getCurAiBehaviors(), type);
}

void initializeAceAI(flecs::entity id, AIType* type)
{
	initializeAI(id, .7f, getCurAiResolve(1), 1.5f, getCurAiAim(1), getCurAiBehaviors(1), type); 
}

f32 getDamageDifficulty(f32 dmg, bool player)
{
	DIFFICULTY_LEVEL lvl = cfg->game.difficulties[DIF_ENEMY_DMG];
	if (player) lvl = cfg->game.difficulties[DIF_PLAYER_DMG];
	f32 adjusted = dmg;
	switch (lvl) {
		case DIFFICULTY_LEVEL::GAMES_JOURNALIST:
			adjusted /= 4.f;
			break;
		case DIFFICULTY_LEVEL::LOW:
			adjusted /= 2.f;
			break;
		case DIFFICULTY_LEVEL::NORMAL:
			break;
		case DIFFICULTY_LEVEL::HIGH:
			adjusted *= 2.f;
			break;
		case DIFFICULTY_LEVEL::UNFAIR:
			adjusted *= 4.f;
			break;
		default:
			break;
	}
	return adjusted;
}

void setDamageDifficulty(flecs::entity ship)
{
	if (!ship.is_alive()) {
		baedsLogger::log("Can't set damage values on entity " + entDebugStr(ship) + "\n");
		return;
	}
	if (!ship.has<FactionComponent>() || !ship.has<HardpointComponent>()) {
		std::string errstring = entDebugStr(ship) + " has missing components for damage difficulty: ";
		if (!ship.has<FactionComponent>()) errstring += "Faction ";
		if (!ship.has<HardpointComponent>()) errstring += "Hardpoint ";
		baedsLogger::log(errstring + "\n");
		return;
	}
	auto fac = ship.get<FactionComponent>();
	auto hards = ship.get_mut<HardpointComponent>();
	for (u32 i = 0; i < hards->hardpointCount; ++i) {
		if (!hards->weapons[i].is_alive() || !hards->weapons[i].has<WeaponInfoComponent>()) continue;
		auto wep = hards->weapons[i].get_mut<WeaponInfoComponent>();
		wep->damage = getDamageDifficulty(wep->damage, fac->type == FACTION_PLAYER);
	}
	if (hards->heavyWeapon.is_alive() && hards->heavyWeapon.has<WeaponInfoComponent>()) {
		auto wep = hards->heavyWeapon.get_mut<WeaponInfoComponent>();
		wep->damage = getDamageDifficulty(wep->damage, fac->type == FACTION_PLAYER);
	}
	if (hards->physWeapon.is_alive() && hards->physWeapon.has<WeaponInfoComponent>()) {
		auto wep = hards->physWeapon.get_mut<WeaponInfoComponent>();
		wep->damage = getDamageDifficulty(wep->damage, fac->type == FACTION_PLAYER);
	}
}

void addStatusEffectToEntity(StatusEffect* eff, flecs::entity ent)
{
	if (ent.has<StatusEffectComponent>()) {
		auto cmp = ent.get_mut<StatusEffectComponent>();
		if (eff->unique != STATEFF_NOT_UNIQUE) {
			for (auto& effect : cmp->effects) {
				if (eff->unique == effect->unique) {
					delete eff;
					effect->curDuration = 0.f;
					return;
				}
			}
		}
		cmp->effects.push_back(std::shared_ptr<StatusEffect>(eff));
		return;
	}
	StatusEffectComponent cmp;
	cmp.effects.push_back(std::shared_ptr<StatusEffect>(eff));
	ent.set<StatusEffectComponent>(cmp);
}

void refreshStatusEffectOnEntity(UNIQUE_STATUS_EFFECT effect, flecs::entity ent)
{
	if (!ent.has<StatusEffectComponent>()) return;

	auto cmp = ent.get_mut<StatusEffectComponent>();
	for (auto& eff : cmp->effects) {
		if (eff->unique == effect) {
			eff->curDuration = 0.f;
			return;
		}
	}
}

void explosiveForce(vector3df position, f32 radius, f32 damage, f32 force)
{
	btVector3 center = irrVecToBt(position);
	btSphereShape shape(radius);
	btPairCachingGhostObject ghost;
	btTransform transform;
	transform.setOrigin(center);
	ghost.setCollisionShape(&shape);
	ghost.setWorldTransform(transform);
	bWorld->addCollisionObject(&ghost);
	for (s32 i = 0; i < ghost.getNumOverlappingObjects(); ++i) {
		btCollisionObject* obj = ghost.getOverlappingObject(i);
		flecs::entity objId = getIdFromBt(obj);

		if (!objId.is_alive()) continue;
		if (!objId.has<BulletRigidBodyComponent>()) continue;

		auto body = (btRigidBody*)obj;

		btVector3 dist = body->getCenterOfMassPosition() - center;
		btVector3 dir = dist;
		dir.safeNormalize();

		f32 distfactor = (radius - dist.length()) / radius;
		if (std::abs(distfactor) > 1.f) {
			distfactor = 1.f;
		}
		body->activate(true);
		btVector3 totalForce = dir * (force * distfactor);
		body->applyCentralImpulse(totalForce);
		body->applyTorqueImpulse(totalForce / 10.f);

		f32 finaldmg = damage * distfactor;
		if (finaldmg < 0) finaldmg = 0;
		if (finaldmg > damage) finaldmg = damage;

		if (!objId.has<HealthComponent>()) continue;

		auto hp = objId.get_mut<HealthComponent>();
		if(finaldmg > 0) hp->registerDamageInstance(DamageInstance(INVALID_ENTITY, objId, DAMAGE_TYPE::EXPLOSIVE,
			finaldmg, device->getTimer()->getTime()));
	}
	bWorld->removeCollisionObject(&ghost);
}

void implosiveForce(vector3df position, f32 radius, f32 damage, f32 force)
{
	btVector3 center = irrVecToBt(position);
	btSphereShape shape(radius);
	btPairCachingGhostObject ghost;
	btTransform transform;
	transform.setOrigin(center);
	ghost.setCollisionShape(&shape);
	ghost.setWorldTransform(transform);
	bWorld->addCollisionObject(&ghost);

	for (s32 i = 0; i < ghost.getNumOverlappingObjects(); ++i) {
		btCollisionObject* obj = ghost.getOverlappingObject(i);
		flecs::entity objId = getIdFromBt(obj);

		if (!objId.is_alive()) continue;
		if (!objId.has<BulletRigidBodyComponent>()) continue;

		auto body = (btRigidBody*)obj;

		btVector3 dist = body->getCenterOfMassPosition() - center;
		btVector3 dir = dist;
		dir.safeNormalize();
		dir = -dir;

		f32 distfactor = (radius - dist.length()) / radius;
		if (std::abs(distfactor) > 1.f) {
			distfactor = 1.f;
		}
		body->activate(true);
		btVector3 totalForce = dir * (force * distfactor);
		body->applyCentralImpulse(totalForce);
		body->applyTorqueImpulse(totalForce / 10.f);

		f32 finaldmg = damage * distfactor;
		if (finaldmg < 0) finaldmg = 0;
		if (finaldmg > damage) finaldmg = damage;

		if (!objId.has<HealthComponent>()) continue;

		auto hp = objId.get_mut<HealthComponent>();
		if (finaldmg > 0) hp->registerDamageInstance(DamageInstance(INVALID_ENTITY, objId, DAMAGE_TYPE::EXPLOSIVE,
			finaldmg, device->getTimer()->getTime()));
	}
	bWorld->removeCollisionObject(&ghost);
}

void explode(vector3df position, f32 duration, f32 scale, f32 radius, f32 damage, f32 force, EXPLOSION_TYPE type)
{
	u32 lvl = (u32)cfg->vid.particleLevel;
	++lvl;//if 0

	ISceneNode* node = smgr->addBillboardSceneNode(0, dimension2df(20.f * scale, 20.f * scale), position, ID_IsNotSelectable);
	node->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);

	node->setMaterialFlag(EMF_LIGHTING, false);
	SColorf color(1.f, .9f, 1.f);
	if (type == EXTYPE_TECH) color = SColorf(.7f, .7f, 1.f);
	auto light = smgr->addLightSceneNode(node, vector3df(0,0,0), color, 150.f, ID_IsNotSelectable);
	light->setName("light");

	ISceneNodeAnimator* anim = getExplosionTextureAnim(type);
	node->addAnimator(anim);
	anim->drop();

	auto sphere = smgr->addSphereSceneNode(radius/20, 16, node, ID_IsNotSelectable);
	sphere->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
	sphere->setMaterialFlag(EMF_LIGHTING, false);
	sphere->setName("sphere");

	anim = getExplosionSphereTextureAnim(type);
	sphere->addAnimator(anim);
	anim->drop();

	anim = new CSceneNodeScalePulseAnimator(device->getTimer()->getTime(), vector3df(radius/15.f), vector3df(1.f), 750U, false);
	sphere->addAnimator(anim);
	anim->drop();
	anim = smgr->createDeleteAnimator(600.f);
	sphere->addAnimator(anim);
	anim->drop();

	anim = smgr->createDeleteAnimator((u32)(duration*1000.f) + 3000);
	node->addAnimator(anim);
	light->addAnimator(anim);
	anim->drop();

	if(force > .1f) explosiveForce(position, radius, damage, force);
}
void implode(vector3df position, f32 duration, f32 scale, f32 radius, f32 damage, f32 force, EXPLOSION_TYPE type)
{
	u32 lvl = (u32)cfg->vid.particleLevel;
	++lvl;//if 0

	ISceneNode* node = smgr->addBillboardSceneNode(0, dimension2df(20.f * scale, 20.f * scale), position, ID_IsNotSelectable);
	node->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);

	node->setMaterialFlag(EMF_LIGHTING, false);
	SColorf color(1.f, .9f, 1.f);
	if (type == EXTYPE_TECH) color = SColorf(.7f, .7f, 1.f);
	auto light = smgr->addLightSceneNode(node, vector3df(0, 0, 0), color, 150.f, ID_IsNotSelectable);
	light->setName("light");

	ISceneNodeAnimator* anim = getExplosionTextureAnim(type);
	node->addAnimator(anim);
	anim->drop();

	auto sphere = smgr->addSphereSceneNode(radius / 20, 16, node, ID_IsNotSelectable);
	sphere->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
	sphere->setMaterialFlag(EMF_LIGHTING, false);
	sphere->setName("sphere");

	anim = getExplosionSphereTextureAnim(type);
	sphere->addAnimator(anim);
	anim->drop();

	anim = smgr->createDeleteAnimator((u32)(duration * 1000.f) + 3000);
	node->addAnimator(anim);
	sphere->addAnimator(anim);
	light->addAnimator(anim);
	anim->drop();

	if(force > .1f) implosiveForce(position, radius, damage, force);
}

void particleEffectBill(std::string which, vector3df position, f32 duration, f32 scale, bool light, SColorf lightCol)
{
	ISceneNode* node = smgr->addBillboardSceneNode(0, dimension2df(scale, scale), position, ID_IsNotSelectable);
	node->setMaterialType(EMT_TRANSPARENT_ALPHA_CHANNEL);
	node->setMaterialFlag(EMF_LIGHTING, false);
	auto anim = getTextureAnim(which, 16, false, false);
	node->addAnimator(anim);
	anim->drop();

	anim = smgr->createDeleteAnimator((u32)(duration * 1000.f) + 500);
	node->addAnimator(anim);
	anim->drop();

	if (light) {
		auto light = smgr->addLightSceneNode(node, vector3df(0, 0, 0), lightCol, 20.f * scale, ID_IsNotSelectable);
		light->setName("light");
		light->addAnimator(anim);
	}
}

void decloakEffect(vector3df position, f32 scale)
{
	particleEffectBill("assets/effects/decloak/", position, 5.f, scale, true, SColorf(.1f, .9f, .2f));
	audioDriver->playGameSound(position, "decloak_fighter.ogg", 1.f, 400.f);
}

flecs::entity getEntityFromNetId(NetworkId id)
{
	if (stateController->networkToEntityDict.find(id) == stateController->networkToEntityDict.end()) {
		baedsLogger::errLog("No entity exists for network id " + std::to_string(id) + "\n");
		return INVALID_ENTITY;
	}
	return stateController->networkToEntityDict.at(id);
}