#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "FCGE_AttackBuff.generated.h"

/**
 * UFCGE_AttackBuff
 * 
 * Duration-based (1 minute / 60s) GameplayEffect that increases the AttackPower
 * attribute by 1 on the target UFCAttributeSet.
 */
UCLASS()
class FC_API UFCGE_AttackBuff : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UFCGE_AttackBuff();
};
