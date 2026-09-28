#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FCBombDamageable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UFCBombDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 * IFCBombDamageable
 * 
 * Interface for gameplay objects that react to explosive/bomb damage,
 * such as secret room walls, rock obstacles, and destructible props.
 */
class FC_API IFCBombDamageable
{
	GENERATED_BODY()

public:
	/**
	 * Called when explosive/bomb damage hits this actor.
	 * Must be handled authoritatively on the server.
	 * 
	 * @param DamageAmount Amount of damage delivered.
	 * @param HitLocation World location of the explosion impact.
	 * @param DamageCauser Actor responsible for triggering the explosion.
	 * @return True if the damage was successfully applied and resulted in destruction/state change.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FC|Combat|Explosive")
	bool ReceiveBombDamage(float DamageAmount, const FVector& HitLocation, AActor* DamageCauser);
};
