#include "Gameplay/Spawner/FCFixedMobSpawner.h"
#include "Character/Mob/FCMobCharacter.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "AI/FCAITypes.h"
#include "TimerManager.h"
#include "Engine/World.h"

AFCFixedMobSpawner::AFCFixedMobSpawner()
{
	bReplicates = true;
	LocationType = EFCMobSpawnLocationType::DesignatedSpawnPoints;
	bProjectToNavigation = true;
	bSpawnAllOnStart = true;
	bClearTimersOnStop = true;
}

void AFCFixedMobSpawner::StartSpawning()
{
	Super::StartSpawning();

	if (!HasAuthority())
	{
		return;
	}

	EnsureTrackingArraysSized();

	if (bSpawnAllOnStart)
	{
		SpawnAllSlots(false);
	}
}

void AFCFixedMobSpawner::StopSpawning()
{
	Super::StopSpawning();

	if (!HasAuthority())
	{
		return;
	}

	if (bClearTimersOnStop)
	{
		if (UWorld* World = GetWorld())
		{
			for (FTimerHandle& Handle : SlotRespawnTimers)
			{
				World->GetTimerManager().ClearTimer(Handle);
			}
		}
	}
}

void AFCFixedMobSpawner::ResetSpawner(bool bDestroyActiveMobs)
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : SlotRespawnTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}

	Super::ResetSpawner(bDestroyActiveMobs);
}

void AFCFixedMobSpawner::DespawnAllMobs()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : SlotRespawnTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}

	for (TWeakObjectPtr<AFCMobCharacter>& SlotMobPtr : SlotLivingMobs)
	{
		SlotMobPtr.Reset();
	}

	Super::DespawnAllMobs();
}

void AFCFixedMobSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		for (FTimerHandle& Handle : SlotRespawnTimers)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AFCFixedMobSpawner::EnsureTrackingArraysSized()
{
	const int32 Count = SpawnSlots.Num();
	if (SlotLivingMobs.Num() != Count)
	{
		SlotLivingMobs.SetNum(Count);
	}
	if (SlotRespawnTimers.Num() != Count)
	{
		SlotRespawnTimers.SetNum(Count);
	}
}

int32 AFCFixedMobSpawner::SpawnAllSlots(bool bForceRespawn)
{
	if (!HasAuthority() || !bIsActive)
	{
		return 0;
	}

	EnsureTrackingArraysSized();

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < SpawnSlots.Num(); ++Index)
	{
		AFCMobCharacter* Spawned = SpawnSlot(Index, bForceRespawn);
		if (Spawned)
		{
			++SpawnedCount;
		}
	}

	return SpawnedCount;
}

AFCMobCharacter* AFCFixedMobSpawner::SpawnSlot(int32 SlotIndex, bool bForceRespawn)
{
	if (!HasAuthority() || !bIsActive)
	{
		return nullptr;
	}

	if (!SpawnSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	EnsureTrackingArraysSized();

	if (SlotLivingMobs[SlotIndex].IsValid())
	{
		if (bForceRespawn)
		{
			DespawnSlot(SlotIndex);
		}
		else
		{
			return SlotLivingMobs[SlotIndex].Get();
		}
	}

	return InternalSpawnSlotMob(SlotIndex);
}

int32 AFCFixedMobSpawner::SpawnSlotsByTag(FName SlotTag, bool bForceRespawn)
{
	if (!HasAuthority() || !bIsActive || SlotTag.IsNone())
	{
		return 0;
	}

	EnsureTrackingArraysSized();

	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < SpawnSlots.Num(); ++Index)
	{
		if (SpawnSlots[Index].SlotTag == SlotTag)
		{
			if (SpawnSlot(Index, bForceRespawn))
			{
				++SpawnedCount;
			}
		}
	}

	return SpawnedCount;
}

void AFCFixedMobSpawner::DespawnSlot(int32 SlotIndex)
{
	if (!HasAuthority() || !SpawnSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	EnsureTrackingArraysSized();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlotRespawnTimers[SlotIndex]);
	}

	if (AFCMobCharacter* LivingMob = SlotLivingMobs[SlotIndex].Get())
	{
		LivingMob->SetOwningSpawner(nullptr);
		LivingMob->Destroy();
		SlotLivingMobs[SlotIndex].Reset();

		ActiveMobs.RemoveAll([LivingMob](const TWeakObjectPtr<AFCMobCharacter>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == LivingMob;
		});

		SynchronizeActiveMobCount();

		if (ActiveMobCount == 0)
		{
			OnAllMobsDefeated.Broadcast(this);
		}
	}
}

void AFCFixedMobSpawner::DespawnSlotsByTag(FName SlotTag)
{
	if (!HasAuthority() || SlotTag.IsNone())
	{
		return;
	}

	EnsureTrackingArraysSized();

	for (int32 Index = 0; Index < SpawnSlots.Num(); ++Index)
	{
		if (SpawnSlots[Index].SlotTag == SlotTag)
		{
			DespawnSlot(Index);
		}
	}
}

AFCMobCharacter* AFCFixedMobSpawner::InternalSpawnSlotMob(int32 SlotIndex)
{
	UWorld* World = GetWorld();
	if (!World || !SpawnSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	// Clear any pending respawn timer for this slot
	World->GetTimerManager().ClearTimer(SlotRespawnTimers[SlotIndex]);

	FTransform WorldTransform;
	if (!GetSlotWorldTransform(SlotIndex, WorldTransform))
	{
		return nullptr;
	}

	FVector SpawnLoc = WorldTransform.GetLocation();
	const FRotator SpawnRot = WorldTransform.Rotator();

	// Optionally project location to NavMesh
	if (bProjectToNavigation)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(SpawnLoc, NavLoc, FVector(200.0f, 200.0f, 400.0f)))
			{
				SpawnLoc = NavLoc.Location;
			}
		}
	}

	// Determine class to spawn
	TSubclassOf<AFCMobCharacter> ClassToSpawn = SpawnSlots[SlotIndex].MobClass ? SpawnSlots[SlotIndex].MobClass : DefaultMobClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = AFCMobCharacter::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFCMobCharacter* SpawnedMob = World->SpawnActor<AFCMobCharacter>(ClassToSpawn, SpawnLoc, SpawnRot, SpawnParams);
	if (SpawnedMob)
	{
		SpawnedMob->SetOwningSpawner(this);

		if (AAIController* AICont = Cast<AAIController>(SpawnedMob->GetController()))
		{
			if (UBlackboardComponent* BlackboardComp = AICont->GetBlackboardComponent())
			{
				BlackboardComp->SetValueAsVector(FFCMobBlackboardKeys::HomeLocation, SpawnLoc);
			}
		}

		ActiveMobs.Add(SpawnedMob);
		SynchronizeActiveMobCount();

		SlotLivingMobs[SlotIndex] = SpawnedMob;

		OnMobSpawned.Broadcast(SpawnedMob);
		OnFixedSlotSpawned.Broadcast(SlotIndex, SpawnedMob);
	}

	return SpawnedMob;
}

void AFCFixedMobSpawner::HandleMobDied(AFCMobCharacter* DeadMob, AActor* Killer)
{
	Super::HandleMobDied(DeadMob, Killer);

	const int32 SlotIndex = FindSlotIndexForMob(DeadMob);
	if (SlotIndex != INDEX_NONE)
	{
		SlotLivingMobs[SlotIndex].Reset();
		OnFixedSlotMobDied.Broadcast(SlotIndex, DeadMob, Killer);

		const FFCFixedMobSpawnSlot& Slot = SpawnSlots[SlotIndex];
		if (Slot.bRespawnOnDeath && HasAuthority() && bIsActive)
		{
			if (UWorld* World = GetWorld())
			{
				FTimerDelegate RespawnDel;
				RespawnDel.BindUObject(this, &AFCFixedMobSpawner::HandleSlotRespawnTimer, SlotIndex);
				World->GetTimerManager().SetTimer(
					SlotRespawnTimers[SlotIndex],
					RespawnDel,
					FMath::Max(0.1f, Slot.RespawnDelay),
					false
				);
			}
		}
	}
}

void AFCFixedMobSpawner::HandleMobDestroyed(AFCMobCharacter* DestroyedMob)
{
	Super::HandleMobDestroyed(DestroyedMob);

	const int32 SlotIndex = FindSlotIndexForMob(DestroyedMob);
	if (SlotIndex != INDEX_NONE)
	{
		SlotLivingMobs[SlotIndex].Reset();
	}
}

void AFCFixedMobSpawner::HandleSlotRespawnTimer(int32 SlotIndex)
{
	if (!HasAuthority() || !bIsActive || !SpawnSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	SpawnSlot(SlotIndex, false);
}

bool AFCFixedMobSpawner::GetSlotWorldTransform(int32 SlotIndex, FTransform& OutTransform) const
{
	if (!SpawnSlots.IsValidIndex(SlotIndex))
	{
		OutTransform = GetActorTransform();
		return false;
	}

	const FFCFixedMobSpawnSlot& Slot = SpawnSlots[SlotIndex];
	if (IsValid(Slot.TargetSpawnActor))
	{
		OutTransform = Slot.TargetSpawnActor->GetActorTransform();
		return true;
	}

	OutTransform = Slot.RelativeTransform * GetActorTransform();
	return true;
}

AFCMobCharacter* AFCFixedMobSpawner::GetLivingSlotMob(int32 SlotIndex) const
{
	if (SlotLivingMobs.IsValidIndex(SlotIndex))
	{
		return SlotLivingMobs[SlotIndex].Get();
	}
	return nullptr;
}

bool AFCFixedMobSpawner::IsSlotAlive(int32 SlotIndex) const
{
	if (SlotLivingMobs.IsValidIndex(SlotIndex))
	{
		AFCMobCharacter* Mob = SlotLivingMobs[SlotIndex].Get();
		return Mob && !Mob->IsDead();
	}
	return false;
}

bool AFCFixedMobSpawner::IsSlotPendingRespawn(int32 SlotIndex) const
{
	if (SlotRespawnTimers.IsValidIndex(SlotIndex))
	{
		if (UWorld* World = GetWorld())
		{
			return World->GetTimerManager().IsTimerActive(SlotRespawnTimers[SlotIndex]);
		}
	}
	return false;
}

int32 AFCFixedMobSpawner::FindSlotIndexForMob(const AFCMobCharacter* Mob) const
{
	if (!Mob)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < SlotLivingMobs.Num(); ++Index)
	{
		if (SlotLivingMobs[Index].Get() == Mob)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

int32 AFCFixedMobSpawner::AddSpawnSlot(const FFCFixedMobSpawnSlot& NewSlot)
{
	EnsureTrackingArraysSized();
	const int32 NewIndex = SpawnSlots.Add(NewSlot);
	SlotLivingMobs.AddDefaulted(1);
	SlotRespawnTimers.AddDefaulted(1);
	return NewIndex;
}

void AFCFixedMobSpawner::ClearSpawnSlots()
{
	DespawnAllMobs();
	SpawnSlots.Empty();
	SlotLivingMobs.Empty();
	SlotRespawnTimers.Empty();
}
