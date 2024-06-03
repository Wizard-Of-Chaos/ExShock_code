#pragma once
#ifndef CAMPAIGN_H
#define CAMPAIGN_H
#include "BaseHeader.h"
#include "Scenario.h"
#include "LoadoutData.h"
#include "Sector.h"
#include "Dialogue.h"
#include "CarrierUpgrades.h"
#include <unordered_map>
#include <list>

/*
* The campaign holds all the data necessary to run the current campaign. This includes ammunition counts, supply counts, the various
* ships and weapons that are currently in storage on the ship, what sector the ship is currently in, and the current wingmen available to the
* player. Includes many functions that act to adjust the campaign.
*/

class Campaign
{
	public:
		Campaign() : currentDifficulty(1), ammunition(10), supplies(100.f) {
			for (u32 i = 0; i < MAX_WINGMEN_ON_WING; ++i) {
				assignedShips[i] = INVALID_DATA_ID;
				assignedWing[i] = INVALID_DATA_ID;
			}
		}
		//Saves a campaign to file.
		bool saveCampaign(std::string fname);
		//Loads a campaign from file.
		bool loadCampaign(std::string fname);
		//Exits the campaign and cleans up memory.
		void exitCampaign();
		//Sets up a new campaign.
		void newCampaign();

		//Returns the present sector.
		Sector* getSector() {return currentSector;}
		bool advance(); //advance called from the gui
		void returnToCampaign(); //returns from a given scenario

		//Gets the current ammo count.
		const s32 getAmmo() const { return ammunition; }
		//Gets the current supply count.
		const f32 getSupplies() const { return supplies; }

		const u32 getValidWingmanCount() const {
			u32 count = 0;
			for (const auto& wingman : m_wingmen) {
				if (!wingman->injured && wingman->id != 0) ++count;
			}
			return count;
		}
		//Adds ammo to the current ammo count.
		void addAmmo(s32 amt);
		//Attempts to remove up to "amt" from the current ammo count. Returns the amount it was actually able to get.
		s32 removeAmmo(s32 amt);
		//Adds supplies to the current supply count.
		void addSupplies(f32 amt);
		//Attempts to remove up to "amt" from the current supply count. Returns the amount it was actually able to get.
		f32 removeSupplies(f32 amt);

		//Creates a new ship instance and increments the ship-instance counter. If templateShip is set, it does not increment.
		ShipInstance* createNewShipInstance(u32 id, bool templateShip = false);
		//Creates a new ship instance from a given piece of ship data.
		ShipInstance* buildShipInstanceFromData(ShipData* data);

		//Creates a new weapon instance and increments the weapon-instance counter. If template wep is set, it does not increment.
		//This function builds either a physics weapon, regular weapon, or heavy weapon. It pulls the data from the weapon component.
		//This will ALSO add the weapon instance to the current inventory.
		WeaponInstance* createNewWeaponInstance(WeaponInfoComponent wep, bool templateWep = false); //agnostic on whether or not it's a physics weapon or not
		//Builds a random weapon instance. This only creates regular weapons.
		WeaponInstance* createRandomWeaponInstance(); //only regular weapons
		//Gets the weapon data from an instance.
		WeaponData* getWepDataFromInstance(WeaponInstance* inst);
		//Builds a random ship instance.
		ShipInstance* createRandomShipInstance();
		//Creates the starter ship with two machine guns and a Tuxedo ship.
		ShipInstance* buildStarterShip();

		//Adds a new ship upgrade instance based on the given data with the set value.
		ShipUpgradeInstance* addNewShipUpgradeInstance(ShipUpgradeData* dat, f32 value);
		//Adds a new weapon upgrade instance based on the given data with the set value.
		WeaponUpgradeInstance* addNewWeaponUpgradeInstance(WeaponUpgradeData* dat, f32 value);
		//Adds a random ship upgrade instance with random values.
		ShipUpgradeInstance* addRandomShipUpgrade();
		//Adds a random weapon upgrade instance with random values.
		WeaponUpgradeInstance* addRandomWeaponUpgrade();
		//Removes the ship upgrade from the campaign. Does not delete the instance.
		bool removeShipUpgrade(ShipUpgradeInstance* inst);
		//Removes the weapon upgrade from the campaign. Does not delete the instance.
		bool removeWeaponUpgrade(WeaponUpgradeInstance* inst);
		//Adds the given ship upgrade instance to the given ship instance on the given slot.
		void assignShipUpgrade(ShipInstance* inst, u32 slot, ShipUpgradeInstance* upgrade);
		//Adds the given weapon upgrade instance to the given weapon on the given slot.
		void assignWepUpgrade(WeaponInstance* inst, u32 slot, WeaponUpgradeInstance* upgrade);

		//Adds a ship instance to the campaign hangar.
		bool addShipInstanceToHangar(ShipInstance* inst);
		//Adds a weapon instance to the ship inventory.
		bool addWeapon(WeaponInstance* inst);
		//Removes a ship from the available ships.
		bool removeShipInstance(ShipInstance* inst);
		//Removes a weapon from the available weapons.
		bool removeWeapon(WeaponInstance* inst);

		//Destroys the given ship instance - this removes it from the campaign, removes all its upgrades and weapons, and deletes all of it.
		bool destroyShip(ShipInstance* inst);

		//Gets a ship by the campaign ID. This is not the data ID.
		ShipInstance* getShip(instId uniqueId);
		//Gets a wingman by ID.
		WingmanInstance* getWingman(instId uniqueId);
		//Gets a weapon by the campaign ID. This is not the data ID.
		//This function 
		WeaponInstance* getWeapon(instId uniqueId);
		//Gets a physics weapon by the campaign ID. This is not the data ID.
		WeaponInstance* getPhysWeapon(instId uniqueId);
		//Gets a heavy weapon by the campaign ID. This is not the data ID.
		WeaponInstance* getHeavyWeapon(instId uniqueId);

		//Gets a ship upgrade by the campaign ID. This is not the data ID.
		ShipUpgradeInstance* getShipUpgrade(instId uniqueId);
		//Gets a weapon upgrade by the campaign ID. This is not the data ID.
		WeaponUpgradeInstance* getWeaponUpgrade(instId uniqueId);

		//Strips a ship of weapons and upgrades.
		void stripShip(ShipInstance* inst);
		//Strips a weapon of upgrades.
		void stripWeapon(WeaponInstance* inst);
		//Strips all unassigned ships.
		void stripAllUnassignedShips();
		//Strips all unassigned weapons.
		void stripAllUnassignedWeapons();

		//Gets the current player ship.
		ShipInstance* getPlayerShip() { return getShip(getWingman(0)->assignedShip); }
		//Gets the wingman instance assigned to the player.
		WingmanInstance* getPlayer() { return getWingman(0); }

		//Gets a list of wingmen who are currently available to be recruited.
		std::vector<const WingmanMarker*> getAvailableWingmen();
		//Adds the wingman instance to the campaign.
		void addWingman(WingmanInstance* inst);
		//Gets the wingman instance assigned to the given wing slot.
		WingmanInstance* getAssignedWingman(u32 pos);
		//Sets the wingman instance to the given wingman slot.
		void setAssignedWingman(WingmanInstance* man, u32 pos);
		//Removes the wingman instance from the given wingman slot.
		void removeAssignedWingman(u32 pos);
		//Gets the ship instance assigned to the given wingman slot.
		ShipInstance* getAssignedShip(u32 pos);
		//Sets the ship instance assigned to the given wingman slot.
		void setAssignedShip(ShipInstance* inst, u32 pos);
		//Removes the assigned ship from the given wingman slot.
		void removeAssignedShip(u32 pos);

		//Assignes the given ship and the wingman to each other.
		bool assignWingmanToShip(WingmanInstance* wingman, ShipInstance* ship);

		u32 getDifficulty() const { return currentDifficulty; }

		const std::list<ShipInstance*>& ships() { return m_ships; }
		const std::list<WingmanInstance*>& wingmen() { return m_wingmen; }
		const std::list<WeaponInstance*>& weapons() { return m_weapons; }
		const std::list<WeaponInstance*>& physWeapons() { return m_physWeapons; }
		const std::list<WeaponInstance*>& heavyWeapons() { return m_heavyWeapons; }
		const std::list<ShipUpgradeInstance*>& shipUpgrades() { return m_shipUpgrades; }
		const std::list<WeaponUpgradeInstance*>& weaponUpgrades() { return m_weaponUpgrades; }

		//Gets a dialogue tree by ID.
		DialogueTree getCharacterDialogue(std::wstring id);
		//Gets the dialogues allowed for the current encounter level.
		const std::vector<DialogueTree>& getCurAvailableCharacterDialogues();
		//Sets the given dialogue tree to be used so it can't be played again.
		void setDialogueTreeUsed(std::wstring id, bool used = true);
		//Checks whether or not a given dialogue tree by ID might be used.
		bool isTreeUsed(std::wstring id);
		//Get some random dialogue for the sector, pulling from what's available given who's around.
		void getRandomCharacterDialogues();

		bool isBanterUsed(Banter& banter);
		bool isBanterAvailable(Banter& banter);
		const std::vector<Banter>& getAvailableBanter();
		void setBanterUsed(std::wstring id, bool used = true);
		void loadRandomBanter();

		bool isDialogueAvailable(DialogueTree& tree);
		DialogueTree currentEvent() { return m_currentEvent; }

		bool getFlag(std::wstring flag) { if (m_flags.find(flag) != m_flags.end()) return m_flags[flag]; return false; }
		void setFlag(std::wstring flag, bool set = true) { m_flags[flag] = set; }

		const std::unordered_map<std::string, CarrierUpgrade>& carrierUpgrades() { return m_carrierUpgrades; }
		CarrierUpgrade* getCarrierUpgrade(std::string which) { if (m_carrierUpgrades.find(which) != m_carrierUpgrades.end()) return &m_carrierUpgrades[which]; return nullptr; }
		void increaseCarrierUpgradeTier(std::string which);
		ChaosTheoryStats& getCarrierStats() { return m_carrierStats; }

		void rollForEvent();

		u32 timeSpentInCampaign = 0;
	private:
		std::unordered_map<std::wstring, bool> m_flags;
		void m_loadAllDialogue();
		void m_buildNextSector();
		void m_loadAllCarrierUpgrades();
		void m_loadAllFlags();
		void m_freshCarrierStats(); //FRESH MEAT
		void m_buildSector(SECTOR_TYPE type);
		Sector* currentSector=nullptr;
		u32 currentDifficulty;
		s32 ammunition;
		f32 supplies;

		std::unordered_map<std::wstring, DialogueTree> m_allCharacterDialogues;
		std::vector<DialogueTree> m_availableCharacterDialogues;

		std::unordered_map<std::wstring, Banter> m_allBanter;
		std::vector<Banter> m_availableBanter;

		instId assignedWing[MAX_WINGMEN_ON_WING];
		instId assignedShips[MAX_WINGMEN_ON_WING];
		//WingmanInstance* player;
		instId playerShip = INVALID_DATA_ID;

		std::list<std::wstring> m_allSpeakers;

		std::list<WingmanInstance*> m_wingmen;
		std::list<ShipInstance*> m_ships;
		std::list<WeaponInstance*> m_weapons;
		std::list<WeaponInstance*> m_physWeapons;
		std::list<WeaponInstance*> m_heavyWeapons;
		std::list<ShipUpgradeInstance*> m_shipUpgrades;
		std::list<WeaponUpgradeInstance*> m_weaponUpgrades;

		std::unordered_map<std::wstring, DialogueTree> m_randomEvents;
		std::unordered_map<std::wstring, DialogueTree> m_plotEvents;
		DialogueTree m_currentEvent;

		std::unordered_map<std::string, CarrierUpgrade> m_carrierUpgrades;

		ChaosTheoryStats m_carrierStats;

		instId shipCount = 0;
		instId wepCount = 0;
		instId shipUpgradeCount = 0;
		instId wepUpgradeCount = 0;

};

#endif