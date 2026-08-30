#include "Gameplay/FCChestActor.h"
#include "Gameplay/FCCardPickupActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"

AFCChestActor::AFCChestActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// Root Collision Box
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->InitBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	RootComponent = CollisionBox;

	// Visual Meshes
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(RootComponent);
	BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(BaseMesh);
	LidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Ability System Component & Attribute Set (Supports GAS damage effects)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UFCAttributeSet>(TEXT("AttributeSet"));

	// Gameplay Defaults
	DamageThreshold = 1.0f;
	RewardCardIds.Add(FName("Card_Fireball"));
	CardPickupClass = AFCCardPickupActor::StaticClass();
}

void AFCChestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFCChestActor, bIsOpened);
}

UAbilitySystemComponent* AFCChestActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFCChestActor::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

float AFCChestActor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || bIsOpened)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage >= DamageThreshold || DamageAmount >= DamageThreshold)
	{
		DestroyAndSpawnDrops(EventInstigator, DamageCauser);
	}

	return ActualDamage;
}

void AFCChestActor::DestroyAndSpawnDrops(AController* InstigatorController, AActor* DamageCauser)
{
	if (!HasAuthority() || bIsOpened)
	{
		return;
	}

	bIsOpened = true;

	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	UWorld* World = GetWorld();
	if (World)
	{
		TSubclassOf<AFCCardPickupActor> SpawnClass = CardPickupClass ? CardPickupClass : TSubclassOf<AFCCardPickupActor>(AFCCardPickupActor::StaticClass());
		const int32 NumRewards = RewardCardIds.Num();

		for (int32 i = 0; i < NumRewards; ++i)
		{
			const float Angle = (NumRewards > 1) ? (i * (2.0f * PI / NumRewards)) : 0.0f;
			const float Radius = (NumRewards > 1) ? 60.0f : 0.0f;
			const FVector SpawnOffset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 50.0f);
			const FVector SpawnLocation = GetActorLocation() + SpawnOffset;

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			if (AFCCardPickupActor* DropActor = World->SpawnActor<AFCCardPickupActor>(SpawnClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
			{
				DropActor->SetDropCardId(RewardCardIds[i]);
			}
		}
	}

	if (DestroySound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DestroySound, GetActorLocation());
	}

	if (DestroyVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DestroyVFX, GetActorLocation(), GetActorRotation());
	}

	OnChestBrokenCosmetics();
}

void AFCChestActor::OnRep_IsOpened()
{
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (DestroySound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DestroySound, GetActorLocation());
	}

	if (DestroyVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DestroyVFX, GetActorLocation(), GetActorRotation());
	}

	OnChestBrokenCosmetics();
}
