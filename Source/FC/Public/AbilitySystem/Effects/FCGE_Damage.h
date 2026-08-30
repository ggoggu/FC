#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FCGE_Damage.generated.h"

/**
 * UFCGE_Damage
 * 
 * Instant GameplayEffect that reduces Health attribute on the target UFCAttributeSet.
 */
UCLASS()
class FC_API UFCGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFCGE_Damage();
};
