#include "AI/Tasks/FCBTTask_FindRandomLocation.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/FCAITypes.h"

UFCBTTask_FindRandomLocation::UFCBTTask_FindRandomLocation()
{
	NodeName = TEXT("Find Random Location");

	// Filter allowed blackboard keys to Vector
	OriginKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UFCBTTask_FindRandomLocation, OriginKey));
	TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UFCBTTask_FindRandomLocation, TargetLocationKey));
}

EBTNodeResult::Type UFCBTTask_FindRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Determine search origin
	FVector Origin = ControlledPawn->GetActorLocation();
	if (OriginKey.SelectedKeyName != NAME_None)
	{
		Origin = BlackboardComp->GetValueAsVector(OriginKey.SelectedKeyName);
	}
	else
	{
		const FVector HomeLoc = BlackboardComp->GetValueAsVector(FFCMobBlackboardKeys::HomeLocation);
		if (!HomeLoc.IsNearlyZero())
		{
			Origin = HomeLoc;
		}
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(ControlledPawn->GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation NavLocation;
	if (NavSys->GetRandomReachablePointInRadius(Origin, Radius, NavLocation))
	{
		const FName KeyToSet = TargetLocationKey.SelectedKeyName != NAME_None 
			? TargetLocationKey.SelectedKeyName 
			: FFCMobBlackboardKeys::TargetLocation;

		BlackboardComp->SetValueAsVector(KeyToSet, NavLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

FString UFCBTTask_FindRandomLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Find random navigable point within %.1f radius of %s -> %s"),
		Radius,
		OriginKey.SelectedKeyName != NAME_None ? *OriginKey.SelectedKeyName.ToString() : TEXT("HomeLocation"),
		TargetLocationKey.SelectedKeyName != NAME_None ? *TargetLocationKey.SelectedKeyName.ToString() : TEXT("TargetLocation"));
}
