#include "Networking.h"
#include "BulletRigidBodyComponent.h"
#include "GameStateController.h"
#include "GameController.h"
#include "NetworkingComponent.h"
#include "CrashLogger.h"
#include <iomanip>

size_type bitsNeeded(int min, int max)
{
	int delta = max - min;

	return ceil(log2(delta + 1));
}

Packet::Packet() 
{
	sequenceNumber = 0;
	numEntries = 0;
}

GigaPacket::GigaPacket()
{
	numEntries = 0;
}

PacketData::PacketData() 
{
	//entityId = 0;
	componentId = 0;
}

uint32_t compress_float(const float f, const float min, const float max, const float res)
{
	const float delta = max - min;
	const float maxFloatValue = delta / res;
	const uint32_t maxIntValue = static_cast<uint32_t>(ceil(maxFloatValue));
	
	float normalizedValue = std::clamp((f - min) / delta, 0.0f, 1.0f);

	return static_cast<uint32_t>(floor(normalizedValue * maxIntValue + 0.5f));
}

float decompress_float(const uint32_t val, const float min, const float max, const float res)
{
	const float delta = max - min;
	const float maxFloatValue = delta / res;
	const uint32_t maxIntValue = static_cast<uint32_t>(ceil(maxFloatValue));

	double normalizedValue = val / float(maxIntValue);
	
	return normalizedValue * delta + min;
}


/// <summary>
/// Constructs a packet to be sent over the wire for the current scene
/// NOTE: make sure to free()/delete the created packet once you're done with it
/// </summary>
/// <param name="world">world to be serialized and sent over the wire</param>
/// <returns>a packet to be sent over the wire</returns>
Packet* buildPacket(flecs::world world) 
{
	return nullptr;
}

namespace bitpacker
{
	template <>
	void store(span<byte_type> buffer, size_type offset, BulletRigidBodyComponent& value) noexcept
	{
		insert(buffer, offset + 0, POSITION_BITS_NEEDED, compress_float(value.rigidBody->getWorldTransform().getOrigin().getX(), MIN_POSITION, MAX_POSITION, POSITION_RES));
		insert(buffer, offset + POSITION_BITS_NEEDED, POSITION_BITS_NEEDED, compress_float(value.rigidBody->getWorldTransform().getOrigin().getY(), MIN_POSITION, MAX_POSITION, POSITION_RES));
		insert(buffer, offset + 2 * POSITION_BITS_NEEDED, POSITION_BITS_NEEDED, compress_float(value.rigidBody->getWorldTransform().getOrigin().getZ(), MIN_POSITION, MAX_POSITION, POSITION_RES));

		offset += 3 * POSITION_BITS_NEEDED;

		insert(buffer, offset + 0, VELOCITY_BITS_NEEDED, compress_float(value.rigidBody->getLinearVelocity().getX(), MIN_VELOCITY, MAX_VELOCITY, VELOCITY_RES));
		insert(buffer, offset + VELOCITY_BITS_NEEDED, VELOCITY_BITS_NEEDED, compress_float(value.rigidBody->getLinearVelocity().getY(), MIN_VELOCITY, MAX_VELOCITY, VELOCITY_RES));
		insert(buffer, offset + 2 * VELOCITY_BITS_NEEDED, VELOCITY_BITS_NEEDED, compress_float(value.rigidBody->getLinearVelocity().getZ(), MIN_VELOCITY, MAX_VELOCITY, VELOCITY_RES));

		offset += 3 * VELOCITY_BITS_NEEDED;
		// figure out which component is smallest
		btQuaternion orientation = value.rigidBody->getOrientation();

		auto max = -2;
		uint32_t index = 0;

		// find the index of the largest component
		for (int i = 0; i < 4; i++)
		{
			if (abs(orientation[i]) > max)
			{
				max = abs(orientation[i]);
				index = i;
			}
		}

		// force largest value to always be positive (q and -q represent the same orientation)
		if (orientation[index] < 0)
		{
			orientation *= -1;
		}
		// index of missing orientation component to buffer
		insert(buffer, offset, 2, index);

		offset += 2;
		for (int i = 0; i < 4; i++)
		{
			if (i != index)
			{
				// serialize component
				insert(buffer, offset, ORIENTATION_BITS_NEEDED, compress_float(orientation[i], -sqrt(2) / 2, sqrt(2) / 2, ORIENTATION_RES));
				offset += ORIENTATION_BITS_NEEDED;
			}
		}

		// angular velocity

		insert(buffer, offset + 0, ANGULAR_VELOCITY_BITS_NEEDED, compress_float(value.rigidBody->getAngularVelocity().getX(), MIN_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY, ANGULAR_VELOCITY_RES));
		insert(buffer, offset + ANGULAR_VELOCITY_BITS_NEEDED, ANGULAR_VELOCITY_BITS_NEEDED, compress_float(value.rigidBody->getAngularVelocity().getY(), MIN_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY, ANGULAR_VELOCITY_RES));
		insert(buffer, offset + 2 * ANGULAR_VELOCITY_BITS_NEEDED, ANGULAR_VELOCITY_BITS_NEEDED, compress_float(value.rigidBody->getAngularVelocity().getZ(), MIN_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY, ANGULAR_VELOCITY_RES));
	}

	template <>
	void get(span<const byte_type> buffer, size_type offset, BulletRigidBodyComponent& out) noexcept
	{
		// deserialize position
		btVector3 position;

		uint32_t compressed_float = extract<uint32_t>(buffer, offset, POSITION_BITS_NEEDED);
		float decompressed_float = decompress_float(compressed_float, MIN_POSITION, MAX_POSITION, POSITION_RES);
		position.setX(decompressed_float);
		compressed_float = extract<uint32_t>(buffer, offset + POSITION_BITS_NEEDED, POSITION_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_POSITION, MAX_POSITION, POSITION_RES);
		position.setY(decompressed_float);
		compressed_float = extract<uint32_t>(buffer, offset + 2 * POSITION_BITS_NEEDED, POSITION_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_POSITION, MAX_POSITION, POSITION_RES);
		position.setZ(decompressed_float);

		offset += 3 * POSITION_BITS_NEEDED;

		// deserialize linear velocity
		btVector3 velocity;

		compressed_float = extract<uint32_t>(buffer, offset + 0, VELOCITY_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_VELOCITY, MAX_VELOCITY, VELOCITY_RES);
		velocity.setX(decompressed_float);

		compressed_float = extract<uint32_t>(buffer, offset + VELOCITY_BITS_NEEDED, VELOCITY_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_VELOCITY, MAX_VELOCITY, VELOCITY_RES);
		velocity.setY(decompressed_float);

		compressed_float = extract<uint32_t>(buffer, offset + 2 * VELOCITY_BITS_NEEDED, VELOCITY_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_VELOCITY, MAX_VELOCITY, VELOCITY_RES);
		velocity.setZ(decompressed_float);

		offset += 3 * VELOCITY_BITS_NEEDED;

		// deserialize index of missing orientation component
		uint32_t index = extract<uint32_t>(buffer, offset, 2);
		offset += 2;
		


		btQuaternion orientation;
		// w.l.o.g., assume w is largest component
		// w^2 + x^2 + y^2 + z^2 = 1 => w = sqrt(1-x^2-y^2-z^2)
		// when compressing we ensured largest component is always positive
		float missing_component = 1.0f;
		for (int i = 0; i < 4; i++)
		{
			if (i != index)
			{
				compressed_float = extract<uint32_t>(buffer, offset, ORIENTATION_BITS_NEEDED);
				decompressed_float = decompress_float(compressed_float, -sqrt(2) / 2, sqrt(2) / 2, ORIENTATION_RES);
				orientation[i] = decompressed_float;
				missing_component -= decompressed_float * decompressed_float;
				offset += ORIENTATION_BITS_NEEDED;
			}
		}

		orientation[index] = sqrt(missing_component);

		// ensure magnitude hasn't deviated from 1 due to compression artifacts, should cause some imperceptible snapping
		orientation.normalize();

		btVector3 angularVelocity;

		compressed_float = extract<uint32_t>(buffer, offset + 0, ANGULAR_VELOCITY_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY, ANGULAR_VELOCITY_RES);
		angularVelocity.setX(decompressed_float);

		compressed_float = extract<uint32_t>(buffer, offset + ANGULAR_VELOCITY_BITS_NEEDED, ANGULAR_VELOCITY_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY, ANGULAR_VELOCITY_RES);
		angularVelocity.setY(decompressed_float);

		compressed_float = extract<uint32_t>(buffer, offset + 2 * ANGULAR_VELOCITY_BITS_NEEDED, ANGULAR_VELOCITY_BITS_NEEDED);
		decompressed_float = decompress_float(compressed_float, MIN_ANGULAR_VELOCITY, MAX_ANGULAR_VELOCITY, ANGULAR_VELOCITY_RES);
		angularVelocity.setZ(decompressed_float);

		// build new transform and apply extracted velocities
		btTransform transform;

		transform.setIdentity();

		transform.setOrigin(position);
		transform.setRotation(orientation);

		out.rigidBody->setWorldTransform(transform);

		out.rigidBody->setLinearVelocity(velocity);
		out.rigidBody->setAngularVelocity(angularVelocity);
	}

	template <>
	void store(span<byte_type> buffer, size_type offset, MapGenObstacle& value) noexcept
	{
		insert(buffer, offset + 0, 1, value.isShip);
		insert(buffer, offset + 1, 1, value.isWeapon);
		insert(buffer, offset + 2, 8 * sizeof(uint32_t), static_cast<uint32_t>(value.id));
		insert(buffer, offset + 2 + 8 * sizeof(uint32_t), 8 * sizeof(flecs::entity_t), value.entity);
		offset += 8 * sizeof(uint32_t) + 2 + 8 * sizeof(flecs::entity_t);
		
		insert(buffer, offset + 0, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.position.X));
		insert(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.position.Y));
		insert(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.position.Z));
		offset += 3 * 8 * sizeof(uint32_t);

		insert(buffer, offset + 0, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.rotation.X));
		insert(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.rotation.Y));
		insert(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.rotation.Z));
		offset += 3 * 8 * sizeof(uint32_t);

		insert(buffer, offset + 0, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.scale.X));
		insert(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.scale.Y));
		insert(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.scale.Z));
		offset += 3 * 8 * sizeof(uint32_t);

		insert(buffer, offset + 0, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.startLinVel));
		insert(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.startRotVel));
		insert(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.mass));
		offset += 3 * 8 * sizeof(uint32_t);

		insert(buffer, offset + 0, 4, static_cast<uint32_t>(value.faction));
		offset += 4;
		for (int i = 0; i < sizeof(value.extraFloats) / sizeof(f32); i++)
		{
			insert(buffer, offset + 0, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.extraFloats));
			offset += 8 * sizeof(uint32_t);
		}
	}
	
	template<>
	void get(span<const byte_type> buffer, size_type offset, MapGenObstacle& out) noexcept
	{
		out.isShip = extract<bool>(buffer, offset + 0, 1);
		out.isWeapon = extract<bool>(buffer, offset + 1, 1);
		out.id = extract<uint32_t>(buffer, offset + 2, 8 * sizeof(uint32_t));
		out.entity = extract<flecs::entity_t>(buffer, offset + 2 + 8 * sizeof(uint32_t), 8 * sizeof(flecs::entity_t));
		offset += 8 * sizeof(uint32_t) + 2 + 8 * sizeof(flecs::entity_t);

		uint32_t temp;
		temp = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
		out.position.X = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.position.Y = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.position.Z = *reinterpret_cast<f32*>(&temp);
		offset += 3 * 8 * sizeof(uint32_t);

		temp = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
		out.rotation.X = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.rotation.Y = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.rotation.Z = *reinterpret_cast<f32*>(&temp);
		offset += 3 * 8 * sizeof(uint32_t);

		temp = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
		out.scale.X = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.scale.Y = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.scale.Z = *reinterpret_cast<f32*>(&temp);
		offset += 3 * 8 * sizeof(uint32_t);

		temp = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
		out.startLinVel = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.startRotVel = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.mass = *reinterpret_cast<f32*>(&temp);
		offset += 3 * 8 * sizeof(uint32_t);

		out.faction = static_cast<FACTION_TYPE>(extract<uint32_t>(buffer, offset + 0, 4));
		offset += 4;
		for (int i = 0; i < sizeof(out.extraFloats) / sizeof(f32); i++)
		{
			temp = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
			out.extraFloats[i] = *reinterpret_cast<f32*>(&temp);
			offset += 8 * sizeof(uint32_t);
		}
	}

	template <>
	void store(span<byte_type> buffer, size_type offset, MapGenShip& value) noexcept
	{
		insert(buffer, offset + 0, 1, value.isArchetype);
		insert(buffer, offset + 1, 8*sizeof(uint32_t), static_cast<uint32_t>(value.id));
		insert(buffer, offset + 1 + 8 * sizeof(uint32_t), 8 * sizeof(flecs::entity_t), value.entity);
		offset += 1 + 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t);
		for (int i = 0; i < sizeof(value.wepArchetype) / sizeof(bool); i++)
		{
			insert(buffer, offset + 0, 1, value.wepArchetype[i]);
			offset += 1;
		}
		for (int i = 0; i < sizeof(value.hardpointEntities) / sizeof(flecs::entity_t); i++)
		{
			insert(buffer, offset + 0, sizeof(flecs::entity_t), value.hardpointEntities[i]);
			offset += 8 * sizeof(flecs::entity_t);
		}
		for (int i = 0; i < sizeof(value.hardpoints) / sizeof(dataId); i++)
		{
			insert(buffer, offset + 0, 8*sizeof(uint32_t), static_cast<uint32_t>(value.hardpoints[i]));
			offset += 8 * sizeof(uint32_t);
		}

		insert(buffer, offset + 0, 1, value.physArchetype);
		insert(buffer, offset + 1, 8 * sizeof(uint32_t), static_cast<uint32_t>(value.phys));
		insert(buffer, offset + 1 + 8 * sizeof(uint32_t), 8 * sizeof(flecs::entity_t), value.physEntity);
		insert(buffer, offset + 1 + 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t), 1, value.heavyArchetype);
		insert(buffer, offset + 2 + 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t), 8 * sizeof(uint32_t), static_cast<uint32_t>(value.heavy));
		insert(buffer, offset + 2 + 2 * 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t), 8 * sizeof(flecs::entity_t), value.heavyEntity);
		offset += 2 + 2 * 8 * sizeof(uint32_t) + 2 * 8 * sizeof(flecs::entity_t);

		insert(buffer, offset + 0, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.position.X));
		insert(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.position.Y));
		insert(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.position.Z));
		offset += 3 * 8 * sizeof(uint32_t);

		insert(buffer, offset + 0, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.rotation.X));
		insert(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.rotation.Y));
		insert(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.rotation.Z));
		offset += 3 * 8 * sizeof(uint32_t);

		insert(buffer, offset + 0, 4, static_cast<uint32_t>(value.faction));
	}
	template <>
	void get(span<const byte_type> buffer, size_type offset, MapGenShip& out) noexcept
	{
		out.isArchetype = extract<bool>(buffer, offset + 0, 1);
		out.id = extract<uint32_t>(buffer, offset + 1, 8 * sizeof(uint32_t));
		out.entity = extract<flecs::entity_t>(buffer, offset + 1 + 8 * sizeof(uint32_t), 8 * sizeof(flecs::entity_t));
		offset += 1 + 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t);

		for (int i = 0; i < sizeof(out.wepArchetype) / sizeof(bool); i++)
		{
			out.wepArchetype[i] = extract<bool>(buffer, offset + 0, 1);
			offset += 1;
		}
		for (int i = 0; i < sizeof(out.hardpointEntities) / sizeof(flecs::entity_t); i++)
		{
			out.hardpointEntities[i] = extract<flecs::entity_t>(buffer, offset + 0, sizeof(flecs::entity_t));
			offset += 8 * sizeof(flecs::entity_t);
		}
		for (int i = 0; i < sizeof(out.hardpoints) / sizeof(dataId); i++)
		{
			out.hardpoints[i] = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
			offset += 8 * sizeof(uint32_t);
		}

		out.physArchetype = extract<bool>(buffer, offset + 0, 1);
		out.phys = extract<uint32_t>(buffer, offset + 1, 8 * sizeof(uint32_t));
		out.physEntity = extract<flecs::entity_t>(buffer, offset + 1 + 8 * sizeof(uint32_t), 8 * sizeof(flecs::entity_t));
		out.heavyArchetype = extract<bool>(buffer, offset + 1 + 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t), 1);
		out.heavy = extract<uint32_t>(buffer, offset + 2 + 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t), 8 * sizeof(uint32_t));
		out.heavyEntity = extract<flecs::entity_t>(buffer, offset + 2 + 2 * 8 * sizeof(uint32_t) + 8 * sizeof(flecs::entity_t), 8 * sizeof(flecs::entity_t));
		offset += 2 + 2 * 8 * sizeof(uint32_t) + 2 * 8 * sizeof(flecs::entity_t);

		uint32_t temp;
		temp = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
		out.position.X = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.position.Y = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.position.Z = *reinterpret_cast<f32*>(&temp);
		offset += 3 * 8 * sizeof(uint32_t);

		temp = extract<uint32_t>(buffer, offset + 0, 8 * sizeof(uint32_t));
		out.rotation.X = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.rotation.Y = *reinterpret_cast<f32*>(&temp);
		temp = extract<uint32_t>(buffer, offset + 2 * 8 * sizeof(uint32_t), 8 * sizeof(uint32_t));
		out.rotation.Z = *reinterpret_cast<f32*>(&temp);
		offset += 3 * 8 * sizeof(uint32_t);

		out.faction = static_cast<FACTION_TYPE>(extract<uint32_t>(buffer, offset + 0, 4));
	}
	template <>
	void store(span<byte_type> buffer, size_type offset, HealthComponent& value) noexcept
	{
		insert(buffer, offset, HEALTH_BITS_NEEDED, compress_float(value.health, MIN_HEALTH, MAX_HEALTH, HEALTH_RES));
		offset += HEALTH_BITS_NEEDED;
		insert(buffer, offset, HEALTH_BITS_NEEDED, compress_float(value.maxHealth, MIN_HEALTH, MAX_HEALTH, HEALTH_RES));
		offset += HEALTH_BITS_NEEDED;
		insert(buffer, offset, HEALTH_REGEN_BITS_NEEDED, compress_float(value.healthRegen, MIN_HEALTH_REGEN, MAX_HEALTH_REGEN, HEALTH_REGEN_RES));
		offset += HEALTH_REGEN_BITS_NEEDED;
		insert(buffer, offset, SHIELDS_BITS_NEEDED, compress_float(value.shields, MIN_SHIELDS, MAX_SHIELDS, SHIELDS_RES));
		offset += SHIELDS_BITS_NEEDED;
		insert(buffer, offset, SHIELDS_BITS_NEEDED, compress_float(value.shields, MIN_SHIELDS, MAX_SHIELDS, SHIELDS_RES));
		offset += SHIELDS_BITS_NEEDED;
		insert(buffer, offset, RECHARGE_RATE_BITS_NEEDED, compress_float(value.rechargeRate, MIN_RECHARGE_RATE, MAX_RECHARGE_RATE, RECHARGE_RATE_RES));
		offset += RECHARGE_RATE_BITS_NEEDED;
		insert(buffer, offset, RECHARGE_DELAY_BITS_NEEDED, compress_float(value.rechargeDelay, MIN_RECHARGE_DELAY, MAX_RECHARGE_DELAY, RECHARGE_DELAY_RES));
		offset += RECHARGE_DELAY_RES;
	}
	template <>
	void get(span<const byte_type> buffer, size_type offset, HealthComponent& out) noexcept
	{
		out.health = decompress_float(extract<uint32_t>(buffer, offset, HEALTH_BITS_NEEDED), MIN_HEALTH, MAX_HEALTH, HEALTH_RES);
		offset += HEALTH_BITS_NEEDED;
		out.maxHealth = decompress_float(extract<uint32_t>(buffer, offset, HEALTH_BITS_NEEDED), MIN_HEALTH, MAX_HEALTH, HEALTH_RES);
		offset += HEALTH_BITS_NEEDED;
		out.healthRegen = decompress_float(extract<uint32_t>(buffer, offset, HEALTH_REGEN_BITS_NEEDED), MIN_HEALTH_REGEN, MAX_HEALTH_REGEN, HEALTH_REGEN_RES);
		offset += HEALTH_REGEN_BITS_NEEDED;
		out.shields = decompress_float(extract<uint32_t>(buffer, offset, SHIELDS_BITS_NEEDED), MIN_SHIELDS, MAX_SHIELDS, SHIELDS_RES);
		offset += SHIELDS_BITS_NEEDED;
		out.rechargeRate = decompress_float(extract<uint32_t>(buffer, offset, RECHARGE_RATE_BITS_NEEDED), MIN_RECHARGE_RATE, MAX_RECHARGE_RATE, RECHARGE_RATE_RES);
		offset += RECHARGE_RATE_BITS_NEEDED;
		out.rechargeDelay = decompress_float(extract<uint32_t>(buffer, offset, RECHARGE_DELAY_BITS_NEEDED), MIN_RECHARGE_DELAY, MAX_RECHARGE_DELAY, RECHARGE_DELAY_RES);
	}
	template <>
	void store(span<byte_type> buffer, size_type offset, Network_ShotFired& value) noexcept
	{
		insert(buffer, offset, 8 * sizeof(flecs::entity_t), value.firingWeapon);
		offset += 8 * sizeof(flecs::entity_t);
		for (int i = 0; i < sizeof(value.directions) / sizeof(vector3df); i++)
		{
			insert(buffer, offset, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.directions[i].X));
			offset += 8 * sizeof(uint32_t);
			insert(buffer, offset, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.directions[i].Y));
			offset += 8 * sizeof(uint32_t);
			insert(buffer, offset, 8 * sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.directions[i].Z));
			offset += 8 * sizeof(uint32_t);
		}

		insert(buffer, offset, 8*sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.spawn.X));
		offset += 8 * sizeof(uint32_t);
		insert(buffer, offset, 8*sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.spawn.Y));
		offset += 8 * sizeof(uint32_t);
		insert(buffer, offset, 8*sizeof(uint32_t), *reinterpret_cast<uint32_t*>(&value.spawn.Z));
		offset += 8 * sizeof(uint32_t);

		for (int i = 0; i < sizeof(value.projIds) / sizeof(flecs::entity_t); i++)
		{
			insert(buffer, offset, 8 * sizeof(flecs::entity_t), value.projIds[i]);
			offset += 8 * sizeof(flecs::entity_t);
		}
	}
	template <>
	void get(span<const byte_type> buffer, size_type offset, Network_ShotFired& out) noexcept
	{
		out.firingWeapon = extract<flecs::entity_t>(buffer, offset, 8 * sizeof(flecs::entity_t));
		offset += 8 * sizeof(flecs::entity_t);
		uint32_t temp = 0;
		for (int i = 0; i < sizeof(out.directions) / sizeof(vector3df); i++)
		{
			temp = extract<uint32_t>(buffer, offset, 8 * sizeof(uint32_t));
			out.directions[i].X = *reinterpret_cast<f32*>(&temp);
			offset += 8 * sizeof(uint32_t);
			temp = extract<uint32_t>(buffer, offset, 8 * sizeof(uint32_t));
			out.directions[i].Y = *reinterpret_cast<f32*>(&temp);
			offset += 8 * sizeof(uint32_t);
			temp = extract<uint32_t>(buffer, offset, 8 * sizeof(uint32_t));
			out.directions[i].Z = *reinterpret_cast<f32*>(&temp);
			offset += 8 * sizeof(uint32_t);
		}
		temp = extract<uint32_t>(buffer, offset, 8 * sizeof(uint32_t));
		out.spawn.X = *reinterpret_cast<f32*>(&temp);
		offset += 8 * sizeof(uint32_t);
		temp = extract<uint32_t>(buffer, offset, 8 * sizeof(uint32_t));
		out.spawn.Y = *reinterpret_cast<f32*>(&temp);
		offset += 8 * sizeof(uint32_t);
		temp = extract<uint32_t>(buffer, offset, 8 * sizeof(uint32_t));
		out.spawn.Z = *reinterpret_cast<f32*>(&temp);
		offset += 8 * sizeof(uint32_t);

		for (int i = 0; i < sizeof(out.projIds) / sizeof(flecs::entity_t); i++)
		{
			out.projIds[i] = extract<flecs::entity_t>(buffer, offset, 8 * sizeof(flecs::entity_t));
			offset += 8 * sizeof(flecs::entity_t);
		}
	}
}


void initializeNetworkingComponent(flecs::entity ent, uint16_t priority)
{
	NetworkingComponent n;
	n.priority = priority;
	n.accumulatorEntry = std::shared_ptr<AccumulatorEntry>(new AccumulatorEntry(ent));
	gameController->priorityAccumulator.push_back(n.accumulatorEntry);
	ent.set<NetworkingComponent>(n);
}

EResult sendPacket(HSteamNetConnection conn, Packet* packet, int type)
{
	baedsLogger::log("Load packet seq number: " + std::to_string(packet->sequenceNumber) + "\n");
	baedsLogger::log("Load packet packet type: " + std::to_string(packet->packetType) + "\n");
	baedsLogger::log("Load packet num entries: " + std::to_string(packet->numEntries) + "\n");
	EResult res = stateController->steamNetworkingSockets->SendMessageToConnection(conn, packet, sizeof(Packet), type, nullptr);
	if (res == k_EResultFail) {
		baedsLogger::errLog("Failed to send packet to connection " + std::to_string(conn) + "!\n");
	}
	baedsLogger::log("Sent packet to client " + std::to_string(conn) + ", result " + std::to_string(res) + "\n");
	return res;
}

std::vector<Packet> getPackets(HSteamNetConnection conn)
{
	std::vector<Packet> packets;
	SteamNetworkingMessage_t* buffer[MAX_PACKETS];
	int count = stateController->steamNetworkingSockets->ReceiveMessagesOnConnection(conn, buffer, MAX_PACKETS);
	for (u32 i = 0; i < count; ++i) {
		packets.push_back(Packet());
		Packet& p = packets.back();

		auto receivedPacket = reinterpret_cast<const Packet*>(buffer[i]->m_pData);
		baedsLogger::log("Received packet seq number: " + std::to_string(receivedPacket->sequenceNumber) + "\n");
		baedsLogger::log("Received packet packet type: " + std::to_string(receivedPacket->packetType) + "\n");
		baedsLogger::log("Received packet num entries: " + std::to_string(receivedPacket->numEntries) + "\n");
		std::memcpy(&p, receivedPacket, sizeof(Packet));
		baedsLogger::log("Memcopied packet seq number: " + std::to_string(p.sequenceNumber) + "\n");
		baedsLogger::log("Memcopied packet packet type: " + std::to_string(p.packetType) + "\n");
		baedsLogger::log("Memcopied packet num entries: " + std::to_string(p.numEntries) + "\n");
		buffer[i]->Release();
	}
	return packets;
}

EResult sendGigaPacket(HSteamNetConnection conn, GigaPacket* packet, int type)
{
	baedsLogger::log("Load **GIGAPACKET** packet type: " + std::to_string(packet->packetType) + "\n");
	baedsLogger::log("Load **GIGAPACKET** num entries: " + std::to_string(packet->numEntries) + "\n");

	EResult res = stateController->steamNetworkingSockets->SendMessageToConnection(conn, packet, sizeof(GigaPacket), type, nullptr);
	if (res == k_EResultFail) {
		baedsLogger::errLog("Failed to send **GIGAPACKET** to connection " + std::to_string(conn) + "!\n");
	}
	baedsLogger::log("Sent **GIGAPACKET** to client " + std::to_string(conn) + ", result " + std::to_string(res) + "\n");
	return res;
}

GigaPacket getGigaPacket(HSteamNetConnection conn)
{
	GigaPacket packet;
	SteamNetworkingMessage_t* buffer[MAX_PACKETS];
	int count = stateController->steamNetworkingSockets->ReceiveMessagesOnConnection(conn, buffer, MAX_PACKETS);
	for (u32 i = 0; i < count; ++i) {

		auto receivedPacket = reinterpret_cast<const GigaPacket*>(buffer[i]->m_pData);
		baedsLogger::log("Received **GIGAPACKET** packet type: " + std::to_string(receivedPacket->packetType) + "\n");
		baedsLogger::log("Received **GIGAPACKET** num entries: " + std::to_string(receivedPacket->numEntries) + "\n");
		std::memcpy(&packet, receivedPacket, sizeof(GigaPacket));
		baedsLogger::log("Memcopied **GIGAPACKET** packet type: " + std::to_string(packet.packetType) + "\n");
		baedsLogger::log("Memcopied **GIGAPACKET** num entries: " + std::to_string(packet.numEntries) + "\n");
	}
	return packet;
}


void NetworkingUpdate() 
{
	SteamNetworkingMessage_t* messages[MAX_PACKETS];

	if (stateController->listenSocket)
	{
		if (stateController->stateUpdatePacketIsValid) {
			baedsLogger::log("State update packet to send\n");
		}
		if (gameController->isPacketValid) {
			//baedsLogger::log("Normal package to send\n");
		}

		gameController->networkShotPacket.packetType = PTYPE_BULLET_CREATION;
		gameController->networkShotPacket.sequenceNumber++;
		gameController->networkShotPacket.numEntries = 0;
		size_t location = 0;

		while (!gameController->m_networkShotsToSend.empty() && location + NETWORK_SHOT_BYTES_NEEDED < MAX_DATA_SIZE)
		{
			auto networkShotToSend = gameController->m_networkShotsToSend.back();
			auto entry = reinterpret_cast<BulletCreationPacketData*>(((std::byte*)(&gameController->networkShotPacket.data)) + location);
			bitpacker::store<Network_ShotFired&>(byte_span(entry->data, NETWORK_SHOT_BYTES_NEEDED), 0, networkShotToSend);
			location += NETWORK_SHOT_BYTES_NEEDED;
			gameController->networkShotPacket.numEntries++;
		}
		Packet deadEntityPacket;
		deadEntityPacket.packetType = PTYPE_KILL;
		deadEntityPacket.sequenceNumber = 0;
		deadEntityPacket.numEntries = 0;
		location = 0;
		while (!gameController->getNetworkDeadEntities().empty() && location + sizeof(flecs::entity_t) < MAX_DATA_SIZE)
		{
			flecs::entity deadEntity = gameController->getNetworkDeadEntities().back();
			auto entry = reinterpret_cast<flecs::entity_t*>(((std::byte*)(&deadEntityPacket.data)) + location);
			*entry = deadEntity.id();
			location += sizeof(flecs::entity_t);
			gameController->getNetworkDeadEntities().pop_back();
		}

		for (auto& client : stateController->clientData)
		{
			if (!client.active) continue;
			int numMessages = stateController->steamNetworkingSockets->ReceiveMessagesOnConnection(client.connection, messages, MAX_PACKETS);
			//if(numMessages > 0) baedsLogger::log("We got mail! num messages:" + std::to_string(numMessages) + "\n");
			for (int i = 0; i < numMessages; i++)
			{
				if (messages[i]->m_cbSize <= 0) {
					messages[i]->Release();
					continue;
				}
				auto receivedPacket = reinterpret_cast<const Packet*>(messages[i]->m_pData);
				switch (receivedPacket->packetType)
				{
				case PTYPE_SCENARIO_DATA:
					for (int slot = 0; slot < sizeof(client.packetBuffer) / sizeof(Packet); slot++)
					{
						if (client.packetSlotEmpty[slot])
						{
							std::memcpy(&(client.packetBuffer[slot]), receivedPacket, sizeof(Packet));
							client.packetSlotEmpty[slot] = false;
							break;
						}
					}
					break;
				case PTYPE_GAME_STATE_CHANGE:
					// wtf how'd we get here
					break;
				case PTYPE_BULLET_CREATION:
				{
					Network_ShotFired networkShot;
					size_t location = 0;
					for (int j = 0; j < receivedPacket->numEntries; j++)
					{
						auto entry = reinterpret_cast<const BulletCreationPacketData*>(&(receivedPacket->data) + location);
						bitpacker::get<>(const_byte_span(entry->data, NETWORK_SHOT_BYTES_NEEDED), 0, networkShot);
						gameController->addShotFromNetwork(networkShot);
						location += NETWORK_SHOT_BYTES_NEEDED;
					}
					break;
				}
				case PTYPE_KILL:
				{
					size_t location = 0;
					for (int j = 0; j < receivedPacket->numEntries; j++)
					{
						auto entity = *reinterpret_cast<const flecs::entity_t*>(&(receivedPacket->data) + location);
						auto ent = game_world->entity(entity);
						gameController->markForDeath(ent);
						location += sizeof(flecs::entity_t);
					}
				}
				}
				messages[i]->Release();
			}
			if (gameController->isPacketValid)
			{
				auto res = stateController->steamNetworkingSockets->SendMessageToConnection(client.connection, gameController->packet, sizeof(Packet), k_nSteamNetworkingSend_Unreliable, nullptr);
			}
			if (stateController->stateUpdatePacketIsValid)
			{
				baedsLogger::log("Sending state update change to connection " + std::to_string(client.connection) + " (packet size " + std::to_string(sizeof(Packet)) + "), protocol " + std::to_string(k_nSteamNetworkingSend_Reliable));
				auto res = stateController->steamNetworkingSockets->SendMessageToConnection(client.connection, stateController->stateUpdatePacket, sizeof(Packet), k_nSteamNetworkingSend_Reliable, nullptr);
				
				stateController->stateUpdatePacketIsValid = false;
				baedsLogger::log(" | returned " + std::to_string(res) + "\n");
			}
			if (gameController->networkShotPacket.numEntries > 0)
			{
				auto res = stateController->steamNetworkingSockets->SendMessageToConnection(client.connection, &gameController->networkShotPacket, sizeof(Packet), k_nSteamNetworkingSend_Reliable, nullptr);
			}
			if (deadEntityPacket.numEntries > 0)
			{
				auto res = stateController->steamNetworkingSockets->SendMessageToConnection(stateController->hostConnection, &deadEntityPacket, sizeof(Packet), k_nSteamNetworkingSend_Reliable, nullptr);
			}
		}
		gameController->isPacketValid = false;
		stateController->stateUpdatePacketIsValid = false;
		gameController->m_networkShotsToSend.clear();
	}
	else if (stateController->hostConnection)
	{
		int numMessages = stateController->steamNetworkingSockets->ReceiveMessagesOnConnection(stateController->hostConnection, messages, MAX_PACKETS);
		//if (numMessages > 0) baedsLogger::log("We got mail! num messages:" + std::to_string(numMessages) + "\n");
		for (int i = 0; i < numMessages; i++)
		{
			if (messages[i]->m_cbSize <= 0) continue;
			auto receivedPacket = reinterpret_cast<const Packet*>(messages[i]->m_pData);

			switch (receivedPacket->packetType)
			{
			case PTYPE_SCENARIO_DATA:
				for (int slot = 0; slot < sizeof(stateController->hostPacketBuffer) / sizeof(Packet); slot++)
				{
					if (stateController->hostPacketSlotEmpty[slot])
					{
						std::memcpy(&(stateController->hostPacketBuffer[slot]), receivedPacket, sizeof(Packet));
						stateController->hostPacketSlotEmpty[slot] = false;
						break;
					}
				}
				break;
			case PTYPE_GAME_STATE_CHANGE:
			{
				std::memcpy((stateController->stateUpdatePacket), receivedPacket, sizeof(Packet));
				stateController->stateUpdatePacketIsValid = true;
				GAME_STATE state = *reinterpret_cast<GAME_STATE*>(stateController->stateUpdatePacket->data);
				//check state validity
				if (state != GAME_FINISHED && state != GAME_RUNNING && state != GAME_PAUSED && state != GAME_MENUS) {
					baedsLogger::errLog("State " + std::to_string(state) + " is not valid.\n");
					stateController->stateUpdatePacketIsValid = false;
				}
				else {
					baedsLogger::log("Network state update received, switching to state " + std::to_string(state) + "\n");
					stateController->setState(state);
				}
				stateController->stateUpdatePacketIsValid = false;
				break;
			}
			case PTYPE_BULLET_CREATION:
			{
				Network_ShotFired networkShot;
				size_t location = 0;
				for (int j = 0; j < receivedPacket->numEntries; j++)
				{
					auto entry = reinterpret_cast<const BulletCreationPacketData*>(&(receivedPacket->data) + location);
					bitpacker::get<>(const_byte_span(entry->data, NETWORK_SHOT_BYTES_NEEDED), 0, networkShot);
					gameController->addShotFromNetwork(networkShot);
					location += NETWORK_SHOT_BYTES_NEEDED;
				}
				break;
			}
			case PTYPE_KILL:
			{
				size_t location = 0;
				for (int j = 0; j < receivedPacket->numEntries; j++)
				{
					auto entity = *reinterpret_cast<const flecs::entity_t*>(&(receivedPacket->data) + location);
					auto ent = game_world->entity(entity);
					gameController->markForDeath(ent);
					location += sizeof(flecs::entity_t);
				}
			}
			}
			messages[i]->Release();
		}

		if (gameController->isPacketValid)
		{
			auto res = stateController->steamNetworkingSockets->SendMessageToConnection(stateController->hostConnection, gameController->packet, sizeof(Packet), k_nSteamNetworkingSend_Unreliable, nullptr);
			gameController->isPacketValid = false;
		}
		gameController->networkShotPacket.packetType = PTYPE_BULLET_CREATION;
		gameController->networkShotPacket.sequenceNumber++;
		gameController->networkShotPacket.numEntries = 0;
		size_t location = 0;

		while (!gameController->m_networkShotsToSend.empty() && location + NETWORK_SHOT_BYTES_NEEDED < MAX_DATA_SIZE)
		{
			auto networkShotToSend = gameController->m_networkShotsToSend.back();
			auto entry = reinterpret_cast<BulletCreationPacketData*>(((std::byte*)(&gameController->networkShotPacket.data)) + location);
			bitpacker::store<Network_ShotFired&>(byte_span(entry->data, NETWORK_SHOT_BYTES_NEEDED), 0, networkShotToSend);
			location += NETWORK_SHOT_BYTES_NEEDED;
			gameController->networkShotPacket.numEntries++;
		}
		if (gameController->networkShotPacket.numEntries > 0)
		{
			auto res = stateController->steamNetworkingSockets->SendMessageToConnection(stateController->hostConnection, &gameController->networkShotPacket, sizeof(Packet), k_nSteamNetworkingSend_Reliable, nullptr);
		}
		Packet deadEntityPacket;
		deadEntityPacket.packetType = PTYPE_KILL;
		deadEntityPacket.sequenceNumber = 0;
		deadEntityPacket.numEntries = 0;
		location = 0;
		while (!gameController->getNetworkDeadEntities().empty() && location + sizeof(flecs::entity_t) < MAX_DATA_SIZE)
		{
			flecs::entity deadEntity = gameController->getNetworkDeadEntities().back();
			auto entry = reinterpret_cast<flecs::entity_t*>(((std::byte*)(&deadEntityPacket.data)) + location);
			*entry = deadEntity.id();
			location += sizeof(flecs::entity_t);
			gameController->getNetworkDeadEntities().pop_back();
		}
		if (deadEntityPacket.numEntries > 0)
		{
			auto res = stateController->steamNetworkingSockets->SendMessageToConnection(stateController->hostConnection, &deadEntityPacket, sizeof(Packet), k_nSteamNetworkingSend_Reliable, nullptr);
		}
	}
}

