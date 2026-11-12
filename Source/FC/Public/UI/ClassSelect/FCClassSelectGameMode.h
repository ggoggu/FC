#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassSelectGameMode.generated.h"

/**
 * AFCClassSelectGameMode
 * 
 * GameMode for the Class Selection Level (Lv_ClassSelect).
 * Configures the dedicated UI PlayerController, disables character pawn spawning,
 * and manages transition to the target battle or dungeon level upon class confirmation.
 */
UCLASS()
class FC_API AFCClassSelectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFCClassSelectGameMode();

	/** Default URL of the destination map to load upon class confirmation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FC|ClassSelect|Transition")
	FString NextLevelMapURL = TEXT("/Game/Level/Lv_BattleTest");

	/** Transitions all connected players to the target gameplay level */
	UFUNCTION(BlueprintCallable, Category = "FC|ClassSelect|Transition")
	void TransitionToNextLevel();
};
