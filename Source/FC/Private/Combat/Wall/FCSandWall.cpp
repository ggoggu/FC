#include "Combat/Wall/FCSandWall.h"
#include "Combat/Projectile/FCProjectileBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AFCSandWall::AFCSandWall()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	// 1. Root Box Collision Component (Thickness 30cm, Width 240cm, Height 180cm)
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitBoxExtent(FVector(15.0f, 120.0f, 90.0f));
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // Let characters move smoothly without clipping
	RootComponent = CollisionComponent;

	// 2. Visual Mesh Component (Cube shape scaled to match the protective barrier)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMeshFinder.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.3f, 2.4f, 1.8f));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (BaseMatFinder.Succeeded())
	{
		MeshComponent->SetMaterial(0, BaseMatFinder.Object);
	}

	// 3. Glowing Earthen/Sand Point Light
	SandLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SandLight"));
	SandLight->SetupAttachment(RootComponent);
	SandLight->SetIntensity(2000.0f);
	SandLight->SetLightColor(FColor(245, 195, 90));
	SandLight->SetAttenuationRadius(400.0f);
	SandLight->bUseInverseSquaredFalloff = false;

	// 4. Default Duration: Exactly 1.0 second
	WallDuration = 1.0f;
	InitialLifeSpan = 1.0f;
}

void AFCSandWall::SetWallDuration(float InDuration)
{
	WallDuration = FMath::Max(0.1f, InDuration);
	SetLifeSpan(WallDuration);
}

void AFCSandWall::BeginPlay()
{
	Super::BeginPlay();

	// Authoritative lifetime management
	SetLifeSpan(WallDuration);

	// Setup visual dynamic material with warm sandy color
	if (MeshComponent)
	{
		if (UMaterialInstanceDynamic* DynMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0))
		{
			DynMat->SetVectorParameterValue(FName("Color"), FLinearColor(0.85f, 0.68f, 0.38f, 1.0f));
			DynMat->SetScalarParameterValue(FName("Roughness"), 0.95f);
		}
	}

	// Bind collision callbacks
	if (CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
		CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);

		CollisionComponent->OnComponentHit.AddDynamic(this, &AFCSandWall::OnWallHit);
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AFCSandWall::OnWallBeginOverlap);
	}
}

bool AFCSandWall::CanAbsorbAttackFrom(const AActor* Attacker) const
{
	if (!Attacker || Attacker == this)
	{
		return false;
	}

	// Cannot absorb attacks from the caster or friendly projectiles
	if (GetInstigator() && (Attacker == GetInstigator() || Attacker->GetInstigator() == GetInstigator()))
	{
		return false;
	}

	if (GetOwner() && (Attacker == GetOwner() || Attacker->GetOwner() == GetOwner()))
	{
		return false;
	}

	return true;
}

void AFCSandWall::AbsorbIncomingAttack(AActor* AttackingActor, const FVector& ImpactLocation)
{
	if (!CanAbsorbAttackFrom(AttackingActor))
	{
		return;
	}

	if (HandledAttackers.Contains(AttackingActor))
	{
		return;
	}
	HandledAttackers.Add(AttackingActor);

	AbsorbedAttackCount++;

	if (HasAuthority())
	{
		Multicast_PlayAbsorbCosmetics(ImpactLocation);

		// Safely destroy hostile projectile to absorb the attack
		if (IsValid(AttackingActor))
		{
			AttackingActor->Destroy();
		}
	}
}

void AFCSandWall::OnWallHit(
	UPrimitiveComponent* HitComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	const FVector ImpactLoc = Hit.ImpactPoint.IsZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
	AbsorbIncomingAttack(OtherActor, ImpactLoc);
}

void AFCSandWall::OnWallBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	const FVector ImpactLoc = SweepResult.ImpactPoint.IsZero() ? OtherActor->GetActorLocation() : FVector(SweepResult.ImpactPoint);
	AbsorbIncomingAttack(OtherActor, ImpactLoc);
}

void AFCSandWall::Multicast_PlayAbsorbCosmetics_Implementation(const FVector_NetQuantize& ImpactLocation)
{
	if (AbsorbSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AbsorbSound, ImpactLocation);
	}

	if (AbsorbVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, AbsorbVFX, ImpactLocation);
	}
}
