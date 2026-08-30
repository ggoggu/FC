#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "Data/Card/FCCardTypes.h"
#include "FCChestActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UAbilitySystemComponent;
class UFCAttributeSet;
class AFCCardPickupActor;
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

	/** Manually triggers chest destruction and card drop spawning (Server-Only) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Chest")
	void DestroyAndSpawnDrops(AController* InstigatorController = nullptr, AActor* DamageCauser = nullptr);

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

	// --- Gameplay & Drops ---
	UPROPERTY(ReplicatedUsing = OnRep_IsOpened, BlueprintReadOnly, Category = "Chest|State")
	bool bIsOpened = false;

	/** Minimum damage required to break the chest open */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Gameplay", meta = (ClampMin = "0.0"))
	float DamageThreshold = 1.0f;

	/** Cards dropped into the world when chest is broken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chest|Drops")
	TArray<FName> RewardCardIds;

	/** Class of the collectible card drop actor to spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Chest|Drops")
	TSubclassOf<AFCCardPickupActor> CardPickupClass;

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
};
