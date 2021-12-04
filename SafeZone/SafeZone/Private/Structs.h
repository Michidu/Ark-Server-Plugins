#pragma once

#include "API/ARK/Ark.h"

struct ATriggerBase : AActor
{
	static UClass* GetPrivateStaticClass() { return NativeCall<UClass*>(nullptr, "ATriggerBase.GetPrivateStaticClass"); }
	TSubobjectPtr<UShapeComponent>& CollisionComponentField() { return *GetNativePointerField<TSubobjectPtr<UShapeComponent>*>(this, "ATriggerBase.CollisionComponent"); }
};

struct ATriggerSphere : ATriggerBase
{
	static UClass* GetClass()
	{
		static UClass* Class = Globals::FindClass("Class /Script/Engine.TriggerSphere");

		return Class;
	}
};

struct FGrappleTether
{
	APrimalCharacter* GrappledParentPrimalCharField() { return *GetNativePointerField<APrimalCharacter**>(this, "FGrappleTether.GrappledParentPrimalChar"); }
	APrimalCharacter* GrappleOwnerField() { return *GetNativePointerField<APrimalCharacter**>(this, "FGrappleTether.GrappleOwner"); }

	// Functions

	void BreakTether() { NativeCall<void>(this, "FGrappleTether.BreakTether"); }
};

struct APrimalProjectileGrapplingHook : AShooterProjectile
{
	static UClass* GetPrivateStaticClass() { return NativeCall<UClass*>(nullptr, "APrimalProjectileGrapplingHook.GetPrivateStaticClass"); }
};
