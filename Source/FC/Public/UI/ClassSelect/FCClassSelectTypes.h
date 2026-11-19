#pragma once

#include "CoreMinimal.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassSelectTypes.generated.h"

/**
 * Configuration for starting decks configurable per character class in Blueprints.
 */
USTRUCT(BlueprintType)
struct FC_API FFCClassStartingDeckConfig
{
	GENERATED_BODY()

	/** List of Card IDs included in this class's starting deck */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Class|Deck")
	TArray<FName> CardIds;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClassSelectedSignature, EFCCharacterClass, SelectedClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnClassConfirmedSignature, EFCCharacterClass, ConfirmedClass, const TArray<FName>&, StartingDeck);
