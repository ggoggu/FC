#include "Combat/Projectile/FCProjectileBase.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
#include "Combat/Projectile/Behaviors/FCProjectileBehaviors.h"
#include "Combat/Interfaces/FCCombatTargetInterface.h"
#include "Data/FCGameplayTags.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "Combat/FCCombatUtils.h"
#include "Map/FCRoomBase.h"
#include "Map/FCBombDamageable.h"
#include "Net/UnrealNetwork.h"

AFCProjectileBase::AFCProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	DamageEffectClass = UFCGE_Damage::StaticClass();
	SourceClassTag = FCGameplayTags::Class_Mage;
	SourceCardTypeTag = FCGameplayTags::Card_Type_Attack;
	AttackTags.AddTag(FCGameplayTags::Combat_Attack_Projectile);

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

void AFCProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCProjectileBase, ProjectileDataAsset, COND_InitialOnly);
}

FGameplayTagContainer AFCProjectileBase::GetAttackTags() const
{
	return AttackTags;
}

bool AFCProjectileBase::CanBeInterceptedBy(const AActor* InterceptorActor) const
{
	if (!InterceptorActor || InterceptorActor == this)
	{
		return false;
	}

	if (AttackTags.HasTag(FCGameplayTags::Combat_Attack_Unblockable))
	{
		return false;
	}

	// Friendly fire protection: do not intercept own or caster's projectiles
	if (GetInstigator() && (InterceptorActor == GetInstigator() || InterceptorActor->GetInstigator() == GetInstigator()))
	{
		return false;
	}

	if (GetOwner() && (InterceptorActor == GetOwner() || InterceptorActor->GetOwner() == GetOwner()))
	{
		return false;
	}

	return true;
}

void AFCProjectileBase::OnInterceptedBy(AActor* InterceptorActor, const FVector& ImpactLocation)
{
	if (!HasAuthority())
	{
		return;
	}

	PlayImpactCosmetics(ImpactLocation);
	Destroy();
}

void AFCProjectileBase::OnRep_ProjectileDataAsset()
{
	if (ProjectileDataAsset && !bInitializedFromDataAsset)
	{
		InitializeFromDataAsset(ProjectileDataAsset);
	}
}

void AFCProjectileBase::InitializeFromDataAsset(const UFCProjectileDataAsset* InDataAsset)
{
	if (!InDataAsset)
	{
		return;
	}

	ProjectileDataAsset = InDataAsset;
	bInitializedFromDataAsset = true;
	bBehaviorsNormalized = false;

	// Combat configuration
	Damage = InDataAsset->Damage;
	AttackPowerScale = InDataAsset->AttackPowerScale;
	DamageEffectClass = InDataAsset->DamageEffectClass ? InDataAsset->DamageEffectClass : TSubclassOf<UGameplayEffect>(UFCGE_Damage::StaticClass());
	ExplosionRadius = InDataAsset->ExplosionRadius;
	bPiercing = InDataAsset->bPiercing;
	MaxPierceCount = InDataAsset->MaxPierceCount;
	SpawnActorOnImpact = InDataAsset->SpawnActorOnImpact;
	ProjectileElementTags = InDataAsset->ProjectileElementTags;
	SourceClassTag = InDataAsset->SourceClassTag.IsValid() ? InDataAsset->SourceClassTag : FCGameplayTags::Class_Mage.GetTag();
	SourceCardTypeTag = InDataAsset->SourceCardTypeTag.IsValid() ? InDataAsset->SourceCardTypeTag : FCGameplayTags::Card_Type_Attack.GetTag();
	Behaviors = InDataAsset->Behaviors;

	if (!InDataAsset->AttackTags.IsEmpty())
	{
		AttackTags = InDataAsset->AttackTags;
	}
	AttackTags.AddTag(FCGameplayTags::Combat_Attack_Projectile);

	// Movement Component
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InDataAsset->LaunchSpeed;
		ProjectileMovement->MaxSpeed = InDataAsset->MaxSpeed;
		ProjectileMovement->ProjectileGravityScale = InDataAsset->GravityScale;
		ProjectileMovement->bRotationFollowsVelocity = InDataAsset->bRotationFollowsVelocity;
		ProjectileMovement->bShouldBounce = InDataAsset->bShouldBounce;
	}

	SetLifeSpan(InDataAsset->LifeSpan);

	if (UNiagaraSystem* LoadedImpactVFX = InDataAsset->ImpactVFX.LoadSynchronous())
	{
		ImpactVFX = LoadedImpactVFX;
	}

	if (USoundBase* LoadedImpactSound = InDataAsset->ImpactSound.LoadSynchronous())
	{
		ImpactSound = LoadedImpactSound;
	}

	EnsureNormalizedBehaviors();
}

void AFCProjectileBase::EnsureNormalizedBehaviors()
{
	if (bBehaviorsNormalized)
	{
		return;
	}
	bBehaviorsNormalized = true;

	auto HasBehaviorOfType = [this](const UScriptStruct* StructType) -> bool
	{
		for (const TInstancedStruct<FFCProjectileBehaviorBase>& Entry : Behaviors)
		{
			if (Entry.GetScriptStruct() && Entry.GetScriptStruct()->IsChildOf(StructType))
			{
				return true;
			}
		}
		return false;
	};

	if (ExplosionRadius > 0.0f && !HasBehaviorOfType(FFCProjectileBehavior_RadialExplosion::StaticStruct()))
	{
		FFCProjectileBehavior_RadialExplosion Radial;
		Radial.ExplosionRadius = ExplosionRadius;
		Behaviors.Add(TInstancedStruct<FFCProjectileBehaviorBase>::Make<FFCProjectileBehavior_RadialExplosion>(Radial));
	}

	if (SpawnActorOnImpact && !HasBehaviorOfType(FFCProjectileBehavior_SpawnActorOnImpact::StaticStruct()))
	{
		FFCProjectileBehavior_SpawnActorOnImpact SpawnBehavior;
		SpawnBehavior.ActorClass = SpawnActorOnImpact;
		Behaviors.Add(TInstancedStruct<FFCProjectileBehaviorBase>::Make<FFCProjectileBehavior_SpawnActorOnImpact>(SpawnBehavior));
	}

	if (bPiercing && MaxPierceCount > 0 && !HasBehaviorOfType(FFCProjectileBehavior_Pierce::StaticStruct()))
	{
		FFCProjectileBehavior_Pierce PierceBehavior;
		PierceBehavior.MaxPierceCount = MaxPierceCount;
		Behaviors.Add(TInstancedStruct<FFCProjectileBehaviorBase>::Make<FFCProjectileBehavior_Pierce>(PierceBehavior));
	}
}

void AFCProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileDataAsset && !bInitializedFromDataAsset)
	{
		InitializeFromDataAsset(ProjectileDataAsset);
	}

	EnsureNormalizedBehaviors();

	if (HasAuthority())
	{
		for (const TInstancedStruct<FFCProjectileBehaviorBase>& Entry : Behaviors)
		{
			if (const FFCProjectileBehaviorBase* Behavior = Entry.GetPtr())
			{
				Behavior->OnProjectileSpawned(this);
			}
		}
	}

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

	if (OtherActor->IsA<AFCRoomBase>() || OtherActor->IsA<AWorldSettings>())
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

	if (OtherActor->IsA<AFCRoomBase>() || OtherActor->IsA<AWorldSettings>())
	{
		return;
	}

	const bool bValidCombatTarget = UFCCombatUtils::IsAttackableTarget(GetInstigator(), OtherActor)
		|| OtherActor->Implements<UFCCombatTargetInterface>()
		|| OtherActor->Implements<UFCBombDamageable>()
		|| Cast<IAbilitySystemInterface>(OtherActor) != nullptr;

	if (OtherComp)
	{
		const FName ProfileName = OtherComp->GetCollisionProfileName();
		if ((ProfileName == FName("Trigger") || ProfileName == FName("OverlapAll") || ProfileName == FName("OverlapAllDynamic")) && !bValidCombatTarget)
		{
			return;
		}
	}

	const bool bBlockingComp = OtherComp && (OtherComp->GetCollisionResponseToChannel(CollisionComponent->GetCollisionObjectType()) == ECR_Block);
	if (!bValidCombatTarget && !bBlockingComp)
	{
		return;
	}

	ProcessImpact(OtherActor, SweepResult);
}

void AFCProjectileBase::Destroyed()
{
	Super::Destroyed();

	if (GetNetMode() != NM_DedicatedServer && !bHasPlayedCosmetics)
	{
		PlayImpactCosmetics(GetActorLocation());
	}
}

void AFCProjectileBase::ProcessImpact(AActor* OtherActor, const FHitResult& HitResult)
{
	if (HitActors.Contains(OtherActor))
	{
		return;
	}
	HitActors.Add(OtherActor);

	const FVector ImpactLocation = HitResult.ImpactPoint.IsZero() ? GetActorLocation() : FVector(HitResult.ImpactPoint);

	if (HasAuthority())
	{
		EnsureNormalizedBehaviors();

		bool bHasRadialBehavior = false;
		for (const TInstancedStruct<FFCProjectileBehaviorBase>& Entry : Behaviors)
		{
			if (Entry.GetScriptStruct() && Entry.GetScriptStruct()->IsChildOf(FFCProjectileBehavior_RadialExplosion::StaticStruct()))
			{
				bHasRadialBehavior = true;
				break;
			}
		}

		// Direct hit damage when no radial explosion behavior replaces single-target damage
		if (!bHasRadialBehavior)
		{
			ApplyDamageToActor(OtherActor, HitResult);
		}

		bool bContinueFlight = false;
		for (const TInstancedStruct<FFCProjectileBehaviorBase>& Entry : Behaviors)
		{
			if (const FFCProjectileBehaviorBase* Behavior = Entry.GetPtr())
			{
				Behavior->OnProjectileImpact(this, OtherActor, HitResult, bContinueFlight);
			}
		}

		if (bContinueFlight)
		{
			Multicast_PlayImpactCosmetics(ImpactLocation);
		}
		else
		{
			PlayImpactCosmetics(ImpactLocation);
			Destroy();
		}
	}
	else
	{
		PlayImpactCosmetics(ImpactLocation);
	}
}

void AFCProjectileBase::ApplyDamageToActor(AActor* TargetActor, const FHitResult& HitResult)
{
	if (!HasAuthority() || !TargetActor || Damage <= 0.0f)
	{
		return;
	}

	const bool bIsBombDamageable = TargetActor->Implements<UFCBombDamageable>();
	if (!bIsBombDamageable && !UFCCombatUtils::IsAttackableTarget(GetInstigator(), TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (const IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = TargetASI->GetAbilitySystemComponent();
	}

	UAbilitySystemComponent* SourceASC = nullptr;
	if (const IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(GetInstigator()))
	{
		SourceASC = SourceASI->GetAbilitySystemComponent();
	}

	TSubclassOf<UGameplayEffect> EffectiveDamageGE = DamageEffectClass ? DamageEffectClass : TSubclassOf<UGameplayEffect>(UFCGE_Damage::StaticClass());
	if (TargetASC && EffectiveDamageGE)
	{
		UAbilitySystemComponent* SpecCreatorASC = SourceASC ? SourceASC : TargetASC;
		FGameplayEffectContextHandle Context = SpecCreatorASC->MakeEffectContext();
		Context.AddInstigator(GetInstigator(), this);
		Context.AddHitResult(HitResult);

		if (FGameplayEffectSpecHandle SpecHandle = SpecCreatorASC->MakeOutgoingSpec(EffectiveDamageGE, 1.0f, Context); SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(FCGameplayTags::Data_Damage, Damage);
			if (AttackPowerScale > 0.0f)
			{
				SpecHandle.Data->SetSetByCallerMagnitude(FCGameplayTags::Data_Magnitude, AttackPowerScale);
			}
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else if (TargetASC)
	{
		if (const UFCAttributeSet* AttributeSet = TargetASC->GetSet<UFCAttributeSet>())
		{
			const_cast<UFCAttributeSet*>(AttributeSet)->ApplyIncomingDamageValue(Damage, GetInstigator(), this, &HitResult);
		}
	}
	else
	{
		UGameplayStatics::ApplyPointDamage(TargetActor, Damage, GetVelocity().GetSafeNormal(), HitResult, GetInstigatorController(), this, nullptr);

		if (bIsBombDamageable)
		{
			IFCBombDamageable::Execute_ReceiveBombDamage(TargetActor, Damage, HitResult.ImpactPoint, GetInstigator());
		}
	}

	// Execute per-target hit behaviors (e.g. explicit ApplyElementTags)
	bool bHasExplicitElementBehavior = false;
	for (const TInstancedStruct<FFCProjectileBehaviorBase>& Entry : Behaviors)
	{
		if (Entry.GetScriptStruct() && Entry.GetScriptStruct()->IsChildOf(FFCProjectileBehavior_ApplyElementTags::StaticStruct()))
		{
			bHasExplicitElementBehavior = true;
		}
		if (const FFCProjectileBehaviorBase* Behavior = Entry.GetPtr())
		{
			Behavior->OnProjectileHitActor(this, TargetActor, HitResult);
		}
	}

	// Dispatch class on-hit traits polymorphically via ClassData->TraitBehavior when no explicit element behavior overrode it
	if (!bHasExplicitElementBehavior && ProjectileElementTags.Num() > 0)
	{
		AActor* SourceActor = GetInstigator() ? Cast<AActor>(GetInstigator()) : Cast<AActor>(this);
		UFCCombatUtils::ApplyAttackCardHitTraits(SourceActor, TargetActor, SourceClassTag, SourceCardTypeTag, ProjectileElementTags);
	}
}

void AFCProjectileBase::PlayImpactCosmetics(const FVector& Location)
{
	if (GetNetMode() == NM_DedicatedServer || bHasPlayedCosmetics)
	{
		return;
	}

	if (!bPiercing || CurrentPierceCount >= MaxPierceCount)
	{
		bHasPlayedCosmetics = true;
	}

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
		APawn* InstigatorPawn = GetInstigator();
		if (InstigatorPawn && InstigatorPawn->IsLocallyControlled())
		{
			return;
		}

		PlayImpactCosmetics(Location);
	}
}
