#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/FCBombDamageable.h"
#include "Combat/Interfaces/FCCombatTargetInterface.h"
#include "Data/FCGameplayTags.h"
#include "FCDestructibleWall.generated.h"

class UStaticMeshComponent;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFCWallDestroyed, class AFCDestructibleWall*, Wall);

UCLASS()
class FC_API AFCDestructibleWall : public AActor, public IFCBombDamageable, public IFCCombatTargetInterface
{
	GENERATED_BODY()

public:
	AFCDestructibleWall();

	// --- IFCCombatTargetInterface ---
	virtual FGameplayTag GetCombatTeamTag() const override { return FCGameplayTags::Combat_Target_Destructible; }
	virtual bool IsCombatTargetAlive() const override { return !bIsDestroyed; }
	virtual bool CanBeTargetedBy(const AActor* SourceActor) const override { return !bIsDestroyed && SourceActor != this; }
	virtual FVector GetCombatAimLocation() const override { return GetActorLocation(); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IFCBombDamageable Interface Implementation (Server authoritative)
	virtual bool ReceiveBombDamage_Implementation(float DamageAmount, const FVector& HitLocation, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "FC|Wall")
	void SetLinkedSecretRoomCoord(const FIntPoint& InCoord) { LinkedSecretRoomCoord = InCoord; }

	UFUNCTION(BlueprintPure, Category = "FC|Wall")
	const FIntPoint& GetLinkedSecretRoomCoord() const { return LinkedSecretRoomCoord; }

	UFUNCTION(BlueprintPure, Category = "FC|Wall")
	bool IsWallDestroyed() const { return bIsDestroyed; }

	/** Sets exact passage wall dimensions (X=GapDepth, Y=SpanLength, Z=TotalHeight in cm) and floor slab height. Server authoritative. */
	UFUNCTION(BlueprintCallable, Category = "FC|Wall")
	void SetWallDimensions(const FVector& InDimensions, float InFloorThickness = 100.0f);

	UFUNCTION(BlueprintPure, Category = "FC|Wall")
	FVector GetWallDimensions() const { return WallDimensions; }

	UFUNCTION(BlueprintCallable, Category = "FC|Wall")
	void ApplyWallDimensions();

	UStaticMeshComponent* GetIntactWallMeshComponent() const { return IntactWallMesh; }
	UStaticMeshComponent* GetPassageFloorMeshComponent() const { return PassageFloorMesh; }

public:
	UPROPERTY(BlueprintAssignable, Category = "FC|Wall|Events")
	FOnFCWallDestroyed OnWallDestroyed;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	// Cosmetic Blueprint override for Chaos physics destruction / particle bursts
	UFUNCTION(BlueprintNativeEvent, Category = "FC|Wall|Cosmetic")
	void OnPlayDestructionCosmetics();

	UFUNCTION()
	virtual void OnRep_IsDestroyed();

	UFUNCTION()
	virtual void OnRep_WallDimensions();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> IntactWallMesh;

	/** Permanent passage bridge floor underneath the destructible wall so players can walk into the secret room after destruction. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> PassageFloorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Wall")
	FIntPoint LinkedSecretRoomCoord = FIntPoint::ZeroValue;

	/** Desired wall dimensions in cm (X=GapDepth, Y=SpanLength, Z=TotalHeight). Zero preserves Blueprint default scale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FC|Wall", ReplicatedUsing = OnRep_WallDimensions)
	FVector WallDimensions = FVector::ZeroVector;

	/** Height of the permanent PassageFloorMesh slab in cm (Z in [0, FloorThickness]); IntactWallMesh sits above it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FC|Wall", ReplicatedUsing = OnRep_WallDimensions, meta = (ClampMin = "0.0"))
	float FloorThickness = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Wall|Audio")
	TObjectPtr<USoundBase> DestructionSound;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Wall|VFX")
	TObjectPtr<UNiagaraSystem> DestructionVFX;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsDestroyed, BlueprintReadOnly, Category = "FC|Wall", meta = (AllowPrivateAccess = "true"))
	bool bIsDestroyed = false;
};
