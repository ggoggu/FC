#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/FCAITypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "FCMobAIController.generated.h"

class UBehaviorTree;
class UBlackboardData;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class AFCMobCharacter;

/**
 * AI Controller for Humanoid Mobs with sight perception and behavior tree execution
 */
UCLASS()
class FC_API AFCMobAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFCMobAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Checks whether a perceived actor is a valid player target */
	virtual bool IsTargetViablePlayer(AActor* InActor) const;

	UAIPerceptionComponent* GetPerceptionComp() const { return MobPerceptionComponent; }
	UAISenseConfig_Sight* GetSightConfig() const { return SightConfig; }

	UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }
	void SetBehaviorTreeAsset(UBehaviorTree* InBT) { BehaviorTreeAsset = InBT; }

	UBlackboardData* GetBlackboardAsset() const { return BlackboardAsset; }
	void SetBlackboardAsset(UBlackboardData* InBB) { BlackboardAsset = InBB; }

protected:
	virtual void BeginPlay() override;

	/** Callback when AI perception senses or loses sight of an actor */
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** Behavior Tree asset executed on possess */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/** Optional Blackboard asset override (if null, uses BehaviorTreeAsset->BlackboardAsset) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|AI")
	TObjectPtr<UBlackboardData> BlackboardAsset;

	/** AI Perception component for vision */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|AI|Perception", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> MobPerceptionComponent;

	/** Sight configuration */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|AI|Perception", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Sight Radius in cm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|AI|Perception", meta = (ClampMin = "100.0"))
	float SightRadius = 1500.0f;

	/** Lose Sight Radius in cm */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|AI|Perception", meta = (ClampMin = "100.0"))
	float LoseSightRadius = 2000.0f;

	/** Peripheral vision half angle in degrees */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FC|AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 60.0f;
};
