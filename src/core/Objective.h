#pragma once
#include "BaseHeader.h"
#include "InputComponent.h"
#include <functional>

enum OBJECTIVE_TYPE
{
	OBJ_DESTROY,
	OBJ_GO_TO,
	OBJ_COLLECT,
	OBJ_ESCORT,
	OBJ_CAPTURE,
	OBJ_PROTECT,
	OBJ_HOLDOUT,
	OBJ_TUTORIAL,
	OBJ_NOT_LOADED
};
const extern std::unordered_map<OBJECTIVE_TYPE, std::string> objNameStrings;

class Objective
{
	public:
		Objective(std::list<flecs::entity> targets, bool hasTimer = false, f32 timerCount = 0) : targets(targets), hasTimer(hasTimer), timer(timerCount) {}
		virtual void setup() {}
		const bool isObjOver(f32 dt);
		bool hasRadioSpawn() {return spawnsRadio; }
		vector3df position() { return m_position; }
		void setRadioSpawn(vector3df pos = vector3df(0,0,0), bool setting = true) { spawnsRadio = setting; m_position = pos; }

		//returns true when the objective has been completed
		virtual const bool objectiveUpdate() = 0;
		//returns true if the objective is successful
		virtual const bool success() = 0;
		void setTimer(const f32 timeToSet) { hasTimer = true; timer = timeToSet; }
		void resetTimer() { currentTime = 0; }
		void removeTimer() { hasTimer = false; }
		void setCompletionCb(const std::function<void()> cb) { completionCb = cb; }
		void onComplete();
		const OBJECTIVE_TYPE type() const { return m_type; }
		void setType(OBJECTIVE_TYPE type) { m_type = type; }
		std::list<flecs::entity> getTargets() { return targets; }
		void addTargets(std::list<flecs::entity> newtargs) { for (auto& ent : newtargs) { targets.push_back(ent); } }
	protected:
		//returns true if timer has expired, false otherwise
		const bool m_timesUp() { if (hasTimer) { if (currentTime > timer) return true; } return false; }
		std::function<void()> completionCb;
		std::list<flecs::entity> targets;
		
		bool spawnsRadio = false;
		bool completeTriggered = false;
		bool hasTimer = false;
		f32 timer = 0;
		f32 currentTime = 0;
		f32 dt=.005f;

		OBJECTIVE_TYPE m_type = OBJ_NOT_LOADED;

		vector3df m_position = vector3df(0, 0, 0); //only used for radio spawning
};

class RemoveEntityObjective : public Objective
{
	public:
		RemoveEntityObjective(std::list<flecs::entity> targets, bool hasTimer = false, f32 timerCount = 0) : Objective(targets, hasTimer, timerCount) {
			m_type = OBJECTIVE_TYPE::OBJ_DESTROY;
		}
		virtual const bool objectiveUpdate();
		virtual const bool success();
	private:

};

class WaveDefenseObjective : public Objective
{
	public:
		WaveDefenseObjective(std::list<flecs::entity> targets = {}, bool hasTimer = false, f32 timerCount = 0) : Objective(targets, hasTimer, timerCount) {
			m_type = OBJECTIVE_TYPE::OBJ_DESTROY;
		}
		struct _wave {
			s32 aceShipId = 1;
			s32 aceWepId = 1;
			s32 shipId = 1;
			s32 wepId = 1;
			s32 turretId = 1;
			s32 turretWepId = 1;
		};
		void addWave(s32 ace, s32 aceWep, s32 ship, s32 shipWep, s32 turret, s32 turretWep) {
			m_waves.push_back({ ace, aceWep, ship, shipWep, turret, turretWep });
		}
		virtual const bool objectiveUpdate();
		virtual const bool success();
	private:
		std::list<_wave> m_waves;
};

class GoToPointObjective : public Objective
{
	public:
		GoToPointObjective(std::list<flecs::entity> targets, bool hasTimer = false, f32 timerCount = 0) : Objective(targets, hasTimer, timerCount) {
			m_type = OBJECTIVE_TYPE::OBJ_GO_TO;
		}
		void setPoint(vector3df point) {
			m_position = point;
			m_usesEntity = false;
		}
		void setDistance(f32 distance) {
			m_dist = distance;
		}
		virtual const bool objectiveUpdate();
		virtual const bool success();
	private:
		f32 m_dist = 100.f;
		bool m_usesEntity = true;
};

class EscortObjective : public Objective
{
public:
		EscortObjective(std::list<flecs::entity> targets, bool hasTimer = false, f32 timerCount = 0) : Objective(targets, hasTimer, timerCount) {
			m_type = OBJECTIVE_TYPE::OBJ_ESCORT;
		}
		void setPoint(vector3df point) {
			m_position = point;
			m_usesEntity = false;
		}
		void setDistance(f32 distance) {
			m_dist = distance;
		}
		virtual const bool objectiveUpdate();
		virtual const bool success();
		flecs::entity m_escorted = INVALID_ENTITY;

	private:
		f32 m_dist = 90.f;
		bool m_usesEntity = true;
};

class DockObjective : public Objective
{
	public:
		DockObjective(std::list<flecs::entity> targets, bool hasTimer = false, f32 timerCount = 0) : Objective(targets, hasTimer, timerCount) {
			m_type = OBJECTIVE_TYPE::OBJ_CAPTURE;
			dockStr = "I've docked -- keep us covered while we clean out the station.";
			breakStr = "I can't stay docked here! I need to re-dock!";
			completeStr = "We're in control, commander. Thanks for the cover fire.";
			spkr = "Sarah Ivanova";
		}
		virtual const bool objectiveUpdate();
		virtual const bool success();
		std::string dockStr;
		std::string breakStr;
		std::string completeStr;
		std::string spkr;
		f32 timeToCapture = 60.f;
		f32 timeSpentCapturing = 0.f;
		f32 enemySpawnRate = 9.f;
		bool appliesStickySpawner = true;
	private:
		bool isCurrentlyDocked = false;
};

class ProtectObjective : public Objective
{
	public:
		ProtectObjective(std::list < flecs::entity> targets, f32 timerCount = 60.f) : Objective(targets, true, timerCount) {
			m_type = OBJECTIVE_TYPE::OBJ_PROTECT;
		}
		virtual const bool objectiveUpdate();
		virtual const bool success();
	private:
};

class KeyPressObjective : public Objective
{
	public:
		KeyPressObjective(std::vector<INPUT> inputsToHit) : Objective({}) {
			m_type = OBJECTIVE_TYPE::OBJ_TUTORIAL;
			for (u32 i = 0; i < IN_MAX_ENUM; ++i) {
				inputsTriggered[i] = false;
				inputsRequired[i] = false;
			}
			for (auto& in : inputsToHit) {
				inputsRequired[in] = true;
			}
		}
		virtual const bool objectiveUpdate();
		virtual const bool success();
	private:
		bool inputsTriggered[IN_MAX_ENUM];
		bool inputsRequired[IN_MAX_ENUM];
};