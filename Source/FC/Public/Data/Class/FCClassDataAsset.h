#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassDataAsset.generated.h"

/**
 * UFCClassDataAsset
 * 
 * Primary Data Asset holding immutable definitions of a playable character class,
 * including base stats, affinity elements, starting deck, and tags.
 */
UCLASS(BlueprintType)
class FC_API UFCClassDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFCClassDataAsset();

	/** Authoritative class configuration and stats */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	FFCCharacterClassData ClassData;

	/** Primary Asset ID override for Asset Manager discovery */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Helper to get Class Type */
	UFUNCTION(BlueprintPure, Category = "Class|Data")
	EFCCharacterClass GetClassType() const { return ClassData.ClassType; }
};
