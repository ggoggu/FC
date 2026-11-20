#pragma once

#include "CoreMinimal.h"
#include "FCCardEffectBase.generated.h"

struct FFCCardExecutionContext;

/**
 * FFCCardEffectBase
 *
 * Stateless TInstancedStruct composite fragment executed when a card resolves on the server.
 * Multiple fragments compose cleanly on a single DataTable card row (e.g. Damage + Shield + Draw).
 */
USTRUCT(BlueprintType)
struct FC_API FFCCardEffectBase
{
	GENERATED_BODY()

	virtual ~FFCCardEffectBase() = default;

	/** Executes this effect fragment on the authoritative server */
	virtual bool Execute(FFCCardExecutionContext& Context) const
	{
		return false;
	}

	/** Collects soft asset references (GE/GA/Projectile/VFX) for async preloading */
	virtual void GatherSoftAssetPaths(TArray<FSoftObjectPath>& OutPaths) const {}
};
