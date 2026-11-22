#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCardTargetingComponent.generated.h"

class UFCCardDataAsset;

/**
 * UFCCardTargetingComponent
 *
 * Modular client-side card targeting component attached to AFCPlayerController.
 * Performs screen-to-world raycasts and delegates strategy resolution to each card's
 * polymorphic TInstancedStruct TargetPolicy (FFCCardTargetPolicyBase).
 */
UCLASS(ClassGroup = (Card), meta = (BlueprintSpawnableComponent))
class FC_API UFCCardTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFCCardTargetingComponent();

	/** Resolves targeting info under the owning controller's current mouse cursor */
	UFUNCTION(BlueprintCallable, Category = "Card|Targeting")
	FFCCardTargetInfo ResolveCardTargetUnderCursor(const UFCCardDataAsset* CardAsset = nullptr) const;

	/** Resolves targeting info at a specific 2D viewport screen coordinate */
	UFUNCTION(BlueprintCallable, Category = "Card|Targeting")
	FFCCardTargetInfo ResolveCardTargetAtScreenPosition(const FVector2D& ScreenPos, const UFCCardDataAsset* CardAsset = nullptr) const;

	/** Synchronizes targeting tuning parameters from owning controller defaults */
	void ConfigureTargetingParams(bool bInAutoTarget, float InMaxRange, float InHalfAngleDegrees, float InProximityRadius);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting")
	bool bAutoTargetAttackablesOnProjectileCards = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "100.0"))
	float ProjectileTargetMaxRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "5.0", ClampMax = "89.0"))
	float ProjectileTargetHalfAngleDegrees = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Card|Targeting", meta = (ClampMin = "50.0"))
	float ProjectileTargetProximityRadius = 350.0f;
};
