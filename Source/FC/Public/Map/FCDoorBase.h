#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FCDoorBase.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFCDoorLockChanged, bool, bIsLocked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFCDoorOpenChanged, bool, bIsOpen);

UCLASS()
class FC_API AFCDoorBase : public AActor
{
	GENERATED_BODY()

public:
	AFCDoorBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-side authoritative state mutation
	UFUNCTION(BlueprintCallable, Category = "FC|Door")
	void SetDoorLocked(bool bNewLocked);

	UFUNCTION(BlueprintCallable, Category = "FC|Door")
	void SetDoorOpen(bool bNewOpen);

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	bool IsDoorLocked() const { return bIsLocked; }

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	bool IsDoorOpen() const { return bIsOpen; }

public:
	UPROPERTY(BlueprintAssignable, Category = "FC|Door|Events")
	FOnFCDoorLockChanged OnDoorLockChanged;

	UPROPERTY(BlueprintAssignable, Category = "FC|Door|Events")
	FOnFCDoorOpenChanged OnDoorOpenChanged;

protected:
	virtual void BeginPlay() override;

	// Cosmetic Blueprint overrides for door opening/closing animation & sound
	UFUNCTION(BlueprintNativeEvent, Category = "FC|Door|Cosmetic")
	void OnPlayLockEffect(bool bLocked);

	UFUNCTION(BlueprintNativeEvent, Category = "FC|Door|Cosmetic")
	void OnPlayOpenEffect(bool bOpen);

	UFUNCTION()
	virtual void OnRep_IsLocked();

	UFUNCTION()
	virtual void OnRep_IsOpen();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> DoorFrameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UBoxComponent> InteractionTrigger;

	// Sound cues for door feedback
	UPROPERTY(EditDefaultsOnly, Category = "FC|Door|Audio")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Door|Audio")
	TObjectPtr<USoundBase> LockSound;

	UPROPERTY(EditDefaultsOnly, Category = "FC|Door|Audio")
	TObjectPtr<USoundBase> UnlockSound;

private:
	UPROPERTY(ReplicatedUsing = OnRep_IsLocked, BlueprintReadOnly, Category = "FC|Door", meta = (AllowPrivateAccess = "true"))
	bool bIsLocked = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "FC|Door", meta = (AllowPrivateAccess = "true"))
	bool bIsOpen = true;
};
