#pragma once

#include "CoreMinimal.h"
#include "Character/FCCharacterBase.h"
#include "FCMobCharacter.generated.h"

UCLASS()
class FC_API AFCMobCharacter : public AFCCharacterBase
{
	GENERATED_BODY()

public:
	AFCMobCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();
};
