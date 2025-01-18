/*
 *  This Source Code Form is subject to the terms of the Mozilla Public License,
 *  v. 2.0. If a copy of the MPL was not distributed with this file, You can
 *  obtain one at http://mozilla.org/MPL/2.0/.
 *
 *  The original code is copyright (c) 2022, open.mp team and contributors.
 */

#include <Impl/pool_impl.hpp>
#include <Server/Components/Actors/actors.hpp>
#include <Server/Components/CustomModels/custommodels.hpp>
#include <Server/Components/Fixes/fixes.hpp>
#include <netcode.hpp>
#include <sdk.hpp>

using namespace Impl;

struct PlayerActorData final : IExtension
{
	PROVIDE_EXT_UID(0xd1bb1d1f96c7e572)
	uint8_t numStreamed = 0;

	void freeExtension() override
	{
		delete this;
	}

	void reset() override
	{
		numStreamed = 0;
	}
};

class Actor final : public IActor, public PoolIDProvider, public NoCopy
{
private:
	HybridString<MAX_ACTOR_NAME + 1> name_;
	int virtualWorld_;
	int16_t skin_;
	int32_t weapon_;
	bool invulnerable_;
	bool animationLoop_;
	Vector3 pos_;
	float angle_;
	float health_;
	float armour_;
	UniqueIDArray<IPlayer, PLAYER_POOL_SIZE> streamedFor_;
	AnimationData animation_;
	int vehicle_;
	int seat_;
	ActorSpawnData spawnData_;
	bool* allAnimationLibraries_;
	bool* validateAnimations_;
	ICustomModelsComponent*& modelsComponent_;
	IFixesComponent* fixesComponent_;
	IVehiclesComponent*& vehiclesComponent_;

	void restream()
	{
		for (IPlayer* player : streamedFor_.entries())
		{
			streamOutForClient(*player);
			streamInForClient(*player);
		}
	}

	void streamInForClient(IPlayer& player)
	{
		NetCode::RPC::ShowActorForPlayer showActorForPlayerRPC(player.getClientVersion() == ClientVersion::ClientVersion_SAMP_03DL);
		showActorForPlayerRPC.ActorID = poolID;
		showActorForPlayerRPC.Name = StringView(name_);
		showActorForPlayerRPC.Angle = angle_;
		showActorForPlayerRPC.Health = health_;
		showActorForPlayerRPC.Armour = armour_;
		showActorForPlayerRPC.Invulnerable = invulnerable_;
		showActorForPlayerRPC.Position = pos_;
		showActorForPlayerRPC.SkinID = skin_;

		if (modelsComponent_)
		{
			modelsComponent_->getBaseModel(showActorForPlayerRPC.SkinID, showActorForPlayerRPC.CustomSkin);
		}

		PacketHelper::send(showActorForPlayerRPC, player);

		if (animationLoop_)
		{
			NetCode::RPC::ApplyActorAnimationForPlayer RPC(animation_);
			RPC.ActorID = poolID;
			PacketHelper::send(RPC, player);
			if (IPlayerFixesData* data = queryExtension<IPlayerFixesData>(player))
			{
				data->applyAnimation(nullptr, this, &animation_);
			}
		}
	}

	void streamOutForClient(IPlayer& player)
	{
		NetCode::RPC::HideActorForPlayer RPC;
		RPC.ActorID = poolID;
		PacketHelper::send(RPC, player);
	}

public:
	void removeFor(int pid, IPlayer& player)
	{
		if (streamedFor_.valid(pid))
		{
			streamedFor_.remove(pid, player);
		}
	}

	Actor(int skin, Vector3 pos, float angle, bool* allAnimationLibraries, bool* validateAnimations, ICustomModelsComponent*& modelsComponent, IFixesComponent* fixesComponent, IVehiclesComponent*& vehiclesComponent)
		: name_("")
		, virtualWorld_(0)
		, skin_(skin)
		, weapon_(PlayerWeapon_Fist)
		, invulnerable_(true)
		, animationLoop_(false)
		, pos_(pos)
		, angle_(angle)
		, health_(100.f)
		, armour_(0.f)
		, spawnData_ { pos, angle, skin }
		, vehicle_(INVALID_VEHICLE_ID)
		, seat_(SEAT_NONE)
		, allAnimationLibraries_(allAnimationLibraries)
		, validateAnimations_(validateAnimations)
		, modelsComponent_(modelsComponent)
		, fixesComponent_(fixesComponent)
		, vehiclesComponent_(vehiclesComponent)
	{
	}

	void setName(StringView name) override
	{
		name_ = name;

		NetCode::RPC::SetActorNameForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Name = StringView(name);
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	void setNameForPlayer(StringView name, IPlayer& player) override
	{
		NetCode::RPC::SetActorNameForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Name = StringView(name);
		PacketHelper::send(RPC, player);
	}

	StringView getName() override
	{
		return name_;
	}

	void setHealth(float health) override
	{
		health_ = health;
		NetCode::RPC::SetActorHealthForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Health = health_;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	float getHealth() const override
	{
		return health_;
	}

	void setArmour(float armour) override
	{
		armour_ = armour;
		NetCode::RPC::SetActorArmourForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Armour = armour;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	void setArmourForPlayer(float armour, IPlayer& player) override
	{
		NetCode::RPC::SetActorArmourForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Armour = armour;
		PacketHelper::send(RPC, player);
	}

	float getArmour() const override
	{
		return armour_;
	}

	void setInvulnerable(bool invuln) override
	{
		invulnerable_ = invuln;
		restream();
	}

	bool isInvulnerable() const override
	{
		return invulnerable_;
	}

	void applyAnimation(const AnimationData& animation) override
	{
		if ((!validateAnimations_ || *validateAnimations_) && !animationLibraryValid(animation.lib, *allAnimationLibraries_))
		{
			return;
		}
		if (fixesComponent_)
		{
			fixesComponent_->clearAnimation(nullptr, this);
		}

		animation_ = animation;

		if (animation_.loop || animation_.freeze)
		{
			animationLoop_ = true;
		}
		else
		{
			animationLoop_ = false;
			animation_.time = 0;
		}

		NetCode::RPC::ApplyActorAnimationForPlayer RPC(animation);
		RPC.ActorID = poolID;
		for (IPlayer* peer : streamedFor_.entries())
		{
			if (IPlayerFixesData* data = queryExtension<IPlayerFixesData>(*peer))
			{
				data->applyAnimation(nullptr, this, &animation);
			}
			PacketHelper::send(RPC, *peer);
		}
	}

	const AnimationData& getAnimation() const override
	{
		return animation_;
	}

	void clearAnimations() override
	{
		if (fixesComponent_)
		{
			fixesComponent_->clearAnimation(nullptr, this);
		}
		animation_.lib.clear();
		animation_.name.clear();
		animationLoop_ = false;

		NetCode::RPC::ClearActorAnimationsForPlayer RPC;
		RPC.ActorID = poolID;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	bool isStreamedInForPlayer(const IPlayer& player) const override
	{
		return streamedFor_.valid(player.getID());
	}

	void streamInForPlayer(IPlayer& player) override
	{
		const int pid = player.getID();
		if (!streamedFor_.valid(pid))
		{
			auto actor_data = queryExtension<PlayerActorData>(player);
			if (actor_data)
			{
				if (actor_data->numStreamed <= MAX_STREAMED_ACTORS)
				{
					++actor_data->numStreamed;
					streamedFor_.add(pid, player);
					streamInForClient(player);
				}
			}
		}
	}

	void streamOutForPlayer(IPlayer& player) override
	{
		const int pid = player.getID();
		if (streamedFor_.valid(pid))
		{
			auto actor_data = queryExtension<PlayerActorData>(player);
			if (actor_data)
			{
				--actor_data->numStreamed;
			}
			streamedFor_.remove(pid, player);
			streamOutForClient(player);
		}
	}

	int getVirtualWorld() const override
	{
		return virtualWorld_;
	}

	void setVirtualWorld(int vw) override
	{
		virtualWorld_ = vw;
	}

	int getID() const override
	{
		return poolID;
	}

	Vector3 getPosition() const override
	{
		return pos_;
	}

	void setPosition(Vector3 position) override
	{
		pos_ = position;

		NetCode::RPC::SetActorPosForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Pos = position;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	void updatePosition(Vector3 position) override
	{
		pos_ = position;
	}

	void putInVehicle(IVehicle& vehicle, int seat, bool force) override
	{
		if (vehiclesComponent_)
		{
			IVehicle* actorVehicle = vehiclesComponent_->get(vehicle_);
			if (actorVehicle)
			{
				actorVehicle->removeActor(this);
			}
		}

		vehicle_ = vehicle.getID();
		seat_ = seat;
		vehicle.addActor(this);

		if (force == true)
		{
			NetCode::RPC::PutActorInVehicleForPlayer RPC;
			RPC.ActorID = poolID;
			RPC.VehicleID = vehicle.getID();
			RPC.SeatID = seat;
			PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
		}
		else
		{
			NetCode::RPC::ActorGoInVehicleForPlayer RPC;
			RPC.ActorID = poolID;
			RPC.VehicleID = vehicle.getID();
			RPC.SeatID = seat;
			PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
		}
	}

	void removeFromVehicle(bool force) override
	{
		if (vehiclesComponent_)
		{
			IVehicle* actorVehicle = vehiclesComponent_->get(vehicle_);
			if (actorVehicle)
			{
				actorVehicle->removeActor(this);
			}
		}

		vehicle_ = INVALID_VEHICLE_ID;
		seat_ = SEAT_NONE;

		NetCode::RPC::RemoveActorFromVehicleForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Force = force;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	int getVehicle() override
	{
		return vehicle_;
	}

	int getSeat() override
	{
		return seat_;
	}

	GTAQuat getRotation() const override
	{
		return GTAQuat(0.f, 0.f, angle_);
	}

	void setRotation(GTAQuat rotation) override
	{
		angle_ = rotation.ToEuler().z;

		NetCode::RPC::SetActorFacingAngleForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Angle = angle_;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	void setSkin(int id) override
	{
		skin_ = id;
		restream();
	}

	int getSkin() const override
	{
		return skin_;
	}

	void setWeapon(uint32_t weapon) override
	{
		weapon_ = weapon;

		NetCode::RPC::SetActorWeaponForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.WeaponID = weapon;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	void setWeaponForPlayer(uint32_t weapon, IPlayer& player) override
	{
		NetCode::RPC::SetActorWeaponForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.WeaponID = weapon;
		PacketHelper::send(RPC, player);
	}

	uint32_t getWeapon() const override
	{
		return weapon_;
	}

	void setAim(Vector3 position, int time) override
	{
		NetCode::RPC::SetActorAimForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Position = position;
		RPC.Time = time;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	void setPositionFindZ(Vector3 position) override
	{
		pos_ = position;

		NetCode::RPC::SetActorPosFindZForPlayer RPC;
		RPC.ActorID = poolID;
		RPC.Position = position;
		PacketHelper::broadcastToSome(RPC, streamedFor_.entries());
	}

	const ActorSpawnData& getSpawnData() override
	{
		return spawnData_;
	}

	~Actor()
	{
		removeFromVehicle(true);
	}

	void destream()
	{
		for (IPlayer* player : streamedFor_.entries())
		{
			auto actor_data = queryExtension<PlayerActorData>(player);
			if (actor_data)
			{
				--actor_data->numStreamed;
			}
			streamOutForClient(*player);
		}
	}
};
