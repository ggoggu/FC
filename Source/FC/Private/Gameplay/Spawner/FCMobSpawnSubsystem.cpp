#include "Gameplay/Spawner/FCMobSpawnSubsystem.h"
#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "Engine/World.h"

void UFCMobSpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisteredSpawners.Empty();
}

void UFCMobSpawnSubsystem::Deinitialize()
{
	RegisteredSpawners.Empty();
	Super::Deinitialize();
}

UFCMobSpawnSubsystem* UFCMobSpawnSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	return World ? World->GetSubsystem<UFCMobSpawnSubsystem>() : nullptr;
}

void UFCMobSpawnSubsystem::RegisterSpawner(AFCMobSpawnerBase* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	for (int32 Index = RegisteredSpawners.Num() - 1; Index >= 0; --Index)
	{
		if (!RegisteredSpawners[Index].IsValid())
		{
			RegisteredSpawners.RemoveAt(Index);
		}
		else if (RegisteredSpawners[Index].Get() == Spawner)
		{
			return; // Already registered
		}
	}

	RegisteredSpawners.Add(Spawner);
}

void UFCMobSpawnSubsystem::UnregisterSpawner(AFCMobSpawnerBase* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	RegisteredSpawners.RemoveAll([Spawner](const TWeakObjectPtr<AFCMobSpawnerBase>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Spawner;
	});
}

void UFCMobSpawnSubsystem::GetAllSpawners(TArray<AFCMobSpawnerBase*>& OutSpawners) const
{
	OutSpawners.Reset();
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			OutSpawners.Add(Spawner);
		}
	}
}

void UFCMobSpawnSubsystem::FindSpawnersByTag(FName Tag, TArray<AFCMobSpawnerBase*>& OutSpawners) const
{
	OutSpawners.Reset();
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			if (Spawner->ActorHasTag(Tag))
			{
				OutSpawners.Add(Spawner);
			}
		}
	}
}

int32 UFCMobSpawnSubsystem::GetGlobalActiveMobCount() const
{
	int32 Total = 0;
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (const AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			Total += Spawner->GetActiveMobCount();
		}
	}
	return Total;
}

int32 UFCMobSpawnSubsystem::GetActiveMobCountByTag(FName Tag) const
{
	int32 Total = 0;
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (const AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			if (Spawner->ActorHasTag(Tag))
			{
				Total += Spawner->GetActiveMobCount();
			}
		}
	}
	return Total;
}

void UFCMobSpawnSubsystem::ActivateAllSpawners()
{
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			Spawner->StartSpawning();
		}
	}
}

void UFCMobSpawnSubsystem::DeactivateAllSpawners()
{
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			Spawner->StopSpawning();
		}
	}
}

void UFCMobSpawnSubsystem::ResetAllSpawners(bool bDestroyActiveMobs)
{
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			Spawner->ResetSpawner(bDestroyActiveMobs);
		}
	}
}

void UFCMobSpawnSubsystem::ActivateSpawnersByTag(FName Tag)
{
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			if (Spawner->ActorHasTag(Tag))
			{
				Spawner->StartSpawning();
			}
		}
	}
}

void UFCMobSpawnSubsystem::DeactivateSpawnersByTag(FName Tag)
{
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			if (Spawner->ActorHasTag(Tag))
			{
				Spawner->StopSpawning();
			}
		}
	}
}

void UFCMobSpawnSubsystem::ResetSpawnersByTag(FName Tag, bool bDestroyActiveMobs)
{
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			if (Spawner->ActorHasTag(Tag))
			{
				Spawner->ResetSpawner(bDestroyActiveMobs);
			}
		}
	}
}

void UFCMobSpawnSubsystem::DespawnAllWorldMobs()
{
	for (const TWeakObjectPtr<AFCMobSpawnerBase>& Entry : RegisteredSpawners)
	{
		if (AFCMobSpawnerBase* Spawner = Entry.Get())
		{
			Spawner->DespawnAllMobs();
		}
	}
}
