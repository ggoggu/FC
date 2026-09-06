#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/Class/FCClassTypes.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCombatUtils.generated.h"

class AActor;
class UFCElementComponent;

/**
 * UFCCombatUtils
 * 
 * Modular combat helper library managing hit traits, elemental stack application,
 * and atomic element consumption for both single-target and AoE spells.
 * Designed to provide extensible hooks for future classes (Warrior, Rogue, Priest).
 */
UCLASS()
class FC_API UFCCombatUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Central dispatcher called when an attack card hits a target (via projectile or direct card play).
	 * Applies class-specific on-hit traits (e.g. Mage adds element stacks to target).
	 */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat")
	static void ApplyAttackCardHitTraits(
		AActor* SourceActor,
		AActor* TargetActor,
		EFCCharacterClass CharacterClass,
		EFCCardType CardType,
		const TArray<EFCElement>& Elements
	);

	/**
	 * Attempts to atomically consume the required element stacks from a single target actor.
	 * Returns true if the target possessed and consumed all required elements.
	 * Returns false if target lacks any required element (no stacks consumed).
	 */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat|Element")
	static bool TryConsumeTargetElements(AActor* TargetActor, const TArray<EFCElement>& RequiredElements);

	/**
	 * Iterates over multiple target actors (e.g. AoE radius overlap, directional sweep)
	 * and atomically consumes required element stacks from each actor that possesses them.
	 * Returns the array of actors whose stacks were successfully consumed.
	 */
	UFUNCTION(BlueprintCallable, Category = "FC|Combat|Element")
	static TArray<AActor*> FilterAndConsumeElements(const TArray<AActor*>& TargetActors, const TArray<EFCElement>& RequiredElements);

	/** Returns how many stacks of a specific element an actor currently holds */
	UFUNCTION(BlueprintPure, Category = "FC|Combat|Element")
	static int32 GetTargetElementCount(AActor* TargetActor, EFCElement Element);

	/** Returns total element stacks across all elements an actor currently holds */
	UFUNCTION(BlueprintPure, Category = "FC|Combat|Element")
	static int32 GetTargetTotalElementStacks(AActor* TargetActor);

	/** Resolves UFCElementComponent on any actor (via direct lookup or CharacterBase) */
	UFUNCTION(BlueprintPure, Category = "FC|Combat|Element")
	static UFCElementComponent* GetElementComponent(AActor* TargetActor);

	/**
	 * Determines whether the candidate actor is a valid, alive, and attackable target
	 * (e.g. Alive FCMobCharacter, unopened FCChestActor, or damageable actor with health > 0).
	 * Excludes self and friendly player characters.
	 */
	UFUNCTION(BlueprintPure, Category = "FC|Combat|Targeting")
	static bool IsAttackableTarget(const AActor* SourceActor, const AActor* TargetCandidate);

	/**
	 * Searches for the best attackable target in the given aim direction or near the aim world location.
	 * Evaluates:
	 * 1. Proximity to AimLocation (within ProximityRadius).
	 * 2. Directional cone from SourceActor towards AimDirection (within HalfAngleDegrees and MaxRange).
	 * 3. Line of sight check against WorldStatic geometry if requested.
	 * Returns nullptr if no attackable object is found.
	 */
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
