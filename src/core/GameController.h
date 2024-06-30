#pragma once

#ifndef FLIGHTEVENTRECEIVER_H
#define FLIGHTEVENTRECEIVER_H
#include "BaseHeader.h"
#include "MapRunner.h"
#include "btUtils.h"
#include "HUDHazard.h"
#include"Networking.h"
#include <mutex>

/*
* The GameController class holds all the necessary information about what's actually going on in the game. It handles all updates for the game.
* It is the Keeper of the DT (delta time) which determines how frequently scene and physics updates are called. It also manages some other data
* like death callbacks, hit callbacks, and so on; any sort of thing that needs to be included in the game proper.
* 
* The init() function is used to initialize the game (set up the physics world, irrlicht stuff, some other things), and the close() function
* gets rid of those (such as when you return to the game menus).
*/

typedef std::function<void(flecs::entity)> deathCallback;

//whoever shot it and the object to be hit
typedef std::function<void(flecs::entity, flecs::entity)> onHitCallback;

struct AccumulatorEntry;
struct Packet;

class HUDResources;
class HUDActiveSelection;
class HUDLargePopup;
class HUDPopup;
class HUDContact;
struct broadCallback;
class Scenario;

bool clientOwnsThisEntity(const flecs::entity& ent);

class GameController
{
	public:
		std::vector<std::shared_ptr<AccumulatorEntry>> priorityAccumulator;
		bool isPacketValid = false;
		Packet* packet;
		bool OnEvent(const SEvent& event);
#if _DEBUG
		btDebugRenderer rend;
#endif 
		GameController();
		void init();
		void close();

		void update();

		bool isNetworked() const { return m_isNetworked; }
		void setNetworked(bool network = true) { m_isNetworked = network; }

		//Cleans out the player HUD at the end of a scenario.
		void clearPlayerHUD();

		Scenario* currentScenario;
		void initScenario();

		//Sets up a death callback on the given entity. When the entity dies, it will call this function.
		void registerDeathCallback(flecs::entity id, deathCallback cb) { deathCallbacks[id.id()] = cb; }
		//Checks if the entity has a death callback.
		bool hasDeathCallback(flecs::entity id) { return (deathCallbacks.find(id.id()) != deathCallbacks.end()); }
		//Sets up a hit callback on the given entity. When the entity gets shot (SHOT, not damaged) it will call this function.
		void registerHitCallback(flecs::entity id, onHitCallback cb) { hitCallbacks[id.id()] = cb; }
		//Checks if the entity has a hit callback.
		bool hasHitCallback(flecs::entity id) { return (hitCallbacks.find(id.id()) != hitCallbacks.end()); }

		void regPopup(std::string spkr, std::string msg) { 
			m_popup_mtx.lock();
			m_popups.push_back({ spkr, msg });
			m_popup_mtx.unlock();
		}
		void applyPopups();

		std::unordered_map<flecs::entity_t, deathCallback> deathCallbacks;
		std::unordered_map<flecs::entity_t, onHitCallback> hitCallbacks;

		void setPlayer(flecs::entity_t id);
		flecs::entity getPlayer();
		void setChaosTheory(flecs::entity_t id);
		flecs::entity getChaosTheory();

		void setWingman(flecs::entity_t id, u32 slot);
		flecs::entity getWingman(u32 slot);
		void disengageWingman(flecs::entity id);
		bool isPlayerAlive;

		void removeContact(flecs::entity contact);
		void addPopup(std::string spkr, std::string msg);
		void addPopup(std::wstring spkr, std::wstring msg);
		void addLargePopup(std::string msg, std::string spkr = "", bool banter = false);
		void addLargePopup(std::wstring msg, std::wstring spkr = L"", bool banter = false);
		void initializeHUD();
		void addContact(flecs::entity ent, bool objective = false, bool radio = false);
		const bool contactTracked(flecs::entity ent);

		HUDHazard* addHazard(flecs::entity ent, HAZARD which);
		HUDHazard* getHazard(flecs::entity ent, HAZARD which=HAZARD::NONE);
		HUDContact* getContact(flecs::entity ent);
		std::unordered_map<flecs::entity_t, HUDContact*>& allContacts() { return trackedContacts; }
		void clearRadioContacts();
		void smackShields(vector3df pos);
		void smackHealth(vector3df pos);
		const HUDActiveSelection* selectHUD();

		MapRunner mapRunner;

		std::shared_ptr<Objective> objective() { return mapRunner.objective(); }
		
		void setSpawns(s32& ace, s32& reg, s32& aceWep, s32& regWep, s32& turret, s32& turretWep) { mapRunner.setSpawns(ace, reg, aceWep, regWep, turret, turretWep); }

		const bool inCombat() const { return m_inCombat; }
		void triggerCombatMusic();
		void triggerAmbientMusic();
		const f32 getTimeSinceStart() { return m_timeSinceScenarioStart; }
		//use this as a hang on physics effects to make sure we're damn good and ready
		const bool stillLoadingHang() const { return (m_timeSinceScenarioStart <= 1.75f); }

		//Specific no-collide relationship for "don't collide with this object", best used for guns on ships.
		const flecs::entity doNotCollideWith() const { return DO_NOT_COLLIDE_WITH; }
		//Fired-by relationship.
		const flecs::entity firedBy() const { return FIRED_BY; }
		void markForDeath(flecs::entity& ent, bool fromNetwork = false);
		bool isMarkedForDeath(flecs::entity& ent) {
			for (auto& entity : m_markedForDeath) {
				if (ent == entity) return true;
			}
			return false;
		}

		void triggerShipSpawn(std::function<void()> triggerlambda) { m_shipsToSpawn.push_back(triggerlambda); }

		void setFire(flecs::entity& gun) {
			m_gunsToFire.push_back(gun);
		}
		const bool isWingmanDisengaged(flecs::entity& ent) const { 
			for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
				if (wingmen[i] == ent) {
					return wingmenDisengaged[i];
				}
			}
			return false;
		}
		const btVector3 playerStart() const;
		const f32 getDt() const { return this->dt; }

		//Takes the shot from the network and adds it to the queue for guns to fire in the game.
		void addShotFromNetwork(Network_ShotFired& shot) { m_networkGunsToFire.push_back(shot); }
		//Adds the shot to the queue for shots to be sent over the network.
		void sendShotToNetwork(Network_ShotFired& shot) { m_networkShotsToSend.push_back(shot); }
		//shots going TO the network

		//Takes the damage from the network and adds it to the damage instances for a given ship.
		void addDamageFromNetwork(Net_DamageInstance& inst) { m_netDamageToApply.push_back(inst); }
		//Adds the shot to damage instances that need to be sent.
		void sendDamageToNetwork(Net_DamageInstance& inst) { networkDamageToSend.push_back(inst); }

		std::vector <Net_DamageInstance> networkDamageToSend;
		std::vector<Network_ShotFired> m_networkShotsToSend;
		std::vector<NetworkId>& getNetworkDeadEntities() { return m_networkMarkedForDeath; }
		Packet networkShotPacket;
		const bool finishAnim() const { return m_finishingAnim; }
		const bool startAnim() const { return m_startingAnim; }
		bool tutorial = false;
	private:
		std::mutex m_popup_mtx;
		std::mutex m_ctct_mtx;
		std::mutex m_rsrc_mtx;

		std::list<std::pair<std::string, std::string>> m_popups;

		std::vector<flecs::entity> m_markedForDeath;
		std::vector<NetworkId> m_networkMarkedForDeath;
		std::vector<flecs::entity> m_gunsToFire;
		//shots FROM the network
		std::vector<Network_ShotFired> m_networkGunsToFire;
		std::vector<Net_DamageInstance> m_netDamageToApply;
		std::vector<std::function<void()>> m_shipsToSpawn;

		flecs::entity_t playerEntity;
		flecs::entity_t chaosTheory;

		flecs::entity_t wingmen[MAX_WINGMEN_ON_WING];
		bool wingmenDisengaged[MAX_WINGMEN_ON_WING];

		void registerComponents();
		void registerSystems();
		void registerRelationships();

		bool open = false;
		bool m_isNetworked = false;
		bool m_finishingAnim = false;
		bool m_startingAnim = true;

		u32 then;
		f32 accumulator = 0.0f;
		f32 dt = 1/100.0f;
		f32 t = 0.0f;

		f32 m_timeSinceScenarioStart = 0.f;
		f32 m_finishTimer = 0.f;
		f32 m_startTimer = 0.f;
		//bullet stuff
		btBroadphaseInterface* broadPhase;
		btDefaultCollisionConfiguration* collisionConfig;
		btCollisionDispatcher* dispatcher;
		btSequentialImpulseConstraintSolverMt* solver;
		broadCallback* collCb;
		btGhostPairCallback* gPairCb;
		btConstraintSolverPoolMt* solverPool;
		btITaskScheduler* taskScheduler;
		void m_hudUpdate();

		IGUIElement* rootHUD=nullptr;
		HUDResources* resources=nullptr;
		HUDActiveSelection* activeSelect=nullptr;
		HUDLargePopup* largePop=nullptr;
		std::list<HUDPopup*> popups;
		std::list<HUDContact*> ctcts;
		std::unordered_map<flecs::entity_t, HUDContact*> trackedContacts;
		std::list<HUDHazard*> hazards;

		bool m_inCombat = false;
		const f32 musicChangeCooldown = 15.f;
		f32 musicChangeCurCooldown = 0.f;

		flecs::entity FIRED_BY = INVALID_ENTITY;
		flecs::entity DO_NOT_COLLIDE_WITH = INVALID_ENTITY;

		Scenario* m_dummyScenario = nullptr;

		struct _storedHudMsg {
			std::string spkr;
			std::string msg;
			bool banter = false;
		};
		std::vector<_storedHudMsg> startMsgs;
};


#endif
