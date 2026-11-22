#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCardExecutionContext.generated.h"

class AActor;
class APawn;
class UAbilitySystemComponent;
class UFCCardDataAsset;
class UFCCardDeckComponent;

/**
 * FFCCardClientTargetQuery
 *
 * Lightweight client-side input snapshot passed to FFCCardTargetPolicyBase::ResolveClientTarget.
 * Decouples card targeting policies from PlayerController and UMG/Slate headers.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardClientTargetQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	TWeakObjectPtr<APawn> CasterPawn;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	FVector DropWorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	TWeakObjectPtr<AActor> DirectHitActor;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	TWeakObjectPtr<UPrimitiveComponent> DirectHitComponent;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	float MaxRange = 3000.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	float HalfAngleDegrees = 30.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	float ProximityRadius = 350.0f;

	UPROPERTY(BlueprintReadWrite, Category = "FC|Card|Targeting")
	bool bEnableAutoTargeting = true;
};

/**
 * FFCCardAbilityTargetData
 *
 * Immutable per-invocation GAS target data passed from the card pipeline into GameplayAbilities
 * via FGameplayEventData, eliminating shared mutable target fields on AFCCharacterBase.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardAbilityTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|TargetData")
	FName CardId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|TargetData")
	int32 UpgradeLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|TargetData")
	FVector AimLocation = FVector::ZeroVector;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> VerifiedTargets;

	virtual TArray<TWeakObjectPtr<AActor>> GetActors() const override
	{
		return VerifiedTargets;
	}

	virtual bool HasOrigin() const override
	{
		return !AimLocation.IsZero();
	}

	virtual FTransform GetOrigin() const override
	{
		return FTransform(AimLocation);
	}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	static const FFCCardAbilityTargetData* ExtractFromHandle(const FGameplayAbilityTargetDataHandle& TargetData)
	{
		if (TargetData.Num() <= 0)
		{
			return nullptr;
		}
		const FGameplayAbilityTargetData* RawData = TargetData.Get(0);
		if (!RawData || RawData->GetScriptStruct() != StaticStruct())
		{
			return nullptr;
		}
		return static_cast<const FFCCardAbilityTargetData*>(RawData);
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;
		Ar << CardId;
		Ar << UpgradeLevel;

		bool bAimSuccess = true;
		AimLocation.NetSerialize(Ar, Map, bAimSuccess);
		bOutSuccess &= bAimSuccess;

		SafeNetSerializeTArray_Default<31>(Ar, VerifiedTargets);
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FFCCardAbilityTargetData> : public TStructOpsTypeTraitsBase2<FFCCardAbilityTargetData>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};

/**
 * FFCCardExecutionContext
 *
 * Server-authoritative, per-card-play execution payload flowing sequentially through
 * TargetPolicy -> Conditions -> PlayEffects -> KeywordRules.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardExecutionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	TWeakObjectPtr<AActor> CasterActor;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	TWeakObjectPtr<UAbilitySystemComponent> CasterASC;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	TWeakObjectPtr<UFCCardDeckComponent> DeckComponent;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	TWeakObjectPtr<UFCCardDataAsset> CardDataAsset;

	/** Non-owning pointer to immutable cached row gameplay data for zero-copy execution */
	const FFCCardGameplayData* CardData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	FGuid CardGuid;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	FName CardId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	int32 UpgradeLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	FFCCardTargetInfo RawTargetInfo;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> VerifiedTargets;

	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	FVector VerifiedAimLocation = FVector::ZeroVector;

	/** Set false when a soft condition (e.g. element consumption) fails after mana was spent */
	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	bool bConditionalEffectsAllowed = true;

	/** Tracks whether at least one effect fragment executed successfully */
	UPROPERTY(BlueprintReadOnly, Category = "FC|Card|Execution")
	bool bExecutedAnyEffect = false;

	AActor* GetPrimaryTarget() const
	{
		return VerifiedTargets.Num() > 0 ? VerifiedTargets[0].Get() : nullptr;
	}

	FGameplayAbilityTargetDataHandle MakeAbilityTargetDataHandle() const
	{
		FGameplayAbilityTargetDataHandle Handle;
		FFCCardAbilityTargetData* Data = new FFCCardAbilityTargetData();
		Data->CardId = CardId;
		Data->UpgradeLevel = UpgradeLevel;
		Data->AimLocation = VerifiedAimLocation;
		Data->VerifiedTargets = VerifiedTargets;
		Handle.Add(Data);
		return Handle;
	}
};
