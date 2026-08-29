#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCardDataAsset.generated.h"

/**
 * UFCCardDataAsset
 * 
 * Primary Data Asset holding immutable catalog definitions of a card type,
 * partitioned cleanly into Authoritative Gameplay logic and Presentation Display data.
 */
UCLASS(BlueprintType)
class FC_API UFCCardDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFCCardDataAsset();

	/** Authoritative combat, ability, tag, and cost data */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay")
	FFCCardGameplayData GameplayData;

	/** Client-side visual, audio, and localized text data */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FFCCardDisplayData DisplayData;

	/** Primary Asset ID override for Asset Manager discovery */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Helper to get Card ID */
	UFUNCTION(BlueprintPure, Category = "Card|Data")
	FName GetCardId() const { return GameplayData.CardId; }
};
