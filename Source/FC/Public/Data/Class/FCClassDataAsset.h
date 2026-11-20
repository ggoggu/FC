#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassDataAsset.generated.h"

/**
 * UFCClassDataAsset
 *
 * Primary Data Asset wrapper holding immutable class definitions keyed by FGameplayTag (Class.*).
 */
UCLASS(BlueprintType)
class FC_API UFCClassDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFCClassDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	FFCCharacterClassData ClassData;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UFUNCTION(BlueprintPure, Category = "Class|Data")
	FGameplayTag GetClassTag() const { return ClassData.ClassTag; }
};
