#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FCGameMode.generated.h"

/**
 * AFCGameMode
 * 
 * Authoritative server-side game mode managing multiplayer card match lifecycle,
 * round progression, turn management, and player spawning/assignment.
 */
UCLASS()
class FC_API AFCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFCGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	/** Saves all connected player states to persistence and transitions to the target level */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FC|GameMode")
	void TransitionToLevel(const FString& MapURL, bool bSeamless = false);

protected:
	virtual void BeginPlay() override;
};
