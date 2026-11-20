#pragma once

#include "CoreMinimal.h"
#include "Card/Policies/FCCardTargetPolicyBase.h"
#include "FCCardTargetPolicies.generated.h"

/**
 * Targets the caster themselves. No cursor targeting or rotation required.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Target: Self"))
struct FC_API FFCCardTargetPolicy_Self : public FFCCardTargetPolicyBase
{
	GENERATED_BODY()

	virtual bool ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const override;
	virtual bool VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const override;
	virtual bool ShouldRotateCasterOnPlay() const override { return false; }
};

/**
 * Targets a single actor within range with optional server Line-of-Sight verification.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Target: Single Target"))
struct FC_API FFCCardTargetPolicy_SingleTarget : public FFCCardTargetPolicyBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting", meta = (ClampMin = "50.0"))
	float MaxRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting")
	bool bAllowFriendlyTargets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting")
	bool bEnableClientAutoTarget = true;

	virtual bool ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const override;
	virtual bool VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const override;
	virtual bool ShouldRotateCasterOnPlay() const override { return true; }
};

/**
 * Aims in a direction toward cursor/target (ideal for skillshot projectiles and directional walls/cones).
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Target: Directional"))
struct FC_API FFCCardTargetPolicy_Directional : public FFCCardTargetPolicyBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting", meta = (ClampMin = "50.0"))
	float MaxRange = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting")
	bool bAllowFriendlyTargets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting")
	bool bEnableClientAutoTarget = true;

	virtual bool ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const override;
	virtual bool VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const override;
	virtual bool ShouldRotateCasterOnPlay() const override { return true; }
};

/**
 * Authoritatively gathers all attackable enemies within a sphere radius around the caster.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Target: All Enemies In Radius"))
struct FC_API FFCCardTargetPolicy_AllEnemiesInRadius : public FFCCardTargetPolicyBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|Card|Targeting", meta = (ClampMin = "50.0"))
	float Radius = 1000.0f;

	virtual bool ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const override;
	virtual bool VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const override;
	virtual bool ShouldRotateCasterOnPlay() const override { return false; }
};

/**
 * Requires no target actor or world location.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "Target: None"))
struct FC_API FFCCardTargetPolicy_None : public FFCCardTargetPolicyBase
{
	GENERATED_BODY()

	virtual bool ResolveClientTarget(const FFCCardClientTargetQuery& Query, const FFCCardGameplayData& CardData, FFCCardTargetInfo& OutTargetInfo) const override;
	virtual bool VerifyServerTargets(const FFCCardExecutionContext& Context, TArray<TWeakObjectPtr<AActor>>& OutVerifiedTargets, FVector& OutVerifiedAimLocation) const override;
	virtual bool ShouldRotateCasterOnPlay() const override { return false; }
};
