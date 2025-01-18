/*
 *  This Source Code Form is subject to the terms of the Mozilla Public License,
 *  v. 2.0. If a copy of the MPL was not distributed with this file, You can
 *  obtain one at http://mozilla.org/MPL/2.0/.
 *
 *  The original code is copyright (c) 2022, open.mp team and contributors.
 */

#include "../Types.hpp"
#include "sdk.hpp"
#include <iostream>

SCRIPT_API(CreateActor, int(int skin, Vector3 position, float angle))
{
	IActorsComponent* component = PawnManager::Get()->actors;
	if (component)
	{
		IActor* actor = component->create(skin, position, angle);
		if (actor)
		{
			return actor->getID();
		}
	}
	return INVALID_ACTOR_ID;
}

SCRIPT_API(DestroyActor, bool(IActor& actor))
{
	PawnManager::Get()->actors->release(actor.getID());
	return true;
}

SCRIPT_API(SetActorName, bool(IActor& actor, const std::string& name))
{
	actor.setName(name);
	return true;
}

SCRIPT_API(GetActorName, int(IActor& actor, OutputOnlyString& name))
{
	name = actor.getName();
	return std::get<StringView>(name).length();
}

SCRIPT_API(SetActorNameForPlayer, bool(IActor& actor, const std::string& name, IPlayer& player))
{
	actor.setNameForPlayer(name, player);
	return true;
}

SCRIPT_API(IsActorStreamedIn, bool(IActor& actor, IPlayer& player))
{
	return actor.isStreamedInForPlayer(player);
}

SCRIPT_API(SetActorVirtualWorld, bool(IActor& actor, int virtualWorld))
{
	actor.setVirtualWorld(virtualWorld);
	return true;
}

SCRIPT_API(GetActorVirtualWorld, int(IActor& actor))
{
	return actor.getVirtualWorld();
}

SCRIPT_API(ApplyActorAnimation, bool(IActor& actor, const std::string& animationLibrary, const std::string& animationName, float delta, bool loop, bool lockX, bool lockY, bool freeze, int time))
{
	const AnimationData animationData(delta, loop, lockX, lockY, freeze, time, animationLibrary, animationName);
	actor.applyAnimation(animationData);
	return true;
}

SCRIPT_API(ClearActorAnimations, bool(IActor& actor))
{
	actor.clearAnimations();
	return true;
}

SCRIPT_API(SetActorPos, bool(IActor& actor, Vector3 position))
{
	actor.setPosition(position);
	return true;
}

SCRIPT_API(SetActorPosFindZ, bool(IActor& actor, Vector3 position))
{
	actor.setPositionFindZ(position);
	return true;
}

SCRIPT_API(SetActorPosNoSync, bool(IActor& actor, Vector3 position))
{
	actor.updatePosition(position);
	return true;
}

SCRIPT_API(GetActorPos, bool(IActor& actor, Vector3& position))
{
	position = actor.getPosition();
	return true;
}

SCRIPT_API(SetActorFacingAngle, bool(IActor& actor, float angle))
{
	actor.setRotation(Vector3(0.0f, 0.0f, angle));
	return true;
}

SCRIPT_API(GetActorFacingAngle, bool(IActor& actor, float& angle))
{
	angle = actor.getRotation().ToEuler().z;
	return true;
}

SCRIPT_API(SetActorHealth, bool(IActor& actor, float health))
{
	actor.setHealth(health);
	return true;
}

SCRIPT_API(GetActorHealth, bool(IActor& actor, float& health))
{
	health = actor.getHealth();
	return true;
}

SCRIPT_API(SetActorArmour, bool(IActor& actor, float armour))
{
	actor.setArmour(armour);
	return true;
}

SCRIPT_API(SetActorArmourForPlayer, bool(IActor& actor, float armour, IPlayer& player))
{
	actor.setArmourForPlayer(armour, player);
	return true;
}

SCRIPT_API(GetActorArmour, bool(IActor& actor, float& armour))
{
	armour = actor.getArmour();
	return true;
}

SCRIPT_API(SetActorInvulnerable, bool(IActor& actor, bool invulnerable))
{
	actor.setInvulnerable(invulnerable);
	return true;
}

SCRIPT_API(IsActorInvulnerable, bool(IActor& actor))
{
	return actor.isInvulnerable();
}

SCRIPT_API(IsValidActor, bool(IActor* actor))
{
	return actor != nullptr;
}

SCRIPT_API(SetActorSkin, bool(IActor& actor, int skin))
{
	actor.setSkin(skin);
	return true;
}

SCRIPT_API(GetActorSkin, int(IActor& actor))
{
	return actor.getSkin();
}

SCRIPT_API(SetActorArmedWeapon, bool(IActor& actor, uint32_t weapon))
{
	actor.setWeapon(weapon);
	return true;
}

SCRIPT_API(SetActorArmedWeaponForPlayer, bool(IActor& actor, uint32_t weapon, IPlayer& player))
{
	actor.setWeaponForPlayer(weapon, player);
	return true;
}

SCRIPT_API(GetActorArmedWeapon, int(IActor& actor))
{
	return actor.getWeapon();
}

SCRIPT_API(SetActorAim, bool(IActor& actor, Vector3 position, int time))
{
	actor.setAim(position, time);
	return true;
}

SCRIPT_API(GetActorAnimation, bool(IActor& actor, OutputOnlyString& animationLibrary, OutputOnlyString& animationName, float& delta, bool& loop, bool& lockX, bool& lockY, bool& freeze, int& time))
{
	const AnimationData& anim = actor.getAnimation();

	animationLibrary = anim.lib;
	animationName = anim.name;
	delta = anim.delta;
	loop = anim.loop;
	lockX = anim.lockX;
	lockY = anim.lockY;
	freeze = anim.freeze;
	time = anim.time;

	return true;
}

SCRIPT_API(GetActorSpawnInfo, bool(IActor& actor, int& skin, Vector3& position, float& angle))
{
	const ActorSpawnData& spawnData = actor.getSpawnData();

	position = spawnData.position;
	angle = spawnData.facingAngle;
	skin = spawnData.skin;
	return true;
}

SCRIPT_API(PutActorInVehicle, bool(IActor& actor, IVehicle& vehicle, int seat, bool force))
{
	actor.putInVehicle(vehicle, seat, force);
	return true;
}

SCRIPT_API(RemoveActorFromVehicle, bool(IActor& actor, bool force))
{
	actor.removeFromVehicle(force);
	return true;
}

SCRIPT_API(GetActorVehicle, int(IActor& actor))
{
	return actor.getVehicle();
}

SCRIPT_API(GetActorVehicleSeat, int(IActor& actor))
{
	return actor.getSeat();
}