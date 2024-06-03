#pragma once

#ifndef GAMESTATECONTROLLER_H
#define GAMESTATECONTROLLER_H
#include "BaseHeader.h"
#include "Networking.h"
#include <isteamnetworkingsockets.h>
#include <isteamnetworkingutils.h>
#include <isteamfriends.h>
#include <isteamuser.h>
#include <condition_variable>
#include <mutex>
#include <map>

const uint32_t MAX_PLAYERS = 3;
typedef uint32_t SequenceNumber;
typedef std::function<void()> RenderJob;

/*
* The states the game can be in - you're in menus, the game is going, the game is paused, or the scenario is over and you're waiting to
* return to menus.
*/
enum GAME_STATE
{
	GAME_MENUS = 0,
	GAME_RUNNING = 1,
	GAME_PAUSED = 2,
	GAME_FINISHED = 3
};


struct ClientConnectionData
{
	bool active; // is this slot in use?
	bool finishedLoading = false; //are we done loading a scenario
	CSteamID steamIDUser; // steamid of the player
	uint64_t tickCountLastData; // last time we got data from the player
	HSteamNetConnection connection; // handle for connection to the player
	Packet packetBuffer[5];
	bool packetSlotEmpty[5];

	std::map<std::pair<flecs::entity_t, uint32_t>, SequenceNumber> sequenceNumberDict;

	ClientConnectionData()
	{
		active = false;
		tickCountLastData = 0;
		connection = 0;
		for (int i = 0; i < sizeof(packetSlotEmpty) / sizeof(bool); i++)
		{
			packetSlotEmpty[i] = true;
		}
	}
};

/*
* The game state controller class holds alllll of the things required for the game to actually function properly. It is set as the
* Irrlicht device's event receiver, and updates its various systems with the OnEvent call. It also includes the current state of the game,
* the main loop, the ability to change states and some logic to determine how to handle those state changes.
*/

class GameStateController : public IEventReceiver
{
	public:
		virtual bool OnEvent(const SEvent& event);
		GameStateController();
		void init();
		void networkingInit();

		void loadAssets();
		void mainLoop();
		void logicUpdate();
		void setState(GAME_STATE newState);
		GAME_STATE getState();

		void loadShipAndWeaponData();
		void toggleMenuBackdrop(bool toggle = true);

		bool inCampaign=false;
		bool goToOptions = false;
		void backToCampaign();
		const ECURSOR_ICON crosshair() const { return crosshairId; }
		const bool isInitialized() const { return gameInitialized; }

		ISteamNetworkingSockets* steamNetworkingSockets;
		ISteamNetworkingUtils* steamNetworkingUtils;
		Packet* stateUpdatePacket;
		bool stateUpdatePacketIsValid = false;

		ClientConnectionData clientData[MAX_PLAYERS];
		const bool isHost() const { return listenSocket; }
		const int slot() const { return networkSlot; }
		void setNetworkSlot(int slot) { networkSlot = slot; }
		HSteamListenSocket listenSocket;
		HSteamNetConnection hostConnection;
		Packet hostPacketBuffer[MAX_PACKETS];
		bool hostPacketSlotEmpty[MAX_PACKETS];
		std::map<std::pair<flecs::entity_t, uint32_t>, SequenceNumber> hostSequenceNumberDict;

		void openListenerSocket();
		void closeListenerSocket();
		//Use this for whenever you need to make a texture call that isn't on the render loop.
		void addRenderingJob(RenderJob job) { jobs.push_back(job); }
#if _DEBUG
		void addDebugLine(line3df line) { debugLines.push_back(line); }
#endif 
	private:
		SCursorSprite m_crosshair;
		ECURSOR_ICON crosshairId;
		f32 musicTimer;
		void stateChange();
		u32 then;
		u32 now;

		GAME_STATE state;
		GAME_STATE oldState;
		GAME_STATE nextState;
		bool stateChangeCalled = false;
		bool gameInitialized = false;
		bool returningToCampaign = false;
		std::list<RenderJob> jobs;
		int networkSlot = 0;

		ISceneNode* skybox = nullptr;
		ISceneNode* tux = nullptr;
		ILightSceneNode* light = nullptr;
		ICameraSceneNode* cam = nullptr;
		ISceneNode* flybys[9] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
		ISceneNode* flybys2[8] = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
		u32 loadHangFrames = 0;
		
		STEAM_CALLBACK(GameStateController, OnNetConnectionStatusChanged, SteamNetConnectionStatusChangedCallback_t);
		
		STEAM_CALLBACK(GameStateController, OnGameRichPresenceJoinRequested, GameRichPresenceJoinRequested_t);

#if _DEBUG
		std::vector<line3df> debugLines;
#endif 

};

#endif