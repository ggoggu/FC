#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FCCameraModeDataAsset.generated.h"

/**
 * UFCCameraModeDataAsset
 * 
 * Defines physical, behavioral, and blending properties for a specific camera perspective
 * (e.g., Shoulder View, First Person, Aim Down Sights (ADS), Cutscene/Dialogue).
 */
UCLASS(BlueprintType)
class FC_API UFCCameraModeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFCCameraModeDataAsset();

	/** Tag identifying this camera mode for gameplay queries and state tracking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Identification")
	FGameplayTag CameraModeTag;

	/** Length of the camera boom / spring arm (0.0 for First Person, ~400.0 for 3rd Person) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|View", meta = (ClampMin = "0.0"))
	float TargetArmLength = 400.0f;

	/** Socket offset applied at the end of the spring arm (e.g. right-shoulder offset or eye level) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|View")
	FVector SocketOffset = FVector(0.0f, 50.0f, 50.0f);

	/** Target offset applied at the spring arm root attachment */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|View")
	FVector TargetOffset = FVector::ZeroVector;

	/** Camera field of view in degrees */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|View", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float FieldOfView = 90.0f;

	/** Whether to test for collision between spring arm and world geometry (disabled in FP to prevent capsule clipping) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Collision")
	bool bDoCollisionTest = true;

	/** Whether camera position lag is enabled for smooth trailing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lag")
	bool bEnableCameraLag = false;

	/** Speed of camera position lag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lag", meta = (EditCondition = "bEnableCameraLag", ClampMin = "0.1"))
	float CameraLagSpeed = 10.0f;

	/** Whether camera rotation lag is enabled */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lag")
	bool bEnableCameraRotationLag = false;

	/** Speed of camera rotation lag */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Lag", meta = (EditCondition = "bEnableCameraRotationLag", ClampMin = "0.1"))
	float CameraRotationLagSpeed = 10.0f;

	/** If true, character rotates with controller Yaw (required for 1st person aiming/moving) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Control")
	bool bUseControllerRotationYaw = false;

	/** If true, character smoothly orients toward movement direction (standard for 3rd person exploration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Control")
	bool bOrientRotationToMovement = true;

	/** If true, hides the local owner's skeletal mesh to prevent head clipping in first person */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Mesh")
	bool bHideCharacterMesh = false;

	/** Time in seconds taken to blend smoothly into this camera mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera|Transition", meta = (ClampMin = "0.0"))
	float BlendTime = 0.3f;
};
