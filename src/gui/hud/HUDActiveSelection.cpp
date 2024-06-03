#include "HUDActiveSelection.h"
#include "GameController.h"
#include "GameStateController.h"
#include "SensorComponent.h"
#include "HUDContact.h"
#include "GameAssets.h"
#include "Config.h"
#include "BulletGhostComponent.h"
#include "AudioDriver.h"
#include "HealthComponent.h"
#include "WeaponInfoComponent.h"
#include "HardpointComponent.h"
#include "GameFunctions.h"
#include "IrrlichtComponent.h"
#include "PlayerComponent.h"

HUDActiveSelection::HUDActiveSelection(IGUIElement* root) : HUDElement(root)
{
	selectGUI = guienv->addImage(assets->getTexture("assets/hud/neutralSelection.png"), position2di(0, 0), root);
	name = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), dimension2du(512, 128)), false, false, root);
	dist = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), dimension2du(128, 128)), false, false, root);
	objType = guienv->addStaticText(L"", rect<s32>(position2di(0, 0), dimension2du(128, 128)), false, false, root);
	setHUDTextSmall(name);
	name->setNotClipped(true);
	setHUDTextSmall(dist);
	setHUDTextSmall(objType);
	objType->setNotClipped(true);
	crosshair = guienv->addImage(assets->getTexture("assets/hud/crosshair.png"), position2di(-200, -200), root);

	selectHP = guienv->addImage(rect<s32>(position2di(0, 0), dimension2du(128,8)), root);
	selectHP->setImage(assets->getTexture("assets/hud/bar_health-shields.png"));

	auto lazy = guienv->addImage(rect<s32>(vector2di(0, 0), dimension2du(128, 8)), selectHP);
	lazy->setImage(assets->getTexture("assets/hud/bar_flash.png"));
	scaleAlign(lazy);

	auto hpFill = guienv->addImage(rect<s32>(position2di(0, 0), dimension2du(128, 8)), selectHP);
	hpFill->setImage(assets->getTexture("assets/hud/bar_fill_health.png"));
	hp = new FillBar(selectHP, hpFill, BARFILLDIR::RIGHT, L"", lazy);

	selectSP = guienv->addImage(rect<s32>(position2di(0, 0), dimension2du(128, 8)), root);
	selectSP->setImage(assets->getTexture("assets/hud/bar_health-shields.png"));

	lazy = guienv->addImage(rect<s32>(vector2di(0, 0), dimension2du(128, 8)), selectSP);
	lazy->setImage(assets->getTexture("assets/hud/bar_flash.png"));
	scaleAlign(lazy);

	auto spFill = guienv->addImage(rect<s32>(position2di(0, 0), dimension2du(128, 8)), selectSP);
	spFill->setImage(assets->getTexture("assets/hud/bar_fill_shields.png"));
	sp = new FillBar(selectSP, spFill, BARFILLDIR::RIGHT, L"", lazy);
	selectGUI->setVisible(false);
	name->setVisible(false);
	crosshair->setVisible(false);
	objType->setVisible(false);
	hp->togVis(false);
	sp->togVis(false);
	//selectHP->setVisible(false);
	//selectSP->setVisible(false);
	setType();
}

HUDActiveSelection::~HUDActiveSelection()
{
	selectGUI->remove();
	name->remove();
	crosshair->remove();
	delete hp;
	delete sp;
	//selectHP->remove();
	//selectSP->remove();
	objType->remove();
}

void HUDActiveSelection::setType()
{
	if (selected == INVALID_ENTITY) return;
	ITexture* img = assets->getTexture("assets/hud/neutralSelection.png");

	if (selected.has<FactionComponent>()) {
		auto fac = selected.get<FactionComponent>();
		if (fac->type == FACTION_HOSTILE) img = assets->getTexture("assets/hud/hostileSelection.png");
		else if (fac->type == FACTION_PLAYER) img = assets->getTexture("assets/hud/friendlySelection.png");
		else if (fac->type == FACTION_UNCONTROLLED) img = assets->getTexture("assets/hud/hazardSelection.png");
	}

	if (gameController->contactTracked(selected)) {
		auto ctct = gameController->getContact(selected);
		if (ctct->isObjective) img = assets->getTexture("assets/hud/objectiveSelection.png");
		else if (ctct->isRadio) img = assets->getTexture("assets/hud/radioSelection.png");
	}

	selectGUI->setImage(img);
	return;
}

void HUDActiveSelection::setVisible(bool vis)
{
	selectGUI->setVisible(vis);
	name->setVisible(vis);
	dist->setVisible(vis);
	objType->setVisible(vis);
	if (selected != INVALID_ENTITY) {
		if (selected.has<BulletRigidBodyComponent>() || selected.has<BulletGhostComponent>()) crosshair->setVisible(vis);
		else crosshair->setVisible(false);
	}
	else crosshair->setVisible(false);

	selectHP->setVisible(vis);
	selectSP->setVisible(vis);
	setType();
	if (vis) {
		if (selected.has<FactionComponent>()) {
			auto fac = selected.get<FactionComponent>();
			if (fac->type == FACTION_HOSTILE) audioDriver->playMenuSound("selection_hostile.ogg");
			else if (fac->type == FACTION_PLAYER) audioDriver->playMenuSound("selection_friendly.ogg");
			else audioDriver->playMenuSound("selection.ogg");

		}
		else {
			audioDriver->playMenuSound("selection.ogg");
		}
	}
}

void HUDActiveSelection::updateElement(flecs::entity playerId)
{
	auto player = gameController->getPlayer().get<PlayerComponent>();
	auto input = gameController->getPlayer().get<InputComponent>();
	auto playerIrr = gameController->getPlayer().get<IrrlichtComponent>();
	auto sensors = gameController->getPlayer().get_mut<SensorComponent>();
	auto rbc = gameController->getPlayer().get<BulletRigidBodyComponent>();

	ICameraSceneNode* camera = player->camera;
	if (input->usingReverseCamera) camera = player->reverseCamera;

	ISceneCollisionManager* coll = smgr->getSceneCollisionManager();
	line3df ray = input->cameraRay;

	if (!sensors->targetContact.is_alive()) { //Check to see if the entity still exists
		sensors->targetContact = INVALID_ENTITY;
	}

	if (input->isKeyDown(cfg->keys.key[IN_SELECT_TARGET]) && selectTimer >= timeBetweenSelects) { //Uses the input component to hurl out a ray selecting anything in its path
		bool mouseOverHUD = false;
		flecs::entity oldTarget = sensors->targetContact;
		flecs::entity newTarget = INVALID_ENTITY;
		for (auto& [id, hud] : gameController->allContacts()) {
			if (!hud) continue;
			if (hud->contactView->getAbsolutePosition().isPointInside(input->mousePixPosition)) { //select any HUD thingy first
				mouseOverHUD = true;
				flecs::entity ent(game_world->get_world(), id);
				sensors->targetContact = ent;
				newTarget = ent;
				selected = ent;
			}
		}
		ISceneNode* selection = nullptr;
		if (mouseOverHUD && sensors->targetContact.has<IrrlichtComponent>()) selection = sensors->targetContact.get<IrrlichtComponent>()->node;
		else selection = coll->getSceneNodeFromRayBB(ray, ID_IsSelectable);

		if (selection) {
			//for some reason this had an extra check? selection->getID() != -1
			if (selection != playerIrr->node && !!!(selection->getID() & ID_IsNotSelectable)) { //extremely angry condition
				flecs::entity id = strToId(std::string(selection->getName()));
				if (id.is_alive()) {
					sensors->targetContact = id;
					newTarget = id;
				}
				std::wstring widestr = wstr(id.doc_name());
				name->setText(widestr.c_str());
				selected = id;
				setVisible();
				selectTimer = 0.f;
			}
		}
		else {
			selected = INVALID_ENTITY;
			setVisible(false);
			sensors->targetContact = INVALID_ENTITY;
		}
		//FILTHSOME!! UNCLEAN UI! *UNCLEAN!!!!!*
		if (gameController->contactTracked(oldTarget)) gameController->getContact(oldTarget)->isDirty = true;
		if (gameController->contactTracked(newTarget)) gameController->getContact(newTarget)->isDirty = true;
		//if (newTarget != INVALID_ENTITY && !gameController->contactTracked(newTarget)) {
			//gameController->addContact(newTarget);
		//}
	}
	if (sensors->targetContact == INVALID_ENTITY) {
		selected = INVALID_ENTITY;
		setVisible(false);
		return;
	}
	if (!sensors->targetContact.has<IrrlichtComponent>()) {
		sensors->targetContact = INVALID_ENTITY;
		return;
	}
	auto irr = sensors->targetContact.get<IrrlichtComponent>();
	//Moves around the selection GUI
	position2di selectionPos = coll->getScreenCoordinatesFrom3DPosition(irr->node->getAbsolutePosition(), camera);
	selectionPos.X -= 64;
	selectionPos.Y -= 64;
	selectGUI->setRelativePosition(selectionPos);
	selectionPos.Y -= 90;
	objType->setRelativePosition(position2di(selectionPos.X, selectionPos.Y - 20));
	name->setRelativePosition(position2di(selectionPos.X - 192, selectionPos.Y));
	dist->setRelativePosition(position2di(selectionPos.X, selectionPos.Y + 20));
	selectionPos.Y += 212;
	position2di hpPos, spPos;
	hpPos = selectionPos;
	selectionPos.Y += 8;
	spPos = selectionPos;
	vector3df vdist;
	if (irr) {
		vdist = irr->node->getAbsolutePosition() - playerIrr->node->getAbsolutePosition();
		f32 len = vdist.getLength();
		if (len <= 1000) {
			auto lstr = fprecis(len, 5);
			std::wstring str = wstr(lstr) + L"m";
			dist->setText(str.c_str());
		}
		else {
			len = len / 1000;
			auto lstr = fprecis(len, 4);
			std::wstring str = wstr(lstr) + L"km";
			dist->setText(str.c_str());
		}
	}
	if (sensors->targetContact.has<HealthComponent>()) {
		auto targetHP = sensors->targetContact.get<HealthComponent>();
		dimension2du hpSize(128,8);
		//hpSize.set((u32)targetHP->health / targetHP->maxHealth * 128, 8);
		selectHP->setRelativePosition(rect<s32>(hpPos, hpSize));
		hp->updateBar(targetHP->health, targetHP->maxHealth);
		if (targetHP->maxShields > 0.f) {
			dimension2du spSize;
			//spSize.set((u32)targetHP->shields / targetHP->maxShields * 128, 8);
			selectSP->setRelativePosition(rect<s32>(spPos, hpSize));
			sp->updateBar(targetHP->shields, targetHP->maxShields);
		}
		else {
			sp->togVis(false);
		}

	} else {
		hp->togVis(false);
		sp->togVis(false);
	}
	if (gameController->contactTracked(sensors->targetContact)) {
		auto ctct = gameController->getContact(sensors->targetContact);
		if (ctct) {
			std::string str = ctct->objtype;
			if (ctct->isObjective) objType->setText(wstr(str).c_str());
			else objType->setText(L"");
		}
	}
	else {
		objType->setText(L"");
	}

	if (!sensors->targetContact.has<BulletRigidBodyComponent>() && !sensors->targetContact.has<BulletGhostComponent>()) return;
	btVector3 velocity = rbc->rigidBody->getLinearVelocity();
	btVector3 forwardTarget;

	if (sensors->targetContact.has<BulletRigidBodyComponent>()) {
		auto targetRBC = sensors->targetContact.get<BulletRigidBodyComponent>();
		u32 wepCount = 0;
		f32 avgSpeed = 0;
		auto hards = playerId.get<HardpointComponent>();
		for (u32 i = 0; i < hards->hardpointCount; ++i) {
			if (!hards->weapons[i].is_alive()) continue;
			++wepCount;
			avgSpeed += hards->weapons[i].get<WeaponInfoComponent>()->projectileSpeed;
		}
		avgSpeed = avgSpeed / (f32)wepCount;
		f32 timeToArrive = vdist.getLength() / (avgSpeed / .1f); //.1f being projectile mass
		if (hards->weapons[0].get<WeaponInfoComponent>()->hitScan) timeToArrive = 0.f;

		forwardTarget = targetRBC->rigidBody->getCenterOfMassPosition() + (targetRBC->rigidBody->getLinearVelocity() * timeToArrive);
		forwardTarget -= (rbc->rigidBody->getLinearVelocity() * timeToArrive);
	}
	else if (sensors->targetContact.has<BulletGhostComponent>()) {
		forwardTarget = irrVecToBt(irr->node->getAbsolutePosition());
	}
	crosshairTarget = forwardTarget;
	position2di crosshairPos = coll->getScreenCoordinatesFrom3DPosition(btVecToIrr(forwardTarget), camera);
	crosshairPos.X -= 32;
	crosshairPos.Y -= 32;
	crosshair->setRelativePosition(crosshairPos);
}