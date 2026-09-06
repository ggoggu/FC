#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "FCBTTask_Attack.generated.h"

class UAnimMontage;

struct FFCAttackTaskMemory
{
	float RemainingDuration = 0.0f;
	float RemainingCastDelay = 0.0f;
	bool bAbilityTriggered = false;
	TWeakObjectPtr<AActor> TargetActor;
};

/**
 * Behavior Tree task that orients toward the target actor, plays an attack montage,
 * and activates the mob's configured attack Gameplay Ability (e.g. UFCGA_Fireball).
 */
UCLASS()
class FC_API UFCBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UFCBTTask_Attack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

	float GetCastDelay() const { return CastDelay; }

protected:
	/** Blackboard key selector for target actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	FBlackboardKeySelector TargetActorKey;

	/** Optional attack montage override. If null, uses AFCMobCharacter::GetAttackMontage() */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	TObjectPtr<UAnimMontage> MontageOverride;

	/** Delay in seconds after attack begins before projectile ability is fired (syncs with animation windup) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "0.0"))
	float CastDelay = 0.25f;

	/** Fallback attack duration in seconds if no montage is playing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "0.1"))
	float AttackDuration = 0.8f;

	/** If true, adjusts mob yaw rotation to face the target immediately upon execution */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	bool bFaceTargetOnExecute = true;

	/** If true, continuously tracks and faces the target during the attack windup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI")
	bool bTrackTargetDuringAttack = true;

	/** Play rate for attack animation montage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|AI", meta = (ClampMin = "0.1"))
	float PlayRate = 1.0f;
};
