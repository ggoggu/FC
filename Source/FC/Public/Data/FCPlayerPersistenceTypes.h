#pragma once

#include "CoreMinimal.h"
#include "Data/Card/FCCardHandContainer.h"
#include "Data/Class/FCClassTypes.h"
#include "FCPlayerPersistenceTypes.generated.h"

/**
 * FFCCardDeckSaveData
 * 
 * Replicated / Persistent snapshot of all card piles and hand cards
 * preserved across map and level transitions.
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardDeckSaveData
{
	GENERATED_BODY()

	/** Current hand cards preserving GUIDs, upgrade levels, and locks */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Card")
	TArray<FFCCardItem> HandCards;

	/** Draw pile card IDs */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Card")
	TArray<FName> DrawPile;

	/** Discard pile card IDs */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Card")
	TArray<FName> DiscardPile;

	/** Exhaust pile card IDs */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Card")
	TArray<FName> ExhaustPile;
};

/**
 * FFCPlayerStatSaveData
 * 
 * Snapshot of player combat attributes (Health, Mana) and character class
 * preserved across level loads and respawns.
 */
USTRUCT(BlueprintType)
struct FC_API FFCPlayerStatSaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Class")
	EFCCharacterClass CharacterClass = EFCCharacterClass::Mage;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Stats")
	float Health = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Stats")
	float Mana = 50.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Stats")
	float MaxMana = 50.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Stats")
	float AttackPower = 0.0f;
};

/**
 * FFCPlayerPersistentData
 * 
 * Aggregated persistent player session record keyed by player identifier.
 */
USTRUCT(BlueprintType)
struct FC_API FFCPlayerPersistentData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Persistence")
	FString PlayerKey;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence")
	FFCCardDeckSaveData DeckData;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence")
	FFCPlayerStatSaveData StatData;

	UPROPERTY(BlueprintReadWrite, Category = "Persistence")
	bool bHasValidData = false;
};
