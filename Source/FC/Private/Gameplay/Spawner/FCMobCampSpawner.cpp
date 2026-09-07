#include "Gameplay/Spawner/FCMobCampSpawner.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

AFCMobCampSpawner::AFCMobCampSpawner()
{
	bReplicates = true;

	MinMobCount = 3;
	MaxMobCount = 5;
	TargetMobCount = 3;

	bAutoReplenishDeficit = true;
	MinReplenishDelay = 5.0f;
	MaxReplenishDelay = 10.0f;

	bRespawnOnWipe = true;
	MinWipeRespawnDelay = 10.0f;
	MaxWipeRespawnDelay = 20.0f;

	bEnableTerritoryLeash = true;
	CampTerritoryRadius = 2500.0f;
	LeashCheckInterval = 3.0f;
}

void AFCMobCampSpawner::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCMobCampSpawner, TargetMobCount, COND_None);
}

void AFCMobCampSpawner::StartSpawning()
{
	Super::StartSpawning();

	if (!HasAuthority())
	{
		return;
	}

	// Calculate initial target count within range
	TargetMobCount = FMath::RandRange(FMath::Min(MinMobCount, MaxMobCount), FMath::Max(MinMobCount, MaxMobCount));

	// Spawn mobs to meet TargetMobCount
	while (ActiveMobCount < TargetMobCount)
	{
		AFCMobCharacter* Spawned = SpawnSingleMob();
		if (!Spawned)
		{
			break;
		}
	}

	// Start territory leash monitoring
	if (bEnableTerritoryLeash && LeashCheckInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(LeashCheckTimerHandle, this, &AFCMobCampSpawner::CheckTerritoryLeash, LeashCheckInterval, true);
	}
}

void AFCMobCampSpawner::StopSpawning()
{
	Super::StopSpawning();

	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReplenishTimerHandle);
		World->GetTimerManager().ClearTimer(WipeRespawnTimerHandle);
		World->GetTimerManager().ClearTimer(LeashCheckTimerHandle);
	}
}

void AFCMobCampSpawner::ResetSpawner(bool bDestroyActiveMobs)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReplenishTimerHandle);
		World->GetTimerManager().ClearTimer(WipeRespawnTimerHandle);
		World->GetTimerManager().ClearTimer(LeashCheckTimerHandle);
	}

	Super::ResetSpawner(bDestroyActiveMobs);
}

void AFCMobCampSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReplenishTimerHandle);
		World->GetTimerManager().ClearTimer(WipeRespawnTimerHandle);
		World->GetTimerManager().ClearTimer(LeashCheckTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AFCMobCampSpawner::HandleMobDied(AFCMobCharacter* DeadMob, AActor* Killer)
{
	Super::HandleMobDied(DeadMob, Killer);

	if (!HasAuthority() || !bIsActive)
	{
		return;
	}

	EvaluatePopulationState();
}

void AFCMobCampSpawner::HandleMobDestroyed(AFCMobCharacter* DestroyedMob)
{
	Super::HandleMobDestroyed(DestroyedMob);

	if (!HasAuthority() || !bIsActive)
	{
		return;
	}

	EvaluatePopulationState();
}

void AFCMobCampSpawner::EvaluatePopulationState()
{
	if (!HasAuthority() || !bIsActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Total Wipeout Condition
	if (ActiveMobCount == 0)
	{
		// Cancel any pending replenish timer as total wipe takes precedence
		World->GetTimerManager().ClearTimer(ReplenishTimerHandle);

		if (bRespawnOnWipe && !World->GetTimerManager().IsTimerActive(WipeRespawnTimerHandle))
		{
			const float DelayMin = FMath::Min(MinWipeRespawnDelay, MaxWipeRespawnDelay);
			const float DelayMax = FMath::Max(MinWipeRespawnDelay, MaxWipeRespawnDelay);
			const float RespawnDelay = FMath::FRandRange(DelayMin, DelayMax);

			TargetMobCount = FMath::RandRange(FMath::Min(MinMobCount, MaxMobCount), FMath::Max(MinMobCount, MaxMobCount));

			World->GetTimerManager().SetTimer(WipeRespawnTimerHandle, this, &AFCMobCampSpawner::RespawnFullCamp, RespawnDelay, false);
			OnCampRespawnStarted.Broadcast(RespawnDelay, TargetMobCount);
		}
		return;
	}

	// 2. Partial Deficit Condition (Count falls below MinMobCount)
	if (ActiveMobCount < MinMobCount)
	{
		if (bAutoReplenishDeficit && !World->GetTimerManager().IsTimerActive(ReplenishTimerHandle) && !World->GetTimerManager().IsTimerActive(WipeRespawnTimerHandle))
		{
			const float DelayMin = FMath::Min(MinReplenishDelay, MaxReplenishDelay);
			const float DelayMax = FMath::Max(MinReplenishDelay, MaxReplenishDelay);
			const float ReplenishDelay = FMath::FRandRange(DelayMin, DelayMax);

			const int32 Needed = FMath::Max(0, TargetMobCount - ActiveMobCount);

			World->GetTimerManager().SetTimer(ReplenishTimerHandle, this, &AFCMobCampSpawner::ReplenishDeficit, ReplenishDelay, false);
			OnCampReplenishStarted.Broadcast(Needed, ReplenishDelay);
		}
	}
}

void AFCMobCampSpawner::RespawnFullCamp()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WipeRespawnTimerHandle);
	}

	if (!HasAuthority() || !bIsActive)
	{
		return;
	}

	while (ActiveMobCount < TargetMobCount)
	{
		AFCMobCharacter* Spawned = SpawnSingleMob();
		if (!Spawned)
		{
			break;
		}
	}
}

void AFCMobCampSpawner::ReplenishDeficit()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReplenishTimerHandle);
	}

	if (!HasAuthority() || !bIsActive)
	{
		return;
	}

	while (ActiveMobCount < TargetMobCount)
	{
		AFCMobCharacter* Spawned = SpawnSingleMob();
		if (!Spawned)
		{
			break;
		}
	}
}

void AFCMobCampSpawner::ForceReplenishDeficit()
{
	if (!HasAuthority())
	{
		return;
	}

	ReplenishDeficit();
}

void AFCMobCampSpawner::ForceFullRespawn()
{
	if (!HasAuthority())
	{
		return;
	}

	DespawnAllMobs();
	TargetMobCount = FMath::RandRange(FMath::Min(MinMobCount, MaxMobCount), FMath::Max(MinMobCount, MaxMobCount));
	RespawnFullCamp();
}

void AFCMobCampSpawner::CheckTerritoryLeash()
{
	if (!HasAuthority() || !bIsActive || !bEnableTerritoryLeash)
	{
		return;
	}

	const FVector SpawnerLoc = GetActorLocation();
	const float SqrRadius = FMath::Square(CampTerritoryRadius);
	bool bMobRemoved = false;

	for (int32 Index = ActiveMobs.Num() - 1; Index >= 0; --Index)
	{
		if (AFCMobCharacter* Mob = ActiveMobs[Index].Get())
		{
			if (!Mob->IsDead())
			{
				const float DistSq = FVector::DistSquared(Mob->GetActorLocation(), SpawnerLoc);
				if (DistSq > SqrRadius)
				{
					// Out of bounds: safely destroy stray mob and trigger replenishment
					Mob->SetOwningSpawner(nullptr);
					Mob->Destroy();
					ActiveMobs.RemoveAt(Index);
					bMobRemoved = true;
				}
			}
		}
		else
		{
			ActiveMobs.RemoveAt(Index);
			bMobRemoved = true;
		}
	}

	if (bMobRemoved)
	{
		SynchronizeActiveMobCount();
		EvaluatePopulationState();
	}
}

bool AFCMobCampSpawner::IsWipeRespawnPending() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetTimerManager().IsTimerActive(WipeRespawnTimerHandle);
	}
	return false;
}

bool AFCMobCampSpawner::IsReplenishPending() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetTimerManager().IsTimerActive(ReplenishTimerHandle);
	}
	return false;
}
