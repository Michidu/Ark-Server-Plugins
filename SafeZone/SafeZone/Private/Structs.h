#pragma once

#include "API/ARK/Ark.h"

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
