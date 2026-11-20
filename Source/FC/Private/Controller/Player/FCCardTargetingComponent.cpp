#include "Controller/Player/FCCardTargetingComponent.h"
#include "Card/FCCardExecutionContext.h"
#include "Card/Policies/FCCardTargetPolicyBase.h"
#include "Combat/FCCombatUtils.h"
#include "Data/Card/FCCardDataAsset.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UFCCardTargetingComponent::UFCCardTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFCCardTargetingComponent::ConfigureTargetingParams(bool bInAutoTarget, float InMaxRange, float InHalfAngleDegrees, float InProximityRadius)
{
	bAutoTargetAttackablesOnProjectileCards = bInAutoTarget;
	ProjectileTargetMaxRange = FMath::Max(100.0f, InMaxRange);
	ProjectileTargetHalfAngleDegrees = FMath::Clamp(InHalfAngleDegrees, 5.0f, 89.0f);
	ProjectileTargetProximityRadius = FMath::Max(50.0f, InProximityRadius);
}

FFCCardTargetInfo UFCCardTargetingComponent::ResolveCardTargetUnderCursor(const UFCCardDataAsset* CardAsset) const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return FFCCardTargetInfo();
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (PC->GetMousePosition(MouseX, MouseY))
	{
		return ResolveCardTargetAtScreenPosition(FVector2D(MouseX, MouseY), CardAsset);
	}

	if (const APawn* MyPawn = PC->GetPawn())
	{
		FFCCardTargetInfo FallbackInfo;
		FallbackInfo.TargetLocation = MyPawn->GetActorLocation() + MyPawn->GetActorForwardVector() * 1000.0f;
		return FallbackInfo;
	}

	return FFCCardTargetInfo();
}

FFCCardTargetInfo UFCCardTargetingComponent::ResolveCardTargetAtScreenPosition(const FVector2D& ScreenPos, const UFCCardDataAsset* CardAsset) const
{
	FFCCardTargetInfo TargetInfo;

	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	APawn* MyPawn = PC ? PC->GetPawn() : nullptr;
	if (!PC || !MyPawn)
	{
		return TargetInfo;
	}

	const FVector PawnLocation = MyPawn->GetActorLocation();

	// 1. Deproject Screen Position to 3D World Ray
	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = MyPawn->GetActorForwardVector();

	if (!PC->DeprojectScreenPositionToWorld(ScreenPos.X, ScreenPos.Y, WorldOrigin, WorldDirection))
	{
		if (PC->PlayerCameraManager)
		{
			WorldOrigin = PC->PlayerCameraManager->GetCameraLocation();
			WorldDirection = PC->PlayerCameraManager->GetCameraRotation().Vector();
		}
	}

	// 2. Line trace along WorldDirection to find 3D Impact / Drop Location
	const float TraceDistance = 15000.0f;
	const FVector TraceEnd = WorldOrigin + WorldDirection * TraceDistance;

	FHitResult HitResult;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(CardTargetDeprojectTrace), true);
	TraceParams.AddIgnoredActor(MyPawn);

	const UWorld* World = GetWorld();
	const bool bHit = World && World->LineTraceSingleByChannel(
		HitResult,
		WorldOrigin,
		TraceEnd,
		ECC_Visibility,
		TraceParams
	);

	FVector DropWorldLocation = FVector::ZeroVector;
	AActor* DirectHitActor = nullptr;

	if (bHit && HitResult.bBlockingHit)
	{
		DropWorldLocation = HitResult.ImpactPoint;
		DirectHitActor = HitResult.GetActor();
		TargetInfo.TargetHitComponent = HitResult.GetComponent();
	}
	else
	{
		const FPlane GroundPlane(PawnLocation, FVector::UpVector);
		DropWorldLocation = FMath::LinePlaneIntersection(WorldOrigin, TraceEnd, GroundPlane);
	}

	TargetInfo.TargetLocation = DropWorldLocation;

	// 3. Delegate targeting resolution to the card's modular TInstancedStruct TargetPolicy
	if (CardAsset)
	{
		if (const FFCCardTargetPolicyBase* TargetPolicy = CardAsset->GameplayData.TargetPolicy.GetPtr())
		{
			FFCCardClientTargetQuery Query;
			Query.CasterPawn = MyPawn;
			Query.DropWorldLocation = DropWorldLocation;
			Query.DirectHitActor = DirectHitActor;
			Query.DirectHitComponent = TargetInfo.TargetHitComponent;
			Query.MaxRange = ProjectileTargetMaxRange;
			Query.HalfAngleDegrees = ProjectileTargetHalfAngleDegrees;
			Query.ProximityRadius = ProjectileTargetProximityRadius;
			Query.bEnableAutoTargeting = bAutoTargetAttackablesOnProjectileCards;

			if (TargetPolicy->ResolveClientTarget(Query, CardAsset->GameplayData, TargetInfo))
			{
				return TargetInfo;
			}
		}
	}

	// 4. Generic cursor query fallback when no CardAsset was passed
	if (DirectHitActor && UFCCombatUtils::IsAttackableTarget(MyPawn, DirectHitActor))
	{
		TargetInfo.TargetActor = DirectHitActor;
		TargetInfo.TargetLocation = DirectHitActor->GetActorLocation();
	}

	return TargetInfo;
}
