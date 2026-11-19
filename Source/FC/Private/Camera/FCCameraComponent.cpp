#include "Camera/FCCameraComponent.h"
#include "Camera/FCCameraModeDataAsset.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

UFCCameraComponent::UFCCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// Attempt to locate default camera mode assets in Character/Player/CameraData
	static ConstructorHelpers::FObjectFinder<UFCCameraModeDataAsset> ShoulderFinder(
		TEXT("/Game/Character/Player/CameraData/DA_CameraMode_Shoulder.DA_CameraMode_Shoulder"));
	if (ShoulderFinder.Succeeded())
	{
		DefaultShoulderMode = ShoulderFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UFCCameraModeDataAsset> FirstPersonFinder(
		TEXT("/Game/Character/Player/CameraData/DA_CameraMode_FirstPerson.DA_CameraMode_FirstPerson"));
	if (FirstPersonFinder.Succeeded())
	{
		DefaultFirstPersonMode = FirstPersonFinder.Object;
	}
}

void UFCCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		if (!SpringArm)
		{
			SpringArm = OwnerCharacter->FindComponentByClass<USpringArmComponent>();
		}
		if (!Camera)
		{
			Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
		}
	}

	// Initialize base camera mode
	if (!BaseCameraMode)
	{
		BaseCameraMode = DefaultShoulderMode;
	}

	if (BaseCameraMode)
	{
		SetCameraMode(BaseCameraMode, true);
	}
}

void UFCCameraComponent::InitializeCamera(USpringArmComponent* InSpringArm, UCameraComponent* InCamera)
{
	SpringArm = InSpringArm;
	Camera = InCamera;

	if (BaseCameraMode)
	{
		SetCameraMode(BaseCameraMode, true);
	}
}

void UFCCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsBlending || !SpringArm || !Camera)
	{
		return;
	}

	BlendElapsed += DeltaTime;
	const float Alpha = (BlendDuration > 0.0f) ? FMath::Clamp(BlendElapsed / BlendDuration, 0.0f, 1.0f) : 1.0f;
	const float SmoothedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

	SpringArm->TargetArmLength = FMath::Lerp(StartArmLength, TargetArmLength, SmoothedAlpha);
	SpringArm->SocketOffset = FMath::Lerp(StartSocketOffset, TargetSocketOffset, SmoothedAlpha);
	SpringArm->TargetOffset = FMath::Lerp(StartTargetOffset, TargetTargetOffset, SmoothedAlpha);
	Camera->SetFieldOfView(FMath::Lerp(StartFOV, TargetFOV, SmoothedAlpha));

	if (Alpha >= 1.0f)
	{
		bIsBlending = false;
		CurrentCameraMode = TargetCameraMode;
		if (CurrentCameraMode)
		{
			ApplyDiscreteModeSettings(CurrentCameraMode);
		}
	}
}

void UFCCameraComponent::ToggleCameraMode()
{
	if (IsFirstPerson())
	{
		if (DefaultShoulderMode)
		{
			SetCameraMode(DefaultShoulderMode, false);
		}
	}
	else
	{
		if (DefaultFirstPersonMode)
		{
			SetCameraMode(DefaultFirstPersonMode, false);
		}
	}
}

void UFCCameraComponent::SetCameraMode(UFCCameraModeDataAsset* NewMode, bool bImmediate)
{
	if (!NewMode)
	{
		return;
	}

	BaseCameraMode = NewMode;
	UFCCameraModeDataAsset* EffectiveMode = ResolveEffectiveCameraMode();

	if (EffectiveMode)
	{
		StartBlendToMode(EffectiveMode, bImmediate ? 0.0f : EffectiveMode->BlendTime);
	}
}

void UFCCameraComponent::PushCameraMode(UFCCameraModeDataAsset* ModeAsset, int32 Priority)
{
	if (!ModeAsset)
	{
		return;
	}

	// Remove duplicate if already present
	PopCameraMode(ModeAsset);

	FFCCameraModeStackEntry Entry;
	Entry.ModeAsset = ModeAsset;
	Entry.Priority = Priority;
	ModeStack.Add(Entry);

	// Sort so highest priority is last
	ModeStack.Sort([](const FFCCameraModeStackEntry& A, const FFCCameraModeStackEntry& B) {
		return A.Priority < B.Priority;
	});

	UFCCameraModeDataAsset* EffectiveMode = ResolveEffectiveCameraMode();
	if (EffectiveMode && EffectiveMode != CurrentCameraMode)
	{
		StartBlendToMode(EffectiveMode, EffectiveMode->BlendTime);
	}
}

void UFCCameraComponent::PopCameraMode(UFCCameraModeDataAsset* ModeAsset)
{
	if (!ModeAsset)
	{
		return;
	}

	const int32 InitialCount = ModeStack.Num();
	ModeStack.RemoveAll([ModeAsset](const FFCCameraModeStackEntry& Entry) {
		return Entry.ModeAsset == ModeAsset;
	});

	if (ModeStack.Num() != InitialCount)
	{
		UFCCameraModeDataAsset* EffectiveMode = ResolveEffectiveCameraMode();
		if (EffectiveMode && EffectiveMode != CurrentCameraMode)
		{
			StartBlendToMode(EffectiveMode, EffectiveMode->BlendTime);
		}
	}
}

bool UFCCameraComponent::IsFirstPerson() const
{
	const UFCCameraModeDataAsset* ActiveMode = TargetCameraMode ? TargetCameraMode : CurrentCameraMode;
	if (ActiveMode)
	{
		return ActiveMode == DefaultFirstPersonMode || ActiveMode->TargetArmLength <= 10.0f;
	}
	return false;
}

UFCCameraModeDataAsset* UFCCameraComponent::ResolveEffectiveCameraMode() const
{
	if (ModeStack.Num() > 0)
	{
		return ModeStack.Last().ModeAsset;
	}
	return BaseCameraMode ? BaseCameraMode : DefaultShoulderMode;
}

void UFCCameraComponent::ApplyDiscreteModeSettings(UFCCameraModeDataAsset* ModeAsset)
{
	if (!ModeAsset)
	{
		return;
	}

	if (OwnerCharacter)
	{
		OwnerCharacter->bUseControllerRotationYaw = ModeAsset->bUseControllerRotationYaw;

		if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = ModeAsset->bOrientRotationToMovement;
		}

		// Only toggle local mesh visibility for the locally controlled player
		if (OwnerCharacter->IsLocallyControlled() && OwnerCharacter->GetMesh())
		{
			OwnerCharacter->GetMesh()->SetOwnerNoSee(ModeAsset->bHideCharacterMesh);
			OwnerCharacter->GetMesh()->bCastHiddenShadow = true;
		}
	}

	if (SpringArm)
	{
		SpringArm->bDoCollisionTest = ModeAsset->bDoCollisionTest;
		SpringArm->bEnableCameraLag = ModeAsset->bEnableCameraLag;
		SpringArm->CameraLagSpeed = ModeAsset->CameraLagSpeed;
		SpringArm->bEnableCameraRotationLag = ModeAsset->bEnableCameraRotationLag;
		SpringArm->CameraRotationLagSpeed = ModeAsset->CameraRotationLagSpeed;
	}
}

void UFCCameraComponent::StartBlendToMode(UFCCameraModeDataAsset* TargetMode, float Duration)
{
	if (!TargetMode)
	{
		return;
	}

	TargetCameraMode = TargetMode;

	// Immediately apply rotation and collision settings to maintain responsive gameplay feel
	ApplyDiscreteModeSettings(TargetMode);

	if (Duration <= 0.0f || !SpringArm || !Camera)
	{
		bIsBlending = false;
		CurrentCameraMode = TargetMode;

		if (SpringArm)
		{
			SpringArm->TargetArmLength = TargetMode->TargetArmLength;
			SpringArm->SocketOffset = TargetMode->SocketOffset;
			SpringArm->TargetOffset = TargetMode->TargetOffset;
		}

		if (Camera)
		{
			Camera->SetFieldOfView(TargetMode->FieldOfView);
		}
		return;
	}

	// Capture current states as start values for smooth continuous interruption
	StartArmLength = SpringArm->TargetArmLength;
	StartSocketOffset = SpringArm->SocketOffset;
	StartTargetOffset = SpringArm->TargetOffset;
	StartFOV = Camera->FieldOfView;

	TargetArmLength = TargetMode->TargetArmLength;
	TargetSocketOffset = TargetMode->SocketOffset;
	TargetTargetOffset = TargetMode->TargetOffset;
	TargetFOV = TargetMode->FieldOfView;

	BlendElapsed = 0.0f;
	BlendDuration = Duration;
	bIsBlending = true;
}
