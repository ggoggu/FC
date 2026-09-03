#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FCGE_MagicShield.generated.h"

/**
 * UFCGE_MagicShield
 * 
 * GameplayEffect that adds a protective shield barrier to the target UFCAttributeSet.
 */
UCLASS()
class FC_API UFCGE_MagicShield : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFCGE_MagicShield();
};
