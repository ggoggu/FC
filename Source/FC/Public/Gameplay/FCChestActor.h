#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "Data/Card/FCCardTypes.h"
#include "Data/Class/FCClassTypes.h"
#include "FCChestActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UAbilitySystemComponent;
class UFCAttributeSet;
class AFCCardPickupActor;
class UFCCardSubsystem;
class USoundBase;
class UNiagaraSystem;

/**
 * AFCChestActor
 * 
 * Server-authoritative Chest actor that shatters and opens upon taking >= 1 damage
 * (from Fireball projectiles, melee, radial AoE, or GAS GameplayEffects).
 * Spawns collectible AFCCardPickupActor drops in the world.
 */
UCLASS()
class FC_API AFCChestActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AFCChestActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Chest")
	bool IsOpened() const { return bIsOpened; }

	UFUNCTION(BlueprintPure, Category = "Chest")
	float GetDamageThreshold() const { return DamageThreshold; }

	UFUNCTION(BlueprintPure, Category = "Chest")
	float GetDestroyDelay() const { return DestroyDelay; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Chest")
	void SetDestroyDelay(float InDelay) { DestroyDelay = FMath::Max(0.0f, InDelay); }

	/** Manually triggers chest destruction and card drop spawning (Server-Only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Chest")
	void DestroyAndSpawnDrops(AController* InstigatorController = nullptr, AActor* DamageCauser = nullptr);

	// --- Drops & Class Filtering Settings ---
	/** Cards dropped into the world when chest is broken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Drops")
	TArray<FName> RewardCardIds;

	/** Whether cards matching the opener's class are allowed to drop (Default: true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Drops")
	bool bAllowOpenerClass = true;

	/** Whether Neutral cards are allowed in drops (Default: false, Class cards only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Drops")
	bool bAllowNeutralCards = false;

	/** Specific other classes individually allowed to drop (e.g. Warrior, Rogue) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Drops")
	TArray<EFCCharacterClass> AllowedOtherClasses;

	/** If true, all other classes are allowed regardless of AllowedOtherClasses (Default: false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Drops")
	bool bAllowAllOtherClasses = false;

	/** Fallback class used if no player instigator can be resolved */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Drops")
	EFCCharacterClass FallbackClass = EFCCharacterClass::Mage;

	/** Class of the collectible card drop actor to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest|Drops")
	TSubclassOf<AFCCardPickupActor> CardPickupClass;

	/** Resolves the character class of the actor/controller that opened/damaged this chest */
	UFUNCTION(BlueprintPure, Category = "Chest|Drops")
	EFCCharacterClass GetOpenerClass(AController* InstigatorController, AActor* DamageCauser) const;

	/** Checks whether a specific card is allowed to drop based on opener class and filter settings */
	UFUNCTION(BlueprintPure, Category = "Chest|Drops")
	bool IsCardAllowedToDrop(FName CardId, EFCCharacterClass OpenerClass, const UFCCardSubsystem* InCardSubsystem = nullptr) const;

	/** Returns filtered reward cards matching opener class and drop filter settings */
	UFUNCTION(BlueprintPure, Category = "Chest|Drops")
	TArray<FName> GetFilteredRewardCardIds(EFCCharacterClass OpenerClass, const UFCCardSubsystem* InCardSubsystem = nullptr) const;

protected:
	virtual void BeginPlay() override;

	// --- Visual & Collision Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LidMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionBox;

	// --- Ability System ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest|Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chest|Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCAttributeSet> AttributeSet;

	// --- Gameplay & State ---
	UPROPERTY(ReplicatedUsing = OnRep_IsOpened, BlueprintReadOnly, Category = "Chest|State")
	bool bIsOpened = false;

	/** Minimum damage required to break the chest open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Gameplay", meta = (ClampMin = "0.0"))
	float DamageThreshold = 1.0f;

	/** Delay in seconds before the destroyed chest actor is completely removed from the world (0 = immediate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Gameplay", meta = (ClampMin = "0.0"))
	float DestroyDelay = 0.1f;

	// --- Presentation ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest|Presentation")
	TObjectPtr<USoundBase> DestroySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest|Presentation")
	TObjectPtr<UNiagaraSystem> DestroyVFX;

	UFUNCTION()
	virtual void OnRep_IsOpened();

	/** Blueprint hook for cosmetic destruction animations and VFX */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chest|Events")
	void OnChestBrokenCosmetics();

	/** Hides visual components and disables all collisions when chest breaks */
	void HideAndDisableChest();
};
