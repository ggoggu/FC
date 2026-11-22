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

	/** Source-tracked lock mutation for doors shared between two rooms. Door unlocks only when all lock sources release it. */
	UFUNCTION(BlueprintCallable, Category = "FC|Door")
	void SetDoorLockedBySource(AActor* LockSource, bool bLock);

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	int32 GetActiveLockCount() const;

	UFUNCTION(BlueprintCallable, Category = "FC|Door")
	void SetDoorOpen(bool bNewOpen);

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	bool IsDoorLocked() const { return bIsLocked; }

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	bool IsDoorOpen() const { return bIsOpen; }

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	float GetDoorWidth() const { return DoorWidth; }

	UFUNCTION(BlueprintCallable, Category = "FC|Door")
	void SetDoorWidth(float NewDoorWidth);

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	float GetPassageDepth() const { return PassageDepth; }

	UFUNCTION(BlueprintCallable, Category = "FC|Door")
	void SetPassageDepth(float NewPassageDepth);

	UFUNCTION(BlueprintPure, Category = "FC|Door")
	float GetDoorHeight() const { return DoorHeight; }

	UFUNCTION(BlueprintCallable, Category = "FC|Door")
	virtual void UpdateDoorDimensions();

	UStaticMeshComponent* GetDoorMeshComponent() const { return DoorMesh; }
	UStaticMeshComponent* GetPassageFloorMeshComponent() const { return PassageFloorMesh; }
	UBoxComponent* GetInteractionTrigger() const { return InteractionTrigger; }

public:
	UPROPERTY(BlueprintAssignable, Category = "FC|Door|Events")
	FOnFCDoorLockChanged OnDoorLockChanged;

	UPROPERTY(BlueprintAssignable, Category = "FC|Door|Events")
	FOnFCDoorOpenChanged OnDoorOpenChanged;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
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

	UFUNCTION()
	virtual void OnRep_DoorWidth();

	/** Lateral opening width of the door passage (local Y axis) in cm. Defaults to 300 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Door|Config", ReplicatedUsing = OnRep_DoorWidth, BlueprintSetter = SetDoorWidth, meta = (ClampMin = "50.0"))
	float DoorWidth = 300.0f;

	/** Longitudinal passage depth bridging adjacent room boundaries (local X axis) in cm. Defaults to 300 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Door|Config", ReplicatedUsing = OnRep_DoorWidth, BlueprintSetter = SetPassageDepth, meta = (ClampMin = "50.0"))
	float PassageDepth = 300.0f;

	/** Vertical clearance height of the doorway opening above FloorThickness in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FC|Door|Config", meta = (ClampMin = "100.0"))
	float DoorHeight = 500.0f;

	/** Height of the passage bridge floor slab in cm (Z in [0, FloorThickness]), flush with adjacent room floors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FC|Door|Config", meta = (ClampMin = "10.0"))
	float FloorThickness = 100.0f;

	/** Longitudinal thickness of the blocking DoorMesh gate (local X axis) in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FC|Door|Config", meta = (ClampMin = "10.0"))
	float DoorGateThickness = 100.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> DoorFrameMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Flush walkway floor slab spanning the exact gap between adjacent rooms. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> PassageFloorMesh;

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
	UPROPERTY(BlueprintReadOnly, Category = "FC|Door", ReplicatedUsing = OnRep_IsLocked, meta = (AllowPrivateAccess = "true"))
	bool bIsLocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Door", ReplicatedUsing = OnRep_IsOpen, meta = (AllowPrivateAccess = "true"))
	bool bIsOpen = true;

	/** Server-authoritative set of actors (rooms) currently holding a combat lock on this shared door. */
	TSet<TWeakObjectPtr<AActor>> ActiveLockSources;
};
