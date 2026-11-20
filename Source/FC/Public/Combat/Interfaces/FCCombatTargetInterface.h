#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "FCCombatTargetInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UFCCombatTargetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * IFCCombatTargetInterface
 *
 * Polymorphic combat targetability and faction contract implemented by characters,
 * destructible chests/walls, and custom combat actors. Eliminates Cast/IsA chains in
 * UFCCombatUtils and abilities.
 */
class FC_API IFCCombatTargetInterface
{
	GENERATED_BODY()

public:
	/** Returns the faction or target classification tag (e.g., Combat.Team.Player, Combat.Team.Enemy, Combat.Target.Destructible) */
	virtual FGameplayTag GetCombatTeamTag() const = 0;

	/** Returns true if this target is currently alive/intact and eligible for combat actions */
	virtual bool IsCombatTargetAlive() const = 0;

	/** Returns true if SourceActor is permitted to attack this target */
	virtual bool CanBeTargetedBy(const AActor* SourceActor) const = 0;

	/** Returns the preferred 3D world point projectiles/spells should aim at (e.g., chest socket or actor center) */
	virtual FVector GetCombatAimLocation() const = 0;
};
