#include "Gameplay/Spawner/FCMobSpawnerBase.h"
#include "Gameplay/Spawner/FCMobSpawnSubsystem.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/SphereComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "AI/FCAITypes.h"
#include "Engine/World.h"

AFCMobSpawnerBase::AFCMobSpawnerBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EditorBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
	if (EditorBillboard)
	{
		EditorBillboard->SetupAttachment(SceneRoot);
		EditorBillboard->bIsEditorOnly = true;
	}

	SpawnRadiusVisualizer = CreateDefaultSubobject<USphereComponent>(TEXT("SpawnRadiusVisualizer"));
	if (SpawnRadiusVisualizer)
	{
		SpawnRadiusVisualizer->SetupAttachment(SceneRoot);
		SpawnRadiusVisualizer->SetSphereRadius(SpawnRadius);
		SpawnRadiusVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SpawnRadiusVisualizer->SetCollisionResponseToAllChannels(ECR_Ignore);
		SpawnRadiusVisualizer->bHiddenInGame = true;
	}
}

void AFCMobSpawnerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCMobSpawnerBase, bIsActive, COND_None);
	DOREPLIFETIME_CONDITION(AFCMobSpawnerBase, ActiveMobCount, COND_None);
}

void AFCMobSpawnerBase::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UFCMobSpawnSubsystem* Subsystem = World->GetSubsystem<UFCMobSpawnSubsystem>())
		{
			Subsystem->RegisterSpawner(this);
		}
	}

	if (HasAuthority() && bAutoStart)
	{
		if (InitialDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(InitialDelayTimerHandle, this, &AFCMobSpawnerBase::StartSpawning, InitialDelay, false);
		}
		else
		{
			StartSpawning();
		}
	}
}

void AFCMobSpawnerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialDelayTimerHandle);

		if (UFCMobSpawnSubsystem* Subsystem = World->GetSubsystem<UFCMobSpawnSubsystem>())
		{
			Subsystem->UnregisterSpawner(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AFCMobSpawnerBase::StartSpawning()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsActive = true;
}

void AFCMobSpawnerBase::StopSpawning()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsActive = false;
}

void AFCMobSpawnerBase::ResetSpawner(bool bDestroyActiveMobs)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bDestroyActiveMobs)
	{
		DespawnAllMobs();
	}

	StartSpawning();
}

void AFCMobSpawnerBase::DespawnAllMobs()
{
	if (!HasAuthority())
	{
		return;
	}

	for (TWeakObjectPtr<AFCMobCharacter>& MobPtr : ActiveMobs)
	{
		if (AFCMobCharacter* Mob = MobPtr.Get())
		{
			Mob->SetOwningSpawner(nullptr);
			Mob->Destroy();
		}
	}

	ActiveMobs.Empty();
	SynchronizeActiveMobCount();
}

AFCMobCharacter* AFCMobSpawnerBase::SpawnSingleMob(TSubclassOf<AFCMobCharacter> MobClassOverride)
{
	if (!HasAuthority() || !CanSpawnMob())
	{
		return nullptr;
	}

	TSubclassOf<AFCMobCharacter> ClassToSpawn = MobClassOverride ? MobClassOverride : SelectMobClass();
	if (!ClassToSpawn)
	{
		ClassToSpawn = AFCMobCharacter::StaticClass();
	}

	FVector SpawnLoc = GetActorLocation();
	if (!GetRandomSpawnLocation(SpawnLoc))
	{
		SpawnLoc = GetActorLocation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFCMobCharacter* SpawnedMob = GetWorld()->SpawnActor<AFCMobCharacter>(ClassToSpawn, SpawnLoc, GetActorRotation(), SpawnParams);
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

		OnMobSpawned.Broadcast(SpawnedMob);
		Multicast_PlaySpawnEffect(SpawnLoc);
	}

	return SpawnedMob;
}

bool AFCMobSpawnerBase::GetRandomSpawnLocation(FVector& OutLocation) const
{
	if (LocationType == EFCMobSpawnLocationType::AtSpawner)
	{
		OutLocation = GetActorLocation();
		return true;
	}

	if (LocationType == EFCMobSpawnLocationType::DesignatedSpawnPoints && DesignatedSpawnPoints.Num() > 0)
	{
		TArray<AActor*> ValidPoints;
		for (AActor* PointActor : DesignatedSpawnPoints)
		{
			if (IsValid(PointActor))
			{
				ValidPoints.Add(PointActor);
			}
		}

		if (ValidPoints.Num() > 0)
		{
			const int32 SelectedIndex = FMath::RandRange(0, ValidPoints.Num() - 1);
			OutLocation = ValidPoints[SelectedIndex]->GetActorLocation();
			return true;
		}
	}

	// RandomInRadius
	const FVector Origin = GetActorLocation();

	if (bProjectToNavigation)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation NavLoc;
			if (NavSys->GetRandomReachablePointInRadius(Origin, SpawnRadius, NavLoc))
			{
				OutLocation = NavLoc.Location;
				return true;
			}
		}
	}

	// Fallback to 2D disk with ground trace
	const float RandAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float RandDist = FMath::FRandRange(0.0f, SpawnRadius);
	FVector Candidate = Origin + FVector(FMath::Cos(RandAngle) * RandDist, FMath::Sin(RandAngle) * RandDist, 0.0f);

	FHitResult HitResult;
	FVector TraceStart = Candidate + FVector(0.0f, 0.0f, 500.0f);
	FVector TraceEnd = Candidate - FVector(0.0f, 0.0f, 1000.0f);
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(MobSpawnerGroundTrace), false, this);

	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
	{
		OutLocation = HitResult.Location + FVector(0.0f, 0.0f, 90.0f);
		return true;
	}

	OutLocation = Candidate;
	return true;
}

TSubclassOf<AFCMobCharacter> AFCMobSpawnerBase::SelectMobClass() const
{
	if (MobEntries.Num() == 0)
	{
		return AFCMobCharacter::StaticClass();
	}

	float TotalWeight = 0.0f;
	for (const FFCMobSpawnEntry& Entry : MobEntries)
	{
		if (Entry.MobClass)
		{
			TotalWeight += FMath::Max(0.01f, Entry.Weight);
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return AFCMobCharacter::StaticClass();
	}

	const float RandomRoll = FMath::FRandRange(0.0f, TotalWeight);
	float Accumulated = 0.0f;

	for (const FFCMobSpawnEntry& Entry : MobEntries)
	{
		if (Entry.MobClass)
		{
			Accumulated += FMath::Max(0.01f, Entry.Weight);
			if (RandomRoll <= Accumulated)
			{
				return Entry.MobClass;
			}
		}
	}

	return MobEntries[0].MobClass ? MobEntries[0].MobClass : TSubclassOf<AFCMobCharacter>(AFCMobCharacter::StaticClass());
}

void AFCMobSpawnerBase::HandleMobDied(AFCMobCharacter* DeadMob, AActor* Killer)
{
	ActiveMobs.RemoveAll([DeadMob](const TWeakObjectPtr<AFCMobCharacter>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == DeadMob;
	});

	SynchronizeActiveMobCount();

	OnMobDied.Broadcast(DeadMob, Killer);

	if (ActiveMobCount == 0)
	{
		OnAllMobsDefeated.Broadcast(this);
	}
}

void AFCMobSpawnerBase::HandleMobDestroyed(AFCMobCharacter* DestroyedMob)
{
	const int32 Removed = ActiveMobs.RemoveAll([DestroyedMob](const TWeakObjectPtr<AFCMobCharacter>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == DestroyedMob;
	});

	if (Removed > 0)
	{
		SynchronizeActiveMobCount();

		if (ActiveMobCount == 0)
		{
			OnAllMobsDefeated.Broadcast(this);
		}
	}
}

bool AFCMobSpawnerBase::CanSpawnMob_Implementation() const
{
	return HasAuthority() && bIsActive;
}

void AFCMobSpawnerBase::GetActiveMobs(TArray<AFCMobCharacter*>& OutMobs) const
{
	OutMobs.Reset();
	for (const TWeakObjectPtr<AFCMobCharacter>& MobPtr : ActiveMobs)
	{
		if (AFCMobCharacter* Mob = MobPtr.Get())
		{
			if (!Mob->IsDead())
			{
				OutMobs.Add(Mob);
			}
		}
	}
}

void AFCMobSpawnerBase::SynchronizeActiveMobCount()
{
	ActiveMobs.RemoveAll([](const TWeakObjectPtr<AFCMobCharacter>& MobPtr)
	{
		return !MobPtr.IsValid() || MobPtr->IsDead();
	});

	ActiveMobCount = ActiveMobs.Num();
}

void AFCMobSpawnerBase::Multicast_PlaySpawnEffect_Implementation(const FVector& SpawnLocation)
{
	// Client presentation hook for visual particles/sound cues
}

void AFCMobSpawnerBase::OnRep_IsActive()
{
}

void AFCMobSpawnerBase::OnRep_ActiveMobCount()
{
}
