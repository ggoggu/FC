#pragma once

#include "CoreMinimal.h"
#include "FCCardTargetPolicyBase.generated.h"

class AActor;
struct FFCCardClientTargetQuery;
struct FFCCardExecutionContext;
struct FFCCardGameplayData;
struct FFCCardTargetInfo;

/**
 * FFCCardTargetPolicyBase
 *
 * Stateless TInstancedStruct strategy contract for resolving client targeting intent
 * and authoritatively verifying targets on the server.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardTargetPolicyBase
{
	GENERATED_BODY()

	virtual ~FFCCardTargetPolicyBase() = default;

	/** Resolves client-side target selection from cursor/aim query */
	virtual bool ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const
	{
		return false;
	}

	/** Authoritatively verifies and populates targets and aim location on the server */
	virtual bool VerifyServerTargets(
		const FFCCardExecutionContext& Context,
		TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets,
		FVector& OutVerifiedAimLocation) const
	{
		return true;
	}

	/** Whether caster pawn should face the resolved aim/target on card play */
	virtual bool ShouldRotateCasterOnPlay() const
	{
		return false;
	}
};
