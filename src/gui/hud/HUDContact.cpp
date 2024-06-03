#include "HUDContact.h"
#include "GameController.h"
#include "GameStateController.h"
#include "SensorComponent.h"
#include "GameAssets.h"
#include "IrrlichtComponent.h"
#include "PlayerComponent.h"
#include "InputComponent.h"
#include "IrrlichtUtils.h"

const SColor CTCT_COLOR[] =
{
	SColor(255,255,255,255), //neutral
	SColor(255,0,205,25), //friendly
	SColor(255,205,0,0), //hostile
	SColor(255,240,205,0), //hazard
	SColor(255,70,90,255), //radio
	SColor(255,140,85,155) //objective
};

const c8* MARKER[] =
{
	"assets/hud/markers/circle_marker_small.png",
	"assets/hud/markers/circle_marker.png",
	"assets/hud/markers/circle_marker.png",
	"assets/hud/markers/circle_marker.png",
	"assets/hud/markers/circle_marker_ring.png",
	"assets/hud/markers/circle_marker_select.png",
};

const c8* MARKER_SMALL[] =
{
	"assets/hud/markers/circle_marker_small.png",
	"assets/hud/markers/circle_marker_small.png",
	"assets/hud/markers/circle_marker_small.png",
	"assets/hud/markers/circle_marker_small.png",
	"assets/hud/markers/circle_marker_small.png",
	"assets/hud/markers/circle_marker_small.png",
};

const c8* CONTACT[] =
{
	"assets/hud/contacts/contact_square_small.png",
	"assets/hud/contacts/contact_square.png",
	"assets/hud/contacts/contact_square.png",
	"assets/hud/contacts/contact_square.png",
	"assets/hud/contacts/contact_diamond.png",
	"assets/hud/contacts/contact_diamond.png",
};

const c8* CONTACT_SMALL[] =
{
	"assets/hud/contacts/contact_square_small.png",
	"assets/hud/contacts/contact_square_small.png",
	"assets/hud/contacts/contact_square_small.png",
	"assets/hud/contacts/contact_square_small.png",
	"assets/hud/contacts/contact_square_small.png",
	"assets/hud/contacts/contact_square_small.png"
};

HUDContact::HUDContact(IGUIElement* root, flecs::entity contactId, flecs::entity playerId, bool obj, bool radio) : HUDElement(root) 
{
	contact = contactId;
	auto ctctFac = contactId.get<FactionComponent>();
	offscreenMarker = guienv->addImage(m_marker(NEUTRAL, false), position2di(0, 0), root);
	contactView = guienv->addImage(m_contact(NEUTRAL, false), position2di(0, 0), root);
	isObjective = obj;
	isRadio = radio;
	updateType(ctctFac);
	offscreenMarker->setRelativePosition(recti(position2di(0), dimension2du(64, 64)));
	contactView->setRelativePosition(recti(position2di(0), dimension2du(64, 64)));

	contactView->setMinSize(dimension2du(64, 64));
	contactView->setMaxSize(dimension2du(64, 64));
	offscreenMarker->setMinSize(dimension2du(64, 64));
	offscreenMarker->setMaxSize(dimension2du(64, 64));
	contactView->setScaleImage(true);
	offscreenMarker->setScaleImage(true);
}

HUDContact::~HUDContact()
{
	if (offscreenMarker) offscreenMarker->remove();
	if (contactView) contactView->remove();
}

inline ITexture* HUDContact::m_marker(const HUDOBJ_TYPE& which, const bool& important) const
{
	if (important) return driver->getTexture(MARKER[which]);
	return driver->getTexture(MARKER_SMALL[which]);
}
inline ITexture* HUDContact::m_contact(const HUDOBJ_TYPE& which, const bool& important) const
{
	if (important) return driver->getTexture(CONTACT[which]);
	return driver->getTexture(CONTACT_SMALL[which]);
}

const HUDContact::HUDOBJ_TYPE HUDContact::m_getType(const FactionComponent* contactFac) const
{
	if (isObjective && inRange) return OBJECTIVE;
	if (isRadio || (isObjective && !inRange)) return RADIO;

	auto playerFac = gameController->getPlayer().get<FactionComponent>();
	if (contactFac) {
		if (playerFac->isHostile(contactFac)) return HOSTILE;
		if (playerFac->isFriendly(contactFac)) return FRIENDLY;
		if (contactFac->type == FACTION_UNCONTROLLED) return HAZARD;
	}
	return NEUTRAL;
}

void HUDContact::updateType(const FactionComponent* contactFac)
{
	HUDOBJ_TYPE newType = m_getType(contactFac);
	if (newType == m_type && !isDirty) return; // who care
	m_type = newType;

	bool important = false;
	if (contactFac) important = contactFac->isImportant;
	if (newType == RADIO || newType == OBJECTIVE) important = true;

	if (gameController->getPlayer().get<SensorComponent>()->targetContact == contact) {
		offscreenMarker->setImage(driver->getTexture("assets/hud/markers/circle_marker_select.png"));
		isSelected = true;
	}
	else {
		offscreenMarker->setImage(m_marker(m_type, important));
		isSelected = false;
	}
	contactView->setImage(m_contact(m_type, important));
	offscreenMarker->setColor(CTCT_COLOR[m_type]);
	contactView->setColor(CTCT_COLOR[m_type]);
	isDirty = false;
}

void HUDContact::setObjective(bool obj, std::string str)
{
	if (obj == isObjective) return;
	isObjective = obj;
	isRadio = obj;
	objtype = str;
	updateType(contact.get<FactionComponent>());
}

bool HUDContact::isValidContact()
{
	if (!contact.is_alive()) return false;
	//if (isRadio) return true;
	checkRange();
	if (!inRange) {
		if (isRadio || isSelected) return true;
		return false;
	}
	return true;
}

void HUDContact::checkRange() 
{
	auto rbc = contact.get<BulletRigidBodyComponent>();
	if (rbc) {
		auto player = gameController->getPlayer();
		auto playerRBC = player.get<BulletRigidBodyComponent>();
		auto sensors = player.get<SensorComponent>();

		btVector3 dist = rbc->rigidBody->getCenterOfMassPosition() - playerRBC->rigidBody->getCenterOfMassPosition();
		if (dist.length() > sensors->detectionRadius) {
			inRange = false;
			return;
		}
	}
	inRange = true;
}

void HUDContact::updateElement(flecs::entity playerId)
{
	auto player = playerId.get<PlayerComponent>();
	auto in = playerId.get<InputComponent>();
	flecs::entity ent(game_world->get_world(), contact);
	if (!ent.is_alive()) return;
	ICameraSceneNode* camera = player->camera;
	ICameraSceneNode* reverse = player->reverseCamera;
	if (in->usingReverseCamera) {
		camera = player->reverseCamera;
		reverse = player->camera;
	}
	ISceneCollisionManager* coll = smgr->getSceneCollisionManager();
	auto irr = ent.get<IrrlichtComponent>();
	if (!irr) return;
	//root->sendToBack(contactView);
	//root->sendToBack(offscreenMarker);

	dimension2du screenSize = driver->getScreenSize();
	bool behind = false;
	position2di contactScreenPos = screenCoords(irr->node->getAbsolutePosition(), camera, behind);

	bool offscreen = (contactScreenPos.X < 0 || contactScreenPos.X > screenSize.Width || contactScreenPos.Y < 0 || contactScreenPos.Y > screenSize.Height);
	//if it's off-screen, set basic element to not visible and the marker to visible
	//else do the reverse
	if (behind || offscreen) {
		contactView->setVisible(false);
		offscreenMarker->setVisible(true);
	}
	else {
		if (!isSelected) contactView->setVisible(true);
		else contactView->setVisible(false);
		offscreenMarker->setVisible(false);
	}

	u32 circleRadius = screenSize.Height / 3;
	position2di screenCenter = position2di(screenSize.Width / 2, screenSize.Height / 2.25);
	position2di vec = contactScreenPos - screenCenter;
	position2di onscreenPos(0, 0);
	if(vec!=onscreenPos)onscreenPos = screenCenter + ((vec * circleRadius) / vec.getLength());
	if (onscreenPos == screenCenter) offscreenMarker->setVisible(false);
	onscreenPos.X -= 32;
	onscreenPos.Y -= 32;
	offscreenMarker->setRelativePosition(onscreenPos);
	onscreenPos = contactScreenPos;
	onscreenPos.X -= 32;
	onscreenPos.Y -= 32;
	contactView->setRelativePosition(onscreenPos);

	auto fac = contact.get<FactionComponent>();
	if (fac) {
		if (fac->type != lastType) updateType(fac);
		lastType = fac->type;
	}
	if (isDirty) {
		updateType(fac);
		isDirty = false;
	}
}