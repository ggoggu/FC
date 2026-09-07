#pragma once

#include "CoreMinimal.h"
#include "FCAITypes.generated.h"

/**
 * High-level AI state for Mob behavior
 */
UENUM(BlueprintType)
enum class EFCMobAIState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Patrol UMETA(DisplayName = "Patrol"),
	Investigating UMETA(DisplayName = "Investigating"),
	Chasing UMETA(DisplayName = "Chasing"),
	Attacking UMETA(DisplayName = "Attacking")
};

/**
 * Standard Blackboard key name definitions for FC Mob AI
 */
struct FC_API FFCMobBlackboardKeys
{
	inline static const FName TargetActor = FName(TEXT("TargetActor"));
	inline static const FName TargetLocation = FName(TEXT("TargetLocation"));
	inline static const FName LastKnownLocation = FName(TEXT("LastKnownLocation"));
	inline static const FName HomeLocation = FName(TEXT("HomeLocation"));
	inline static const FName AIState = FName(TEXT("AIState"));
};
