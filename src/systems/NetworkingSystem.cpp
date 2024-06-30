#include "NetworkingSystem.h"
#include "Networking.h"
#include "NetworkingComponent.h"
#include "BulletRigidBodyComponent.h"
#include "CrashLogger.h"
#include "GameController.h"
#include "GameStateController.h"

static int RUN_EVERY_TWENTY_FRAMES_BOZO_AND_NOT_BEFORE = 0;

void networkingSystem(flecs::iter it, NetworkingComponent* ncs)
{
	++RUN_EVERY_TWENTY_FRAMES_BOZO_AND_NOT_BEFORE; //lol
	if (RUN_EVERY_TWENTY_FRAMES_BOZO_AND_NOT_BEFORE < 20) 
		return;

	RUN_EVERY_TWENTY_FRAMES_BOZO_AND_NOT_BEFORE = 0;
	baedsLogger::logSystem("Networking");

	// accumulate priority
	for (auto i : it)
	{
		auto e = it.entity(i);
		try {
			if (!e.is_alive()) game_throw("Net entity is NOT ALIVE " + entDebugStr(e) + "\n");
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
		auto nc = &ncs[i];
		try {
			if (!nc->accumulatorEntry) game_throw("Net entity is being hell of weird and doesn't have an actual entry?\n");
			nc->accumulatorEntry->accumulatedPriority += nc->priority;
		}
		catch (gameException e) {
			baedsLogger::errLog(e.what());
			continue;
		}
	}

	struct 
	{
		bool operator()(std::shared_ptr<AccumulatorEntry> a, std::shared_ptr<AccumulatorEntry> b) const { return a->accumulatedPriority > b->accumulatedPriority; }
	} accumulatorCmp;

	// sort list largest to smallest priority
	auto accumulator = &gameController->priorityAccumulator;
	std::sort(accumulator->begin(), accumulator->end(), accumulatorCmp);

	auto packet = gameController->packet;
	packet->packetType = PTYPE_SCENARIO_DATA;
	packet->numEntries = 0;
	packet->sequenceNumber++;
	size_t location = 0;
	// iterate accumulator list and construct packet
	for (auto& accumulatorEntry : gameController->priorityAccumulator)
	{
		if (!networkIdInMyRange(accumulatorEntry->id))
			continue; //don't update on shit we don't control

		if (!stateController->networkToEntityDict.contains(accumulatorEntry->id)) {
			baedsLogger::errLog("No entity found for network ID " + std::to_string(accumulatorEntry->id) + "\n");
			continue;
		}
		flecs::entity entity = stateController->networkToEntityDict.at(accumulatorEntry->id);

		if (!entity.is_alive()) {
			baedsLogger::errLog("Attempting to update dead entity!\n");
			continue;
		}

		if (entity.has<WeaponInfoComponent>())
			continue;
		if (!entity.has<NetworkingComponent>())
		{
			baedsLogger::errLog("Entity " + entDebugStr(entity) + " is missing required components NetworkingComponent");
			continue;
		}
		auto nc = entity.get_mut<NetworkingComponent>();
		
		
		if (entity.has<BulletRigidBodyComponent>())
		{
			auto rbc = entity.get_mut<BulletRigidBodyComponent>();
			if (!rbc->rigidBody) {
				baedsLogger::log("Entity " + entDebugStr(entity) + " is missing a rigid body!\n");
				continue;
			}
			auto entry = reinterpret_cast<PacketData*>(reinterpret_cast<std::byte*>(&(packet->data)) + location);
			// if no more room, done
			if (location + sizeof(PacketData) + RBC_BYTES_NEEDED > MAX_DATA_SIZE)
			{
				break;
			}
			entry->networkId = nc->networkedId;
			entry->componentId = NC_RIGID_BODY; // TODO: component id enum
#if _DEBUG
			auto orientationPreCompress = rbc->rigidBody->getOrientation();
#endif

			bitpacker::store<BulletRigidBodyComponent&>(std::span<bitpacker::byte_type>(entry->componentData, RBC_BYTES_NEEDED), 0, *rbc);

			bitpacker::get<>(const_byte_span(entry->componentData, RBC_BYTES_NEEDED), 0, *rbc);
#if _DEBUG
			auto deltaOrientation = rbc->rigidBody->getOrientation() - orientationPreCompress;
			auto deltaLength = deltaOrientation.length();
			if (deltaLength > 3 * ORIENTATION_RES)
			{
				baedsLogger::log("BIG ORIENTATION JUMP OF " + std::to_string(deltaLength) + "WTF:\n");
				baedsLogger::log("\tX: " + std::to_string(orientationPreCompress.getX()) + "\n");
				baedsLogger::log("\tY: " + std::to_string(orientationPreCompress.getY()) + "\n");
				baedsLogger::log("\tZ: " + std::to_string(orientationPreCompress.getZ()) + "\n");
				baedsLogger::log("\tW: " + std::to_string(orientationPreCompress.getW()) + "\n");
				baedsLogger::log("POST COMPRESSION ORIENTATION:\n");
				baedsLogger::log("\tX: " + std::to_string(rbc->rigidBody->getOrientation().getX()) + "\n");
				baedsLogger::log("\tY: " + std::to_string(rbc->rigidBody->getOrientation().getY()) + "\n");
				baedsLogger::log("\tZ: " + std::to_string(rbc->rigidBody->getOrientation().getZ()) + "\n");
				baedsLogger::log("\tW: " + std::to_string(rbc->rigidBody->getOrientation().getW()) + "\n");
				baedsLogger::log("DELTA ORIENTATION: \n");
				baedsLogger::log("\tX: " + std::to_string(deltaOrientation.getX()) + "\n");
				baedsLogger::log("\tY: " + std::to_string(deltaOrientation.getY()) + "\n");
				baedsLogger::log("\tZ: " + std::to_string(deltaOrientation.getZ()) + "\n");
				baedsLogger::log("\tW: " + std::to_string(deltaOrientation.getW()) + "\n");
			}
#endif
			// move to next free location, sizeof(entry) will be the size of the struct not including the variable length array at the end
			location += sizeof(PacketData) + RBC_BYTES_NEEDED;
			packet->numEntries++;

		}

		if (entity.has<HealthComponent>())
		{
			if (location + sizeof(PacketData) + HEALTH_COMPONENT_BYTES_NEEDED > MAX_DATA_SIZE)
			{
				break;
			}
			auto entry = reinterpret_cast<PacketData*>(reinterpret_cast<std::byte*>(&(packet->data)) + location);
			auto hc = entity.get_mut<HealthComponent>();
			entry->componentId = NC_HEALTH;
			entry->networkId = nc->networkedId;
			bitpacker::store<HealthComponent&>(std::span<bitpacker::byte_type>(entry->componentData, HEALTH_COMPONENT_BYTES_NEEDED), 0, *hc);

			location += sizeof(PacketData) + HEALTH_COMPONENT_BYTES_NEEDED;
		}		
	}
	gameController->isPacketValid = true;
	// packet is constructed, ready to send

	if (stateController->listenSocket)
	{
		for (auto& client : stateController->clientData) 
		{
			if (!client.active) continue;
							
			for (int i = 0; i < sizeof(client.packetBuffer) / sizeof(Packet); i++)
			{
				if (client.packetSlotEmpty[i]) continue;
				auto receivedPacket = &(client.packetBuffer[i]);

				size_t received_location = 0;

				for (auto i = 0; i < receivedPacket->numEntries; i++)
				{
					auto entry = reinterpret_cast<PacketData*>(reinterpret_cast<std::byte*>(&(receivedPacket->data)) + received_location);
					received_location += sizeof(PacketData) + RBC_BYTES_NEEDED;

					if (!stateController->networkToEntityDict.contains(entry->networkId)) 
						continue;

					if (networkIdInMyRange(entry->networkId)) //don't update on owned entities
						continue;

					flecs::entity& entityId = stateController->networkToEntityDict[entry->networkId];
					if (client.sequenceNumberDict.contains({ entityId, entry->componentId }) && client.sequenceNumberDict.at({ entityId, entry->componentId }) > receivedPacket->sequenceNumber)
					{
						continue;
					}
					if (!game_world->is_alive(entityId)) {
						baedsLogger::errLog("Update for entry on ID " + std::to_string(entityId) + " is not alive!\n");
						continue;
					}
					auto e = game_world->entity(entityId);
					bool breakLoop = false;
					switch (entry->componentId)
					{
					case (NC_RIGID_BODY):
					{
						if (!e.has<BulletRigidBodyComponent>())
						{
							baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body component!\n");
							break;
						}

						auto entRBC = e.get_mut<BulletRigidBodyComponent>();
						if (!entRBC->rigidBody)
						{
							baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body on its component!\n");
							break;
						}
						bitpacker::get<>(std::span<bitpacker::byte_type>(entry->componentData, RBC_BYTES_NEEDED), 0, *entRBC);
						received_location += sizeof(PacketData) + RBC_BYTES_NEEDED;
						break;
					}
					case (NC_HEALTH):
					{
						if (!e.has<HealthComponent>())
						{
							baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a health component!\n");
							break;
						}
						auto entHC = e.get_mut<HealthComponent>();
						bitpacker::get<>(std::span<bitpacker::byte_type>(entry->componentData, HEALTH_COMPONENT_BYTES_NEEDED), 0, *entHC);
						received_location += sizeof(PacketData) + HEALTH_COMPONENT_BYTES_NEEDED;
						break;
					}
					default:
					{
						baedsLogger::errLog("Entity with unknown component type [" + std::to_string(entry->componentId) + "]\n");
						breakLoop = true;
						break;
					}
					}
					if (breakLoop) break;

					client.sequenceNumberDict[{ entityId, entry->componentId }] = receivedPacket->sequenceNumber;
				}

				client.packetSlotEmpty[i] = true;
			}
		}
	}
	else if (stateController->hostConnection)
	{
		for (int i = 0; i < sizeof(stateController->hostPacketBuffer) / sizeof(Packet); i++)
		{
			if (stateController->hostPacketSlotEmpty[i]) continue;
			auto receivedPacket = &(stateController->hostPacketBuffer[i]);

			size_t received_location = 0;

			for (auto i = 0; i < receivedPacket->numEntries; i++)
			{
				auto entry = reinterpret_cast<PacketData*>(reinterpret_cast<std::byte*>(&(receivedPacket->data)) + received_location);

				if (!stateController->networkToEntityDict.contains(entry->networkId))
					continue;

				if (networkIdInMyRange(entry->networkId)) //don't update on owned entities
					continue;

				flecs::entity& entityId = stateController->networkToEntityDict[entry->networkId];

				if (stateController->hostSequenceNumberDict.find({ entityId, entry->componentId }) != stateController->hostSequenceNumberDict.end()) {
					if (stateController->hostSequenceNumberDict.at({ entityId, entry->componentId }) > receivedPacket->sequenceNumber) {
						continue;
					}
				}
				if (!game_world->is_alive(entityId)) {
					baedsLogger::errLog("Update for entry on ID " + std::to_string(entityId) + " is not alive!\n");
					continue;
				}
				auto e = game_world->entity(entityId);
				bool breakLoop = false;
				switch (entry->componentId)
				{
				case (NC_RIGID_BODY):
				{
					if (!e.has<BulletRigidBodyComponent>())
					{
						baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body component!\n");
						break;
					}
						
					auto entRBC = e.get_mut<BulletRigidBodyComponent>();
					if (!entRBC->rigidBody) 
					{
						baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body on its component!\n");
						break;
					}
					bitpacker::get<>(std::span<bitpacker::byte_type>(entry->componentData, RBC_BYTES_NEEDED), 0, *entRBC);
					received_location += sizeof(PacketData) + RBC_BYTES_NEEDED;
					break;
				}
				case (NC_HEALTH):
				{
					if (!e.has<HealthComponent>())
					{
						baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a health component!\n");
						break;
					}
					auto entHC = e.get_mut<HealthComponent>();
					bitpacker::get<>(std::span<bitpacker::byte_type>(entry->componentData, HEALTH_COMPONENT_BYTES_NEEDED), 0, *entHC);
					received_location += sizeof(PacketData) + HEALTH_COMPONENT_BYTES_NEEDED;
					break;
				}
				default:
				{
					baedsLogger::errLog("Entity with unknown component type [" + std::to_string(entry->componentId) + "]\n");
					breakLoop = true;
					break;
				}	
				}
				if (breakLoop) break;
				stateController->hostSequenceNumberDict[{ entityId, entry->componentId }] = receivedPacket->sequenceNumber;
			}

			stateController->hostPacketSlotEmpty[i] = true;
		}
	}
}