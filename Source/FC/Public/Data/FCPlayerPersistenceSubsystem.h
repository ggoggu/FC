#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/FCPlayerPersistenceTypes.h"
#include "FCPlayerPersistenceSubsystem.generated.h"

class APlayerController;

/**
 * UFCPlayerPersistenceSubsystem
 * 
 * GameInstance Subsystem managing player persistence across level and map transitions.
 * Authoritatively stores and restores Hand cards, Deck piles, and GAS Attribute stats.
 */
UCLASS()
class FC_API UFCPlayerPersistenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Static helper to get the persistence subsystem from any world context */
	UFUNCTION(BlueprintPure, Category = "FC|Persistence", meta = (WorldContext = "WorldContextObject"))
	static UFCPlayerPersistenceSubsystem* Get(const UObject* WorldContextObject);

	/** Generates a unique key string for the given player controller */
	UFUNCTION(BlueprintPure, Category = "FC|Persistence")
	FString GetPlayerKey(const APlayerController* PC) const;

	/** Checks if saved session data exists for the given player key */
	UFUNCTION(BlueprintPure, Category = "FC|Persistence")
	bool HasPlayerData(const FString& PlayerKey) const;

	/** Authoritatively saves deck and stats for a given player key */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Persistence")
	void SavePlayerData(const FString& PlayerKey, const FFCCardDeckSaveData& DeckData, const FFCPlayerStatSaveData& StatData);

	/** Authoritatively loads deck and stats for a given player key */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Persistence")
	bool LoadPlayerData(const FString& PlayerKey, FFCCardDeckSaveData& OutDeckData, FFCPlayerStatSaveData& OutStatData) const;

	/** Automatically extracts and saves Deck and Stats from PlayerController */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Persistence")
	bool SaveFromPlayer(APlayerController* PC);

	/** Automatically restores Deck and Stats to PlayerController, PlayerState, and Pawn */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Persistence")
	bool RestoreToPlayer(APlayerController* PC);

	/** Clears saved session data for a specific player */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Persistence")
	void ClearPlayerData(const FString& PlayerKey);

	/** Clears all saved player session records */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|Persistence")
	void ClearAllPlayerData();

protected:
	/** Map storing player persistent records by unique player key */
	UPROPERTY(Transient)
	TMap<FString, FFCPlayerPersistentData> PersistentRecords;
};
