#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FCGameState.generated.h"

/**
 * AFCGameState
 * 
 * Replicated game state synchronized across all clients for match-wide data,
 * card combat state, and global match progression.
 */
UCLASS()
class FC_API AFCGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFCGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
