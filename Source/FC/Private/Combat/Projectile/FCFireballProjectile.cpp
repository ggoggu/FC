#include "Combat/Projectile/FCFireballProjectile.h"
#include "AbilitySystem/Effects/FCGE_Damage.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AFCFireballProjectile::AFCFireballProjectile()
{
	Damage = 1.0f;
	DamageEffectClass = UFCGE_Damage::StaticClass();
	ExplosionRadius = 0.0f;
	bPiercing = false;
	MaxPierceCount = 0;
	ProjectileElements = { EFCElement::Fire, EFCElement::Earth };
	SourceClass = EFCCharacterClass::Mage;
	SourceCardType = EFCCardType::Attack;

	if (CollisionComponent)
	{
		CollisionComponent->InitSphereRadius(20.0f);
	}

	// 1. Assign Visual Sphere Mesh (Radius 20cm matches Collision Sphere)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded() && MeshComponent)
	{
		MeshComponent->SetStaticMesh(SphereMeshFinder.Object);
		MeshComponent->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.4f));
	}

	// 2. Assign Fiery Emissive Material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMatFinder(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (EmissiveMatFinder.Succeeded() && MeshComponent)
	{
		MeshComponent->SetMaterial(0, EmissiveMatFinder.Object);
	}

	// 3. Dynamic Fire Point Light for illumination
	FireLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FireLight"));
	FireLight->SetupAttachment(RootComponent);
	FireLight->SetIntensity(3000.0f);
	FireLight->SetLightColor(FColor(255, 120, 30));
	FireLight->SetAttenuationRadius(500.0f);
	FireLight->bUseInverseSquaredFalloff = false;

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = 2500.0f;
		ProjectileMovement->MaxSpeed = 2500.0f;
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->bRotationFollowsVelocity = true;
		ProjectileMovement->bShouldBounce = false;
	}

	InitialLifeSpan = 5.0f;
}

void AFCFireballProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Dynamic fiery orange emissive glow
	if (MeshComponent)
	{
		if (UMaterialInstanceDynamic* DynMat = MeshComponent->CreateAndSetMaterialInstanceDynamic(0))
		{
			DynMat->SetVectorParameterValue(FName("Color"), FLinearColor(25.0f, 4.0f, 0.2f, 1.0f));
		}
	}
}

