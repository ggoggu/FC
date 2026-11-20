#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Data/Class/FCClassTypes.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCombatUtils.generated.h"

class AActor;
class UFCElementComponent;

/**
 * UFCCombatUtils
 *
 * Modular combat helper library dispatching class on-hit traits polymorphically
 * via ClassData->TraitBehavior (TInstancedStruct) and targetability via IFCCombatTargetInterface.
 */
UCLASS()
class FC_API UFCCombatUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Polymorphic central dispatcher called when an attack hits a target.
	 * Delegates to the class's TInstancedStruct TraitBehavior (OCP: zero class switches).
	 */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	static void ApplyAttackCardHitTraits(
		AActor* SourceActor,
		AActor* TargetActor,
		FGameplayTag ClassTag,
		FGameplayTag CardTypeTag,
		const TArray<FGameplayTag>& ElementTags
	);

	UFUNCTION(BlueprintCallable, Category = "FC|Combat|Element")
	static bool TryConsumeTargetElements(AActor* TargetActor, const TArray<FGameplayTag>& RequiredElementTags);

	UFUNCTION(BlueprintCallable, Category = "FC|Combat|Element")
	static TArray<AActor*> FilterAndConsumeElements(const TArray<AActor*>& TargetActors, const TArray<FGameplayTag>& RequiredElementTags);

	UFUNCTION(BlueprintPure, Category = "FC|Combat|Element")
	static int32 GetTargetElementCount(AActor* TargetActor, FGameplayTag ElementTag);

	UFUNCTION(BlueprintPure, Category = "FC|Combat|Element")
	static int32 GetTargetTotalElementStacks(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "FC|Combat|Element")
	static UFCElementComponent* GetElementComponent(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "FC|Combat|Targeting")
	static bool IsAttackableTarget(const AActor* SourceActor, const AActor* TargetCandidate);

	UFUNCTION(BlueprintCallable, Category = "FC|Combat|Targeting")
	static AActor* FindBestAttackableTargetInDirection(
		const AActor* SourceActor,
		const FVector& AimDirection,
		const FVector& AimLocation,
		float MaxRange = 3000.0f,
		float HalfAngleDegrees = 30.0f,
		float ProximityRadius = 350.0f,
		bool bCheckLineOfSight = true
	);
};
