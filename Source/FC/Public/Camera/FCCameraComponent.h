#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FCCameraComponent.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ACharacter;
class UFCCameraModeDataAsset;

/**
 * FFCCameraModeStackEntry
 * 
 * Represents an active camera mode in the priority stack (e.g. Base Shoulder View -> ADS -> Cutscene).
 */
USTRUCT(BlueprintType)
struct FFCCameraModeStackEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UFCCameraModeDataAsset> ModeAsset = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	int32 Priority = 0;

	bool operator<(const FFCCameraModeStackEntry& Other) const
	{
		return Priority < Other.Priority;
	}
};

/**
 * UFCCameraComponent
 * 
 * Manages player perspective modes (1st Person, Shoulder View, ADS, Cutscene)
 * via UFCCameraModeDataAsset with smooth runtime blending, priority stacking,
 * and character controller/mesh visibility synchronization.
 */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class FC_API UFCCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFCCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Binds this camera manager to the target spring arm and camera components */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void InitializeCamera(USpringArmComponent* InSpringArm, UCameraComponent* InCamera);

	/** Switches between the default shoulder mode and first-person mode */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ToggleCameraMode();

	/** Sets the base active camera mode, starting a smooth transition if BlendTime > 0 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraMode(UFCCameraModeDataAsset* NewMode, bool bImmediate = false);

	/** Pushes a temporary overlay camera mode onto the priority stack (e.g., ADS, knockdown, cutscene) */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void PushCameraMode(UFCCameraModeDataAsset* ModeAsset, int32 Priority = 1);

	/** Pops an overlay camera mode from the priority stack */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void PopCameraMode(UFCCameraModeDataAsset* ModeAsset);

	/** Returns true if the currently active camera mode represents first person */
	UFUNCTION(BlueprintPure, Category = "Camera")
	bool IsFirstPerson() const;

	/** Returns currently active camera mode data asset */
	UFUNCTION(BlueprintPure, Category = "Camera")
	UFCCameraModeDataAsset* GetCurrentCameraMode() const { return CurrentCameraMode; }

	/** Default shoulder view mode asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Defaults")
	TObjectPtr<UFCCameraModeDataAsset> DefaultShoulderMode;

	/** Default first-person view mode asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera|Defaults")
	TObjectPtr<UFCCameraModeDataAsset> DefaultFirstPersonMode;

protected:
	virtual void BeginPlay() override;

	/** Resolves the highest priority camera mode currently in stack or base */
	UFCCameraModeDataAsset* ResolveEffectiveCameraMode() const;

	/** Updates character rotation orientation, mesh visibility, and spring arm collision */
	void ApplyDiscreteModeSettings(UFCCameraModeDataAsset* ModeAsset);

	/** Begins blending from current camera values to target mode values */
	void StartBlendToMode(UFCCameraModeDataAsset* TargetMode, float Duration);

private:
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UFCCameraModeDataAsset> BaseCameraMode;

	UPROPERTY(Transient)
	TObjectPtr<UFCCameraModeDataAsset> CurrentCameraMode;

	UPROPERTY(Transient)
	TObjectPtr<UFCCameraModeDataAsset> TargetCameraMode;

	UPROPERTY(Transient)
	TArray<FFCCameraModeStackEntry> ModeStack;

	// Blending runtime state
	bool bIsBlending = false;
	float BlendElapsed = 0.0f;
	float BlendDuration = 0.3f;

	float StartArmLength = 400.0f;
	FVector StartSocketOffset = FVector::ZeroVector;
	FVector StartTargetOffset = FVector::ZeroVector;
	float StartFOV = 90.0f;

	float TargetArmLength = 400.0f;
	FVector TargetSocketOffset = FVector::ZeroVector;
	FVector TargetTargetOffset = FVector::ZeroVector;
	float TargetFOV = 90.0f;
};
