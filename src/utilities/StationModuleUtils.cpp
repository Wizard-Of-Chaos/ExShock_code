#include "StationModuleUtils.h"
#include "TurretUtils.h"
#include "BulletRigidBodyComponent.h"
#include "btUtils.h"
#include "PowerComponent.h"
#include "LoadoutData.h"
#include "ObstacleUtils.h"
#include "ShipUtils.h"
#include "GameFunctions.h"
#include "IrrlichtComponent.h"
#include "CrashLogger.h"

const f32 BASE_STATION_MASS = 500.f;
const f32 CONNECTION_MASS = 45.f;


const std::unordered_map<STATION_PIECE, s32> stationPieceIds =
{
	{SPIECE_DOCK, 24},
	{SPIECE_SHIELD, 25},
	{SPIECE_TURRET_L, 26},
	{SPIECE_FILL_T, 27 },
	{SPIECE_FILL_I, 28},
	{SPIECE_FILL_L, 29},
	{SPIECE_SENSOR, 30},
	{SPIECE_FUEL_TANK, 31},
	{SPIECE_LARGE_GENERATOR,23},
	{SPIECE_SMALL_GENERATOR,23}, // fix this later
	{SPIECE_MAX_HUMAN_ROLLS, 28}, //double-dip for use in rolling random debris

	{SPIECE_ALIEN_X_GENERATOR, 36},
	{SPIECE_ALIEN_FILL_I, 37},
	{SPIECE_ALIEN_DOCK, 38},
	{SPIECE_ALIEN_TURRET_Y, 39},
	{SPIECE_ALIEN_SHIELD, 40},

	{SPIECE_BOSS_CENTER, 43},
	{SPIECE_BOSS_ARM, 44},
	{SPIECE_BOSS_SHIELD, 45}

};
const std::unordered_map<std::string, STATION_PIECE> stationPieceNames =
{
	{"dock", SPIECE_DOCK},
	{"shield", SPIECE_SHIELD},
	{"fuel", SPIECE_FUEL_TANK},
	{"sensor", SPIECE_SENSOR},
	{"fill_T", SPIECE_FILL_T},
	{"fill_I", SPIECE_FILL_I},
	{"fill_L", SPIECE_FILL_L},
	{"turret_L", SPIECE_TURRET_L},
	{"reactor_large", SPIECE_LARGE_GENERATOR},
	{"reactor_small", SPIECE_SMALL_GENERATOR},

	{"alien_reactor_large", SPIECE_ALIEN_X_GENERATOR},
	{"alien_fill_I", SPIECE_ALIEN_FILL_I},
	{"alien_dock", SPIECE_ALIEN_DOCK},
	{"alien_turret_Y", SPIECE_ALIEN_TURRET_Y},
	{"alien_shield", SPIECE_ALIEN_SHIELD},

	{"boss_center", SPIECE_BOSS_CENTER},
	{"boss_arm", SPIECE_BOSS_ARM},
	{"boss_shield", SPIECE_BOSS_SHIELD}

};

//if override roll is set to true it will roll for the ENTIRE human list not just max rolls
static const s32 randPiece(bool overrideRoll=false) {
	STATION_PIECE piece;
	if (!overrideRoll)piece = (STATION_PIECE)random.unumEx(SPIECE_MAX_HUMAN_ROLLS);
	else piece = (STATION_PIECE)random.unumEx(SPIECE_MAX_HUMAN);

	return stationPieceIds.at(piece);
}

static void _snapModule(flecs::entity mod, StationModuleOwnerComponent* owner)
{
	auto power = mod.get_mut<PowerComponent>();
	if (!power->generator) power->receivingPowerFrom = INVALID_ENTITY;
	power->disabled = true;
	owner->neutralizeModule(mod);
	owner->removeModule(mod);
}

static ThrustComponent _stationThrust()
{
	ThrustComponent thrst;
	thrst.multiplier = 600.f;
	thrst.brake = 2000.f;
	thrst.strafe = 2000.f;
	thrst.pitch = 2000.f;
	thrst.yaw = 2000.f;
	thrst.roll = 2000.f;
	thrst.linearMaxVelocity = 10000.f;
	thrst.angularMaxVelocity = 20.f;
	thrst.velocityTolerance = 0.f;
	thrst.moves[STOP_ROTATION] = true;
	//thrst.moves[STOP_VELOCITY] = true;
	thrst.nonstopThrust = true;
	return thrst;
}

void snapOffStationBranch(flecs::entity mod)
{
	if (!mod.has<StationModuleComponent>()) return; //what?
	auto smod = mod.get_mut<StationModuleComponent>();
	if (!smod->ownedBy.is_alive()) return;
	if (!smod->ownedBy.has<StationModuleOwnerComponent>()) return;

	auto ownercomp = smod->ownedBy.get_mut<StationModuleOwnerComponent>();
	_snapModule(mod, ownercomp);
	for (s32 i = 0; i < smod->connections; ++i) {
		if (i == smod->connectedOn) continue; //don't iterate INTO the generator module
		if (smod->connectedEntities[i].is_alive()) _snapModule(mod, ownercomp);
	}
}

std::vector<flecs::entity> createModularHumanStation(
	vector3df position, vector3df rotation,
	u32 numPieces, FACTION_TYPE which, vector3df scale, s32 turretId, s32 wepId)
{

	ThrustComponent thrst = _stationThrust();
	auto start = createDynamicObstacle(23, position, rotation, scale, 0, 0, 0);
	start.set<ThrustComponent>(thrst);
	start.set<PowerComponent>(FREE_POWER_COMPONENT); //...free?
	initializeFaction(start, which);

	u32 div = numPieces / 4;

	baedsLogger::log("Building modular human station. Branches of size " + std::to_string(div) + ".\n");

	std::vector<flecs::entity> ret;
	ret.push_back(start);
	
	generateStationBranch(ret, start, 0, div, scale, which, true, false, turretId, wepId);
	generateStationBranch(ret, start, 1, div, scale, which, false, false, turretId, wepId);
	generateStationBranch(ret, start, 2, div, scale, which, false, true, turretId, wepId);
	generateStationBranch(ret, start, 3, div, scale, which, false, true, turretId, wepId);
	baedsLogger::log("Branches done, checking for docks and adding modules to owner.\n");

	StationModuleOwnerComponent owncmp;
	for (u32 i = 1; i < ret.size(); ++i) { //first position is the start piece
		if (!ret[i].is_valid()) continue; //wtf?
		if (!ret[i].has<StationModuleComponent>()) continue; //again, wtf?
		owncmp.modules[owncmp.modCount++] = ret[i];
		auto mod = ret[i].get_mut<StationModuleComponent>();
		mod->ownedBy = start;
		if (mod->hasDock) owncmp.docks[owncmp.dockCount++] = ret[i];
	}
	owncmp.self = start;
	if (owncmp.hasShieldModule()) owncmp.toggleShields(true);
	start.set<StationModuleOwnerComponent>(owncmp);
	baedsLogger::log("Done. Size: " + std::to_string(owncmp.modCount) + ", docks: " + std::to_string(owncmp.dockCount) + "\n");	
	return ret;
}

static flecs::entity addHumanModuleOnBranch(flecs::entity old, s32 oldSlot, vector3df scale, FACTION_TYPE fac, bool& shield, bool& turret, s32 turretId, s32 wepId)
{
	s32 newPiece = randPiece();
	if (shield) {
		newPiece = stationPieceIds.at(SPIECE_SHIELD);
		shield = false;
	}
	s32 turrPieceId = stationPieceIds.at(SPIECE_TURRET_L);
	if (turret && newPiece == turrPieceId) {
		turret = false;
	}
	else if (!turret && newPiece == turrPieceId) {
		while (newPiece == turrPieceId) {
			newPiece = randPiece();
		}
	}

	auto newModuleData = (StationModuleData*)obstacleData[newPiece];

	s32 newSlot = random.unumEx(newModuleData->moduleComp.connections);
	flecs::entity newMod = addModuleToSlot(old, newPiece, oldSlot, newSlot, scale, fac, turretId, wepId);
	return newMod;
}

void generateStationBranch(std::vector<flecs::entity>& retList, flecs::entity oldModule, s32 oldSlot, s32 numPieces,
	vector3df scale, FACTION_TYPE fac, bool hasShield, bool hasTurret, s32 turretId, s32 wepId)
{

	if (oldSlot > MAX_STATION_MODULE_CONNECTIONS) return;

	bool turret = hasTurret;
	bool shield = hasShield;

	auto oldSmod = oldModule.get<StationModuleComponent>();

	//if (oldSmod->connectedEntities[oldSlot].is_alive()) return; //we're done

	auto start = addHumanModuleOnBranch(oldModule, oldSlot, scale, fac, shield, turret, turretId, wepId); //start the new branch
	if (start == INVALID_ENTITY) return; // what?
	retList.push_back(start);

	auto startSmod = start.get<StationModuleComponent>();
	if (startSmod->connections <= 1) return; // we're done here, this is a dock
	if (startSmod->connections > MAX_STATION_MODULE_CONNECTIONS) {
#ifdef _DEBUG
		baedsLogger::errLog("Connections too high: " + std::to_string(startSmod->connections) + "\n");
#endif
		return;
	}
	s32 dividedPieceNum = numPieces / startSmod->connections - 1; //had to avoid the divide-by-0...
	if (dividedPieceNum > MAX_STATION_MODULE_CONNECTIONS) dividedPieceNum = 0;
	for (s32 i = 0; i < startSmod->connections; ++i) {
		if (startSmod->connectedOn == i) continue; //dodge whatever the hell it's connected on
		if (numPieces <= 1) {
			auto mod = addModuleToSlot(start, stationPieceIds.at(SPIECE_DOCK), i, 0, scale, fac);
			if (mod == INVALID_ENTITY) break;
			retList.push_back(mod);
			continue;
		}
		if (i > MAX_STATION_MODULE_CONNECTIONS) {
			break;
		}
		generateStationBranch(retList, start, i, dividedPieceNum, scale, fac, shield, turret, turretId, wepId); //aaaand recurse.
	}
}

static btConeTwistConstraint* _buildConstraint(vector3df ownerPoint, vector3df newPoint,
	vector3df scale, btRigidBody* ownerBody, btRigidBody* newBody)
{	
	btTransform A = ownerBody->getCenterOfMassTransform().inverse();
	btTransform B = newBody->getCenterOfMassTransform().inverse();
	auto constraint = new btConeTwistConstraint(*ownerBody, *newBody, A, B);

	for (u32 j = 0; j < 6; ++j) {
		constraint->setParam(BT_CONSTRAINT_STOP_ERP, .9f, j);
		constraint->setDamping(5.f);
		constraint->setLimit(j, 0.f);
	}
	return constraint;
}

flecs::entity addModuleToSlot(flecs::entity owner, s32 newModuleId, s32 ownerSlot, s32 newModuleSlot, vector3df scale, FACTION_TYPE fac, s32 turretId, s32 wepId)
{
	//std::cout << "adding module to slot... \n";
	if (ownerSlot >= MAX_STATION_MODULE_CONNECTIONS || newModuleSlot >= MAX_STATION_MODULE_CONNECTIONS) {
#ifdef _DEBUG
		auto offender = owner.get<StationModuleComponent>();
		baedsLogger::errLog("Can't add module to slot " + std::to_string(ownerSlot) + " (new slot " + std::to_string(newModuleSlot) + "\n");
		baedsLogger::errLog("Type: " + std::to_string(offender->type) + ", connections: " + std::to_string(offender->connections) + "\n");
#endif
		return INVALID_ENTITY;
	}
	auto ownerStationMod = owner.get_mut<StationModuleComponent>();
	auto dockModuleData = (StationModuleData*)obstacleData[newModuleId];

	if (!owner.is_alive()) return INVALID_ENTITY; //wtf?
	if (ownerSlot == ownerStationMod->connectedOn) return INVALID_ENTITY; //this is where it's connected!
	if (ownerSlot > ownerStationMod->connections) return INVALID_ENTITY; //this number is too big!
	if (newModuleSlot > dockModuleData->moduleComp.connections) return INVALID_ENTITY; //this other number is too big!

	auto irr = owner.get<IrrlichtComponent>();
	auto dockedModule = createDynamicObstacle(newModuleId, 
		getNewModulePos(owner, ownerSlot, dockModuleData, newModuleSlot, scale),
		getNewModuleRotDegrees(owner, ownerSlot, dockModuleData, newModuleSlot),
		scale, CONNECTION_MASS, 0, 0);
	initializeFaction(dockedModule, fac, false);

	if (dockModuleData->hasTurrets) {
		initializeTurretsOnOwner(dockedModule, turretId, wepId);
	}
	auto newStationMod = dockedModule.get_mut<StationModuleComponent>();
	newStationMod->connectedOn = newModuleSlot;
	newStationMod->connectedEntities[newModuleSlot] = owner;	

	
	auto constraint = _buildConstraint(
		ownerStationMod->connectionPoint[ownerSlot], newStationMod->connectionPoint[newModuleSlot], scale,
		owner.get<BulletRigidBodyComponent>()->rigidBody, dockedModule.get<BulletRigidBodyComponent>()->rigidBody
		);

	bWorld->registerConstraint(constraint, owner, dockedModule);
	
	ownerStationMod->connectedEntities[ownerSlot] = dockedModule;

	PowerComponent dummy;
	dummy.isPowered = true;
	dummy.receivingPowerFrom = owner;
	dockedModule.set<PowerComponent>(dummy);
	dockedModule.set<ThrustComponent>(_stationThrust());
	return dockedModule;
}

vector3df getNewModulePos(flecs::entity oldModule, s32 oldSlot, StationModuleData* newModule, s32 newSlot, vector3df scale)
{
	auto rbc = oldModule.get<BulletRigidBodyComponent>();
	auto irr = oldModule.get<IrrlichtComponent>();
	auto oldStatMod = oldModule.get<StationModuleComponent>();
	if (!rbc || !oldStatMod || !irr) return vector3df(0, 0, 0);

	vector3df oldPos = irr->node->getPosition();
	vector3df oldSlotPos = oldStatMod->connectionPoint[oldSlot] * scale;
	vector3df newSlotPos = newModule->moduleComp.connectionPoint[newSlot] * scale;
	
	btQuaternion orient = rbc->rigidBody->getOrientation();
	btVector3 vec = irrVecToBt(oldSlotPos);
	oldSlotPos = btVecToIrr(vec.rotate(orient.getAxis(), orient.getAngle()));

	vector3df rot = getNewModuleRotDegrees(oldModule, oldSlot, newModule, newSlot);
	matrix4 rotMat;
	rotMat.setRotationDegrees(rot);
	rotMat.rotateVect(newSlotPos);

	vector3df ret = ((oldPos + oldSlotPos) - newSlotPos);
	return ret;
}

vector3df getNewModuleRotRadians(flecs::entity oldModule, s32 oldSlot, StationModuleData* newModule, s32 newSlot)
{
	
	auto rbc = oldModule.get<BulletRigidBodyComponent>();
	auto irr = oldModule.get<IrrlichtComponent>();
	auto oldStatMod = oldModule.get<StationModuleComponent>();
	if (!rbc || !oldStatMod || !irr) return vector3df(0, 0, 0);

	btVector3 oldSlotUp = irrVecToBt(oldStatMod->connectionUp[oldSlot]);
	btVector3 oldSlotOut = irrVecToBt(oldStatMod->connectionDirection[oldSlot]);

	btVector3 newSlotUp = irrVecToBt(newModule->moduleComp.connectionUp[newSlot]);
	btVector3 newSlotOut = irrVecToBt(newModule->moduleComp.connectionDirection[newSlot]);

	btQuaternion old = rbc->rigidBody->getOrientation();

	oldSlotUp = oldSlotUp.rotate(old.getAxis(), old.getAngle());
	oldSlotOut = oldSlotOut.rotate(old.getAxis(), old.getAngle());

	btQuaternion ups = shortestArcQuat(newSlotUp, oldSlotUp);
	newSlotOut = newSlotOut.rotate(ups.getAxis(), ups.getAngle());
	newSlotUp = newSlotUp.rotate(ups.getAxis(), ups.getAngle());

	btQuaternion outs = shortestArcQuat(newSlotOut, -oldSlotOut);

	btQuaternion finalRot = outs * ups;
	vector3df ret;
	finalRot.getEulerZYX(ret.Z, ret.Y, ret.X);

	return ret;
}

vector3df getNewModuleRotDegrees(flecs::entity oldModule, s32 oldSlot, StationModuleData* newModule, s32 newSlot)
{
	//functions written SOLELY because I am an idiot
	return getNewModuleRotRadians(oldModule, oldSlot, newModule, newSlot) * RADTODEG;
}

flecs::entity createLooseHumanModule(vector3df pos, vector3df rot, vector3df scale, dataId id, NetworkId net)
{
	flecs::entity mod = createDynamicObstacle((id == INVALID_DATA_ID) ? randPiece(true) : id, pos, rot, scale, 25.f, 0.f, 0.f, false, net);
	std::string name = mod.doc_name();
	name += " [Derelict]";
	mod.set_doc_name(name.c_str());

	//remove excess components
	mod.remove<TurretHardpointComponent>();
	mod.remove<HangarComponent>();
	mod.remove<StationModuleComponent>();
	mod.remove<PowerComponent>();

	mod.get_mut<ObstacleComponent>()->type = DERELICT_STATION_MODULE;

	return mod;
}

struct _moduleNode
{
	STATION_PIECE id;
	std::string name;
	std::vector<_moduleNode*> children;
	_moduleNode* owner = nullptr;
	_moduleNode(STATION_PIECE id, _moduleNode* owner, std::string name) : id(id), owner(owner), name(name) { children.clear(); }
	~_moduleNode() {
		for (auto node : children) delete node;
	}
};

struct _stationTree
{
	_moduleNode* root=nullptr;
	FACTION_TYPE fac = FACTION_PLAYER;
	bool isLarge = true;
	s32 turretId = 0;
	s32 wepId = 3;
	bool loaded = false;
	vector3df scale = vector3df(10.f, 10.f, 10.f);

	_stationTree() {}
	_stationTree(std::string fname) { load(fname); }
	void load(std::string fname) {
		auto xml = createIrrXMLReader(fname.c_str());
		_moduleNode* current = nullptr;

		while (xml->read()) {
			switch (xml->getNodeType()) {
			case EXN_ELEMENT: {
				const stringw name = xml->getNodeName();
				if (name == L"global") {
					std::string faction = xml->getAttributeValueSafe("faction");
					if (faction == "player") fac = FACTION_PLAYER;
					else if (faction == "alien") fac = FACTION_HOSTILE;
					else if (faction == "wild") fac = FACTION_UNCONTROLLED;
					else if (faction == "neutral") fac = FACTION_NEUTRAL;

					turretId = xml->getAttributeValueAsInt("turretId", 0);
					wepId = xml->getAttributeValueAsInt("wepId", 0);
					auto scalenum = xml->getAttributeValueAsFloat("scale", 10.f);
					scale = vector3df(scalenum, scalenum, scalenum);
				}
				else if (name == L"module") {
					auto previous = current;
					current = new _moduleNode(stationPieceNames.at(xml->getAttributeValueSafe("type")), previous, xml->getAttributeValueSafe("type"));
					if (previous) previous->children.push_back(current);
					if (!root) root = current;
				}
				break;
			}
			case EXN_ELEMENT_END: {
				if (current) current = current->owner; //go up a level
				break;
			}
			default:
				break;
			}//end switch
		}
		if (root) loaded = true;
		delete xml;
	}
	~_stationTree() {
		delete root;
	}
};

void _createBranchFromFile(flecs::entity owner, s32 ownerSlot, _stationTree& tree, _moduleNode* which, std::vector<flecs::entity>& ret)
{
	//std::cout << "adding piece " << which->name << " on slot " << ownerSlot << "\n";
	auto ent = addModuleToSlot(owner, stationPieceIds.at(which->id), ownerSlot, 0, tree.scale, tree.fac, tree.turretId, tree.wepId);
	ret.push_back(ent);
	for (u32 i = 0; i < which->children.size(); ++i) {
		//std::cout << "adding branch on slot " << i+1 << "\n";
		_createBranchFromFile(ent, i + 1, tree, which->children[i], ret);
	}
}

std::vector<flecs::entity> createModularStationFromFile(vector3df position, vector3df rotation, std::string fname, bool overrideFac, FACTION_TYPE fac)
{
	baedsLogger::log("Building station from file " + fname + "...\n");
	std::string file = "assets/attributes/prebuilt_stations/";
	_stationTree tree(file + fname);
	if (overrideFac) {
		tree.fac = fac;
	}
	auto start = createDynamicObstacle(stationPieceIds.at(tree.root->id), position, rotation, tree.scale, BASE_STATION_MASS);
	start.set<ThrustComponent>(_stationThrust());
	start.set<PowerComponent>(FREE_POWER_COMPONENT); //...free?
	initializeFaction(start, tree.fac);
	std::vector<flecs::entity> ret;
	ret.push_back(start);
	for (u32 i = 0; i < tree.root->children.size(); ++i) {
		_createBranchFromFile(start, i, tree, tree.root->children[i], ret);
	}
	StationModuleOwnerComponent owncmp;
	for (u32 i = 1; i < ret.size(); ++i) { //first position is the start piece
		if (!ret[i].is_valid()) continue; //wtf?
		if (!ret[i].has<StationModuleComponent>()) continue; //again, wtf?
		owncmp.modules[owncmp.modCount++] = ret[i];
		auto mod = ret[i].get_mut<StationModuleComponent>();
		mod->ownedBy = start;
		if (mod->hasDock) owncmp.docks[owncmp.dockCount++] = ret[i];
	}
	owncmp.self = start;
	if (owncmp.hasShieldModule()) owncmp.toggleShields(true);
	start.set<StationModuleOwnerComponent>(owncmp);

	baedsLogger::log("Done building from file.\n");
	return ret;
}
