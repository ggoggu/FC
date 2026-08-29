#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FCPlayerState.generated.h"

/**
 * AFCPlayerState
 * 
 * Replicated player state containing individual player combat and deck data,
 * ready for Fast Array card collections, turn status, and private stats.
 */
UCLASS()
class FC_API AFCPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFCPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
