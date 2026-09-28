#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Map/FCBombDamageable.h"
#include "FCDestructibleWall.generated.h"

class UStaticMeshComponent;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFCWallDestroyed, class AFCDestructibleWall*, Wall);

UCLASS()
class FC_API AFCDestructibleWall : public AActor, public IFCBombDamageable
{
	GENERATED_BODY()

public:
	AFCDestructibleWall();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IFCBombDamageable Interface Implementation (Server authoritative)
	virtual bool ReceiveBombDamage_Implementation(float DamageAmount, const FVector& HitLocation, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "FC|Wall")
	void SetLinkedSecretRoomCoord(const FIntPoint& InCoord) { LinkedSecretRoomCoord = InCoord; }

	UFUNCTION(BlueprintPure, Category = "FC|Wall")
	const FIntPoint& GetLinkedSecretRoomCoord() const { return LinkedSecretRoomCoord; }

	UFUNCTION(BlueprintPure, Category = "FC|Wall")
	bool IsWallDestroyed() const { return bIsDestroyed; }

public:
	UPROPERTY(BlueprintAssignable, Category = "FC|Wall|Events")
	FOnFCWallDestroyed OnWallDestroyed;

protected:
	virtual void BeginPlay() override;

	// Cosmetic Blueprint override for Chaos physics destruction / particle bursts
	UFUNCTION(BlueprintNativeEvent, Category = "FC|Wall|Cosmetic")
	void OnPlayDestructionCosmetics();

	UFUNCTION()
	virtual void OnRep_IsDestroyed();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> IntactWallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Wall")
	FIntPoint LinkedSecretRoomCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Wall|Audio")
	TObjectPtr<USoundBase> DestructionSound;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Wall|VFX")
	TObjectPtr<UNiagaraSystem> DestructionVFX;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsDestroyed, BlueprintReadOnly, Category = "FC|Wall", meta = (AllowPrivateAccess = "true"))
	bool bIsDestroyed = false;
};
