#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/FCGA_SpawnProjectile.h"
#include "FCGA_Fireball.generated.h"

/**
 * UFCGA_Fireball
 * 
 * Specialized Fireball card ability that shoots a 1-damage fireball projectile
 * in a straight line in the forward facing direction of the player character.
 */
UCLASS()
class FC_API UFCGA_Fireball : public UFCGA_SpawnProjectile
{
	GENERATED_BODY()

public:
	UFCGA_Fireball();
};
