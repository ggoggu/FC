#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FCGameMode.generated.h"

class AFCDungeonManager;

/**
 * AFCGameMode
 * 
 * Authoritative server-side game mode managing multiplayer card match lifecycle,
 * round progression, and player spawning/assignment.
 */
UCLASS()
class FC_API AFCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFCGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;

	/** Get or find the active dungeon manager in the level */
	UFUNCTION(BlueprintPure, Category = "FC|Dungeon")
	AFCDungeonManager* GetDungeonManager() const;

	/** Set or register the active dungeon manager */
	UFUNCTION(BlueprintCallable, Category = "FC|Dungeon")
	void SetDungeonManager(AFCDungeonManager* InDungeonManager);

	/** Saves all connected player states to persistence and transitions to the target level */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|GameMode")
	void TransitionToLevel(const FString& MapURL, bool bSeamless = false);

	/** Number of cards drawn into player's hand when match/session starts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Card", meta = (ClampMin = "1"))
	int32 StartingHandSize = 5;

	/** If true, initializes player deck from their character class data asset. If false, uses GameMode's DefaultStartingDeck. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Card")
	bool bUseClassStartingDeck = false;

	/** Starting deck used for players (used when bUseClassStartingDeck is false or as fallback) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Card")
	TArray<FName> DefaultStartingDeck;

	/** Periodic hand cycle duration in seconds (default: 30.0s) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Card", meta = (ClampMin = "1.0"))
	float DefaultCycleInterval = 30.0f;

	/** Number of cards drawn on periodic hand cycle (default: 5) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Card", meta = (ClampMin = "0"))
	int32 DefaultCycleDrawCount = 5;

	/** Whether to automatically run periodic hand cycle for connected players */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Card")
	bool bEnableAutoHandCycle = true;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	mutable TWeakObjectPtr<AFCDungeonManager> CachedDungeonManager;
};
