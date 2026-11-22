#include "Camera/FCCameraModeDataAsset.h"

UFCCameraModeDataAsset::UFCCameraModeDataAsset()
{
	TargetArmLength = 400.0f;
	SocketOffset = FVector(0.0f, 50.0f, 50.0f);
	TargetOffset = FVector::ZeroVector;
	FieldOfView = 90.0f;
	bDoCollisionTest = true;
	bEnableCameraLag = false;
	CameraLagSpeed = 10.0f;
	bEnableCameraRotationLag = false;
	CameraRotationLagSpeed = 10.0f;
	bUseControllerRotationYaw = false;
	bOrientRotationToMovement = true;
	bHideCharacterMesh = false;
	BlendTime = 0.3f;
}
