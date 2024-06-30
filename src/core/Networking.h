#pragma once

#ifndef NETWORKING_H
#define NETWORKING_H
#include "BaseHeader.h"
#include "LoadoutData.h"
#include <bitpacker.hpp>
#include <algorithm>
#include <stdexcept>
#include <assert.h>
#include <isteamnetworkingsockets.h>

size_type bitsNeeded(int min, int max);

const unsigned int MAX_DATA_SIZE = 8000; // in bytes

//FEAR THE GIGAPACKET BYTE SIZE
//DO NOT USE THIS IN SCENARIOS
//DO NOT USE THIS ANYWHERE OUTSIDE OF SCENARIO GENERATION
const unsigned int GIGAPACKET_SIZE = 65336;

const float MIN_POSITION = -32000.0f;
const float MAX_POSITION = 32000.0f;
const float MIN_VELOCITY = -1000.0f;
const float MAX_VELOCITY = 1000.0f;
const float MIN_ANGULAR_VELOCITY = -16.0f;
const float MAX_ANGULAR_VELOCITY = 16.0f;

const float POSITION_RES = 1/4096.0f;
const float VELOCITY_RES = 1/4096.0f;
const float ORIENTATION_RES = 1/524288.0f;
const float ANGULAR_VELOCITY_RES = 1/4096.0f;

// I don't think MIN_HEALTH will ever change but might as well
const float MIN_HEALTH = 0.0f;
const float MAX_HEALTH = 100000.0f;
const float HEALTH_RES = 1 / 32.0f;
// Same
const float MIN_SHIELDS = 0.0f;
const float MAX_SHIELDS = 100000.0f;
const float SHIELDS_RES = 1 / 32.0f;

const float MIN_HEALTH_REGEN = -1000.0f;
const float MAX_HEALTH_REGEN = 1000.0f;
const float HEALTH_REGEN_RES = 1 / 32.0f;

const float MIN_RECHARGE_RATE = -1000.0f;
const float MAX_RECHARGE_RATE = 1000.0f;
const float RECHARGE_RATE_RES = 1 / 32.0f;

// This one should DEFINITELY never change
const float MIN_RECHARGE_DELAY = 0.0f;
const float MAX_RECHARGE_DELAY = 100000.0f;
const float RECHARGE_DELAY_RES = 1 / 32.0f;

const uint32_t MAX_INT_VALUE_POSITION = static_cast<uint32_t>(ceil((MAX_POSITION - MIN_POSITION) / POSITION_RES));
const size_type POSITION_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_POSITION);
const uint32_t MAX_INT_VALUE_VELOCITY = static_cast<uint32_t>(ceil((MAX_VELOCITY - MIN_VELOCITY) / VELOCITY_RES));
const size_type VELOCITY_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_VELOCITY);
const uint32_t MAX_INT_VALUE_ORIENTATION = static_cast<uint32_t>(ceil(2 / ORIENTATION_RES));
const size_type ORIENTATION_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_ORIENTATION);
const uint32_t MAX_INT_VALUE_ANGULAR_VELOCITY = static_cast<uint32_t>(ceil(MAX_ANGULAR_VELOCITY - MIN_ANGULAR_VELOCITY) / ANGULAR_VELOCITY_RES);
const size_type ANGULAR_VELOCITY_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_ANGULAR_VELOCITY);
const size_type RBC_BITS_NEEDED = 3 * POSITION_BITS_NEEDED + 3 * VELOCITY_BITS_NEEDED + 4 * ORIENTATION_BITS_NEEDED + 3 * ANGULAR_VELOCITY_BITS_NEEDED; // + 2 for the quat index
const size_type RBC_BYTES_NEEDED = ceil(RBC_BITS_NEEDED / 8.0);

const uint32_t MAX_INT_VALUE_HEALTH = static_cast<uint32_t>(ceil((MAX_HEALTH - MIN_HEALTH) / HEALTH_RES));
const size_type HEALTH_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_HEALTH);
const uint32_t MAX_INT_VALUE_SHIELDS = static_cast<uint32_t>(ceil((MAX_SHIELDS - MIN_SHIELDS) / SHIELDS_RES));
const size_type SHIELDS_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_SHIELDS);
const uint32_t MAX_INT_VALUE_HEALTH_REGEN = static_cast<uint32_t>(ceil((MAX_HEALTH_REGEN - MIN_HEALTH_REGEN) / HEALTH_REGEN_RES));
const size_type HEALTH_REGEN_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_HEALTH_REGEN);
const uint32_t MAX_INT_VALUE_RECAHRGE_RATE = static_cast<uint32_t>(ceil((MAX_RECHARGE_RATE - MIN_RECHARGE_RATE) / RECHARGE_RATE_RES));
const size_type RECHARGE_RATE_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_RECAHRGE_RATE);
const uint32_t MAX_INT_VALUE_RECHARGE_DELAY = static_cast<uint32_t>(ceil((MAX_RECHARGE_DELAY - MIN_RECHARGE_DELAY) / RECHARGE_DELAY_RES));
const size_type RECHARGE_DELAY_BITS_NEEDED = bitsNeeded(0, MAX_INT_VALUE_RECHARGE_DELAY);

const size_type MAP_GEN_OBSTACLE_BITS_NEEDED = 11 
	+ 13 * 8 * sizeof(uint32_t) 
	+ 8 * sizeof(uint32_t) * sizeof(MapGenObstacle::extraFloats) / sizeof(f32) 
	+ 8 * sizeof(NetworkId)
	+ 8 * sizeof(NetworkId) * sizeof(MapGenObstacle::turretNetworkIds) / sizeof(NetworkId)
	+ 8 * sizeof(uint32_t) * sizeof(MapGenObstacle::turretIds) / sizeof(dataId)
	+ sizeof(MapGenObstacle::turretArchetype) / sizeof(bool)
	+ 8 * sizeof(uint32_t) * sizeof(MapGenObstacle::turretWepIds) / sizeof(dataId);
const size_type MAP_GEN_OBSTACLE_BYTES_NEEDED = ceil(MAP_GEN_OBSTACLE_BITS_NEEDED / 8.0);

const size_type MAP_GEN_SHIP_BITS_NEEDED = 7 
	+ 9 * 8 * sizeof(uint32_t) 
	+ sizeof(MapGenShip::wepArchetype) / sizeof(bool) 
	+ 8 * sizeof(uint32_t) * sizeof(MapGenShip::hardpoints) / sizeof(dataId) 
	+ 3 * 8 * sizeof(NetworkId) 
	+ 8 * sizeof(NetworkId) * sizeof(MapGenShip::hardpointNetworkIds) / sizeof(NetworkId)
	+ 8 * sizeof(NetworkId) * sizeof(MapGenShip::turretNetworkIds) / sizeof(NetworkId)
	+ 8 * sizeof(uint32_t) * sizeof(MapGenShip::turretIds) / sizeof(dataId)
	+ sizeof(MapGenShip::turretArchetype) / sizeof(bool)
	+ 8 * sizeof(uint32_t) * sizeof(MapGenShip::turretWepIds)/sizeof(dataId);
const size_type MAP_GEN_SHIP_BYTES_NEEDED = ceil(MAP_GEN_SHIP_BITS_NEEDED / 8.0);

const size_type NETWORK_SHOT_BITS_NEEDED = 4 * 8 * sizeof(uint32_t) 
	+ 3 * 8 * sizeof(uint32_t) * sizeof(Network_ShotFired::directions) / sizeof(vector3df) 
	+ 8 * sizeof(uint32_t) * sizeof(Network_ShotFired::projIds) / sizeof(uint32_t);
const size_type NETWORK_SHOT_BYTES_NEEDED = ceil(NETWORK_SHOT_BITS_NEEDED / 8.0);

const size_type HEALTH_COMPONENT_BITS_NEEDED = 2 * HEALTH_BITS_NEEDED
	+ 2 * SHIELDS_BITS_NEEDED
	+ HEALTH_REGEN_BITS_NEEDED
	+ RECHARGE_RATE_BITS_NEEDED
	+ RECHARGE_DELAY_BITS_NEEDED;
const size_type HEALTH_COMPONENT_BYTES_NEEDED = ceil(HEALTH_COMPONENT_BITS_NEEDED / 8.0);

const size_type DAMAGE_BITS = ceil(log2((int)MAX_DAMAGE_TYPES));

const size_type NET_DAMAGE_INSTANCE_BITS_NEEDED = 8 * sizeof(u32)
	+ 2 * 8 * sizeof(NetworkId)
	+ 3 * 8 * sizeof(uint32_t)
	+ DAMAGE_BITS
	+ 8 * sizeof(uint32_t);
const size_type NET_DAMAGE_INSTANCE_BYTES_NEEDED = ceil(NET_DAMAGE_INSTANCE_BITS_NEEDED / 8.0);

const u32 HOST_RANGE = 262144;
const u32 CONNECTED_RANGE = 65536;

const u32 MAX_PACKETS = 5;

typedef uint32_t SequenceNumber;
typedef std::span<bitpacker::byte_type> byte_span;
typedef std::span<const bitpacker::byte_type> const_byte_span;

enum PACKET_TYPE
{
	PTYPE_SCENARIO_DATA,
	PTYPE_GAME_STATE_CHANGE,
	PTYPE_SCENARIO_GEN,
	PTYPE_SCENARIO_GEN_DONE,
	PTYPE_BULLET_CREATION,
	PTYPE_KILL,
	PTYPE_DAMAGE_INSTANCE
};

enum PACKET_SCENARIOGEN_TYPE
{
	PSG_SHIP,
	PSG_WEAPON,
	PSG_OBSTACLE
};

enum NETWORK_COMPONENT_IDS
{
	NC_RIGID_BODY,
	NC_HEALTH
};
#pragma pack(push, 1)
struct PacketData 
{
	PacketData();
	uint32_t networkId;
	NETWORK_COMPONENT_IDS componentId;
	// size of this buffer is determined by its component, gonna need to keep a mapping of componentId's -> buffer sizes
	bitpacker::byte_type componentData[];
};

struct ScenarioGenPacketData
{
	ScenarioGenPacketData();
	PACKET_SCENARIOGEN_TYPE type;
	// size of this buffer is determined by its component, gonna need to keep a mapping of componentId's -> buffer sizes
	bitpacker::byte_type data[];
};

struct BulletCreationPacketData
{
	bitpacker::byte_type data[];
};

struct DamageInstancePacketData
{
	bitpacker::byte_type data[];
};

struct Packet 
{
	Packet();
	SequenceNumber sequenceNumber;
	// TODO: need to send input too
	PACKET_TYPE packetType;
	unsigned int numEntries;
	// data[i] should be reinterpret_cast'd to PacketData
	std::byte data[MAX_DATA_SIZE];
};

struct GigaPacket
{
	GigaPacket();
	PACKET_TYPE packetType;
	unsigned int numEntries;
	std::byte data[GIGAPACKET_SIZE];
};

#pragma pack(pop)


struct AccumulatorEntry
{
	NetworkId id = INVALID_NETWORK_ID;
	uint32_t accumulatedPriority = 0;
	AccumulatorEntry() {}
	AccumulatorEntry(NetworkId id) : id(id) {}
};

uint32_t compress_float(const float f, const float min, const float max, const float res);
float decompress_float(const uint32_t val, const float min, const float max, const float res);

uint32_t compress_int(const int i, const int min, const int max);

struct BulletRigidBodyComponent;

namespace bitpacker
{
	template <>
	void store(span<byte_type> buffer, size_type offset, BulletRigidBodyComponent& value) noexcept;
	template <>
	void store(span<byte_type> buffer, size_type offset, MapGenObstacle& value) noexcept;
	template <>
	void store(span<byte_type> buffer, size_type offset, MapGenShip& value) noexcept;
	template <>
	void store(span<byte_type> buffer, size_type offset, HealthComponent& value) noexcept;
	template <>
	void store(span<byte_type> buffer, size_type offset, Network_ShotFired& value) noexcept;
	template <>
	void store(span<byte_type> buffer, size_type offset, Net_DamageInstance& value) noexcept;
	template <>
	void get(span<const byte_type> buffer, size_type offset, BulletRigidBodyComponent& out) noexcept;
	template <>
	void get(span<const byte_type> buffer, size_type offset, MapGenObstacle& out) noexcept;
	template <>
	void get(span<const byte_type> buffer, size_type offset, MapGenShip& out) noexcept;
	template <>
	void get(span<const byte_type> buffer, size_type offset, HealthComponent& out) noexcept;
	template <>
	void get(span<const byte_type> buffer, size_type offset, Network_ShotFired& out) noexcept;
	template <>
	void get(span<const byte_type> buffer, size_type offset, Net_DamageInstance& out) noexcept;
	
}

void NetworkingUpdate();

EResult sendPacket(HSteamNetConnection conn, Packet* packet, int type=k_nSteamNetworkingSend_Unreliable);
std::vector<Packet> getPackets(HSteamNetConnection conn);

EResult sendGigaPacket(HSteamNetConnection conn, GigaPacket* packet, int type = k_nSteamNetworkingSend_Unreliable);
GigaPacket getGigaPacket(HSteamNetConnection conn);


void initializeNetworkingComponent(flecs::entity ent, uint16_t priority = 1, NetworkId id = INVALID_NETWORK_ID);
void setNetworkIdRange(NetworkId min, NetworkId max);
bool networkIdInMyRange(NetworkId id);
NetworkId generateNewNetworkId();
void destroyNetworkId(NetworkId id);
void clearAllNetworkIds();

#endif
