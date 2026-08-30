#include "Combat/Projectile/FCProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"

AFCProjectileBase::AFCProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// Root Collision Sphere
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(20.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	RootComponent = CollisionComponent;

	// Visual Mesh
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Niagara Flight Trail Component
	FlightVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlightVFXComponent"));
	FlightVFXComponent->SetupAttachment(RootComponent);
	FlightVFXComponent->bAutoActivate = true;

	// Projectile Movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 2500.0f;
	ProjectileMovement->MaxSpeed = 2500.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->SetIsReplicated(true);

	InitialLifeSpan = 5.0f;
}

void AFCProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
		CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);

		CollisionComponent->OnComponentHit.AddDynamic(this, &AFCProjectileBase::OnProjectileHit);
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AFCProjectileBase::OnProjectileOverlap);
	}

	if (FlightSound)
	{
		UGameplayStatics::SpawnSoundAttached(FlightSound, RootComponent);
	}
}

void AFCProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator() || OtherActor == GetOwner())
	{
		return;
	}

	ProcessImpact(OtherActor, Hit);
}

void AFCProjectileBase::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetInstigator() || OtherActor == GetOwner())
	{
		return;
	}

	ProcessImpact(OtherActor, SweepResult);
}

void AFCProjectileBase::ProcessImpact(AActor* OtherActor, const FHitResult& HitResult)
{
	if (HitActors.Contains(OtherActor))
	{
		return;
	}
	HitActors.Add(OtherActor);

	const FVector ImpactLocation = HitResult.ImpactPoint.IsZero() ? GetActorLocation() : FVector(HitResult.ImpactPoint);

	// Server-Authoritative Combat Resolution
	if (HasAuthority())
	{
		if (ExplosionRadius > 0.0f)
		{
			// Radial Damage & Impacts
			TArray<FOverlapResult> OverlapResults;
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.AddIgnoredActor(GetInstigator());

			if (GetWorld()->OverlapMultiByChannel(OverlapResults, ImpactLocation, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams))
			{
				for (const FOverlapResult& Overlap : OverlapResults)
				{
					if (AActor* Target = Overlap.GetActor())
					{
						ApplyDamageToActor(Target, HitResult);
					}
				}
			}
		}
		else
		{
			// Single-Target Damage
			ApplyDamageToActor(OtherActor, HitResult);
		}

		// Optional Spawn Actor On Impact (e.g. summon minions, traps, fields)
		if (SpawnActorOnImpact)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<AActor>(SpawnActorOnImpact, ImpactLocation, GetActorRotation(), SpawnParams);
		}

		// Multicast cosmetic impact audio/visuals to simulated proxies
		Multicast_PlayImpactCosmetics(ImpactLocation);

		// Handle Piercing or Destruction
		if (bPiercing && CurrentPierceCount < MaxPierceCount)
		{
			CurrentPierceCount++;
		}
		else
		{
			Destroy();
		}
	}
	else
	{
		// Autonomous predicted client visual feedback
		PlayImpactCosmetics(ImpactLocation);
	}
}

void AFCProjectileBase::ApplyDamageToActor(AActor* TargetActor, const FHitResult& HitResult)
{
	if (!HasAuthority() || !TargetActor || Damage <= 0.0f)
	{
		return;
	}

	// Resolve Target Ability System Component
	UAbilitySystemComponent* TargetASC = nullptr;
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = ASI->GetAbilitySystemComponent();
	}

	if (TargetASC)
	{
		if (DamageEffectClass)
		{
			FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
			Context.AddInstigator(GetInstigator(), this);
			Context.AddHitResult(HitResult);

			if (FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context); SpecHandle.IsValid())
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
		else
		{
			// Direct Attribute Set deduction fallback
			if (const UFCAttributeSet* AttributeSet = TargetASC->GetSet<UFCAttributeSet>())
			{
				float CurrentHealth = AttributeSet->GetHealth();
				const_cast<UFCAttributeSet*>(AttributeSet)->SetHealth(FMath::Clamp(CurrentHealth - Damage, 0.0f, AttributeSet->GetMaxHealth()));
			}
		}
	}
	else
	{
		// Generic Actor Damage fallback
		UGameplayStatics::ApplyPointDamage(TargetActor, Damage, GetVelocity().GetSafeNormal(), HitResult, GetInstigatorController(), this, nullptr);
	}
}

void AFCProjectileBase::PlayImpactCosmetics(const FVector& Location)
{
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
	}

	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactVFX, Location, GetActorRotation());
	}
}

void AFCProjectileBase::Multicast_PlayImpactCosmetics_Implementation(const FVector_NetQuantize& Location)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		PlayImpactCosmetics(Location);
	}
}
