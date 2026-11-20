#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FCProjectileProviderInterface.generated.h"

class AFCProjectileBase;
class UFCProjectileDataAsset;

UINTERFACE(MinimalAPI, Blueprintable)
class UFCProjectileProviderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * IFCProjectileProviderInterface
 *
 * Dependency Inversion contract allowing avatars (mobs, bosses, turrets) to supply
 * custom projectile class or DataAsset overrides without Cast<AFCMobCharacter>() in GAS abilities.
 */
class FC_API IFCProjectileProviderInterface
{
	GENERATED_BODY()

public:
	virtual TSubclassOf<AFCProjectileBase> GetProjectileClassOverrideForAbility() const = 0;
	virtual const UFCProjectileDataAsset* GetProjectileDataAssetOverrideForAbility() const = 0;
};
