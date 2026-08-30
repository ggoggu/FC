#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCardPickupActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class URotatingMovementComponent;
class UFCCardRewardWidget;
class USoundBase;
class UNiagaraSystem;

/**
 * AFCCardPickupActor
 * 
 * Replicated world pickup actor dropped from chests or enemies.
 * Floats and rotates in place. When a local player pawn steps inside the overlap radius,
 * triggers the MVVM Card Reward selection UI widget.
 */
UCLASS()
class FC_API AFCCardPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AFCCardPickupActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Configures the dropped card ID (Authoritative) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card|Pickup")
	void SetDropCardId(FName InCardId);

	UFUNCTION(BlueprintPure, Category = "Card|Pickup")
	FName GetDropCardId() const { return DropCardId; }

	/** Server RPC to claim this card pickup into player's deck */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Card|Pickup")
	void Server_ClaimPickup(APawn* ClaimerPawn, EFCCardAddDestination Destination = EFCCardAddDestination::DiscardPile);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Pickup", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Pickup", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Pickup", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

	/** Replicated card definition ID contained in this pickup */
	UPROPERTY(ReplicatedUsing = OnRep_DropCardId, BlueprintReadOnly, Category = "Card|Pickup")
	FName DropCardId = NAME_None;

	/** Replicated state indicating whether the card has already been claimed */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Card|Pickup")
	bool bIsClaimed = false;

	/** Optional custom Widget class for reward popup */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|UI")
	TSubclassOf<UFCCardRewardWidget> RewardWidgetClass;

	/** Default destination when added to deck */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|Pickup")
	EFCCardAddDestination DefaultDestination = EFCCardAddDestination::DiscardPile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Cosmetics")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Cosmetics")
	TObjectPtr<UNiagaraSystem> PickupVFX;

	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnRep_DropCardId();

	/** Spawns and displays the Card Reward Widget for the local player */
	void ShowRewardWidgetForPlayer(APawn* PlayerPawn);
};
