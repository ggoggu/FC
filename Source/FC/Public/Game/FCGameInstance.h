#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FCGameInstance.generated.h"

/**
 * UFCGameInstance
 * 
 * Persistent game instance handling global session management,
 * player data persistence, and lifecycle events across map transitions.
 */
UCLASS()
class FC_API UFCGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFCGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;
};
