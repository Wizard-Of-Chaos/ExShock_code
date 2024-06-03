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
	if (RUN_EVERY_TWENTY_FRAMES_BOZO_AND_NOT_BEFORE < 20) return;
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
		auto rbc = game_world->get_alive(accumulatorEntry->entity).get_mut<BulletRigidBodyComponent>();
		auto entry = reinterpret_cast<PacketData*>(&(packet->data) + location);
		// if no more room, done
		if (location + sizeof(PacketData) + RBC_BYTES_NEEDED > MAX_DATA_SIZE)
		{
			break;
		}
		entry->entityId = accumulatorEntry->entity;
		entry->componentId = 0; // TODO: component id enum

		/*
		baedsLogger::log("Rigid body component: \n");
		baedsLogger::log("\tX: " + std::to_string(rbc->rigidBody->getWorldTransform().getOrigin().getX()) + "\n");
		baedsLogger::log("\tY: " + std::to_string(rbc->rigidBody->getWorldTransform().getOrigin().getY()) + "\n");
		baedsLogger::log("\tZ: " + std::to_string(rbc->rigidBody->getWorldTransform().getOrigin().getZ()) + "\n");
		baedsLogger::log("\tVelX: " + std::to_string(rbc->rigidBody->getLinearVelocity().getX()) + "\n");
		baedsLogger::log("\tVelY: " + std::to_string(rbc->rigidBody->getLinearVelocity().getY()) + "\n");
		baedsLogger::log("\tVelZ: " + std::to_string(rbc->rigidBody->getLinearVelocity().getZ()) + "\n");
		baedsLogger::log("\tOriReal: " + std::to_string(rbc->rigidBody->getOrientation().getW()) + "\n");
		baedsLogger::log("\tOriI: " + std::to_string(rbc->rigidBody->getOrientation().getX()) + "\n");
		baedsLogger::log("\tOriJ: " + std::to_string(rbc->rigidBody->getOrientation().getY()) + "\n");
		baedsLogger::log("\tOriK: " + std::to_string(rbc->rigidBody->getOrientation().getZ()) + "\n");
		baedsLogger::log("\tAngularVelX: " + std::to_string(rbc->rigidBody->getAngularVelocity().getX()) + "\n");
		baedsLogger::log("\tAngularVelY: " + std::to_string(rbc->rigidBody->getAngularVelocity().getY()) + "\n");
		baedsLogger::log("\tAngularVelZ: " + std::to_string(rbc->rigidBody->getAngularVelocity().getZ()) + "\n");
		*/
		bitpacker::store<BulletRigidBodyComponent&>(std::span<bitpacker::byte_type>(entry->componentData, RBC_BYTES_NEEDED), 0, *rbc);

		bitpacker::get<>(const_byte_span(entry->componentData, RBC_BYTES_NEEDED), 0, *rbc);
		/*
		baedsLogger::log("Recovered rigid body component: \n");
		baedsLogger::log("\tX: " + std::to_string(rbc->rigidBody->getWorldTransform().getOrigin().getX()) + "\n");
		baedsLogger::log("\tY: " + std::to_string(rbc->rigidBody->getWorldTransform().getOrigin().getY()) + "\n");
		baedsLogger::log("\tZ: " + std::to_string(rbc->rigidBody->getWorldTransform().getOrigin().getZ()) + "\n");
		baedsLogger::log("\tVelX: " + std::to_string(rbc->rigidBody->getLinearVelocity().getX()) + "\n");
		baedsLogger::log("\tVelY: " + std::to_string(rbc->rigidBody->getLinearVelocity().getY()) + "\n");
		baedsLogger::log("\tVelZ: " + std::to_string(rbc->rigidBody->getLinearVelocity().getZ()) + "\n");
		baedsLogger::log("\tOriReal: " + std::to_string(rbc->rigidBody->getOrientation().getW()) + "\n");
		baedsLogger::log("\tOriI: " + std::to_string(rbc->rigidBody->getOrientation().getX()) + "\n");
		baedsLogger::log("\tOriJ: " + std::to_string(rbc->rigidBody->getOrientation().getY()) + "\n");
		baedsLogger::log("\tOriK: " + std::to_string(rbc->rigidBody->getOrientation().getZ()) + "\n");
		baedsLogger::log("\tAngularVelX: " + std::to_string(rbc->rigidBody->getAngularVelocity().getX()) + "\n");
		baedsLogger::log("\tAngularVelY: " + std::to_string(rbc->rigidBody->getAngularVelocity().getY()) + "\n");
		baedsLogger::log("\tAngularVelZ: " + std::to_string(rbc->rigidBody->getAngularVelocity().getZ()) + "\n");
		*/
		// move to next free location, sizeof(entry) will be the size of the struct not including the variable length array at the end
		location += sizeof(PacketData) + RBC_BYTES_NEEDED;
		packet->numEntries++;
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
					auto entry = reinterpret_cast<PacketData*>(&(receivedPacket->data) + received_location);
					if (client.sequenceNumberDict.contains({ entry->entityId, entry->componentId }) && client.sequenceNumberDict.at({ entry->entityId, entry->componentId }) > receivedPacket->sequenceNumber)
					{
						continue;
					}
					if (!game_world->is_alive(entry->entityId)) {
						baedsLogger::errLog("Update for entry on ID " + std::to_string(entry->entityId) + " is not alive!\n");
						continue;
					}
					auto e = game_world->entity(entry->entityId);
					// TODO: branch based on component
					if (e.has<BulletRigidBodyComponent>()) {
						auto entRBC = e.get_mut<BulletRigidBodyComponent>();
						if (entRBC->rigidBody) {
							bitpacker::get<>(std::span<bitpacker::byte_type>(entry->componentData, RBC_BYTES_NEEDED), 0, *entRBC);
						}
						else {
							baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body on its component!\n");
						}
					}
					else {
						baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body component!\n");
					}
					received_location += sizeof(PacketData) + RBC_BYTES_NEEDED;

					client.sequenceNumberDict[{ entry->entityId, entry->componentId }] = receivedPacket->sequenceNumber;
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
				auto entry = reinterpret_cast<PacketData*>(&(receivedPacket->data) + received_location);
				if (stateController->hostSequenceNumberDict.find({ entry->entityId, entry->componentId }) != stateController->hostSequenceNumberDict.end()) {
					if (stateController->hostSequenceNumberDict.at({ entry->entityId, entry->componentId }) > receivedPacket->sequenceNumber) {
						continue;
					}
				}
				if (!game_world->is_alive(entry->entityId)) {
					baedsLogger::errLog("Update for entry on ID " + std::to_string(entry->entityId) + " is not alive!\n");
					continue;
				}
				auto e = game_world->entity(entry->entityId);

				// TODO: branch based on component
				if (e.has<BulletRigidBodyComponent>()) {
					auto entRBC = e.get_mut<BulletRigidBodyComponent>();
					if (entRBC->rigidBody) {
						bitpacker::get<>(std::span<bitpacker::byte_type>(entry->componentData, RBC_BYTES_NEEDED), 0, *entRBC);
					}
					else {
						baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body on its component!\n");
					}
				}
				else {
					baedsLogger::errLog("Entity update [" + entDebugStr(e) + "] asked for entity without a rigid body component!\n");
				}
				received_location += sizeof(PacketData) + RBC_BYTES_NEEDED;

				stateController->hostSequenceNumberDict[{ entry->entityId, entry->componentId }] = receivedPacket->sequenceNumber;
			}

			stateController->hostPacketSlotEmpty[i] = true;
		}
	}
}