#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "FCProjectileBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class USoundBase;
class UGameplayEffect;

/**
 * AFCProjectileBase
 * 
 * Extensible, server-authoritative projectile base actor supporting:
 * - Configurable projectile motion (speed, gravity scale, homing, bouncing)
 * - Configurable visual mesh (sphere, arrow, knife, bomb, etc.) and Niagara flight trail
 * - Configurable impact behaviors (direct damage, radial AoE explosion, piercing, spawn actor on impact)
 * - Soft/hard references for impact audio cues and particle effects
 */
UCLASS()
class FC_API AFCProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AFCProjectileBase();

	// --- Getters & Setters ---
	USphereComponent* GetCollisionComponent() const { return CollisionComponent; }
	UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	float GetDamage() const { return Damage; }
	void SetDamage(float InDamage) { Damage = InDamage; }

	float GetExplosionRadius() const { return ExplosionRadius; }
	void SetExplosionRadius(float InRadius) { ExplosionRadius = InRadius; }

	void SetDamageEffectClass(TSubclassOf<UGameplayEffect> InClass) { DamageEffectClass = InClass; }

protected:
	virtual void BeginPlay() override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FlightVFXComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// --- Gameplay & Impact Configuration ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay", meta = (ClampMin = "0.0"))
	float Damage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay")
	bool bPiercing = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay", meta = (ClampMin = "0"))
	int32 MaxPierceCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay")
	TSubclassOf<AActor> SpawnActorOnImpact;

	// --- Presentation (Audio & Visuals) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Presentation")
	TObjectPtr<USoundBase> FlightSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Presentation")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Presentation")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	// --- Collision Callbacks ---
	UFUNCTION()
	virtual void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Authoritative impact processor */
	virtual void ProcessImpact(AActor* OtherActor, const FHitResult& HitResult);

	/** Applies authoritative damage to target actor */
	virtual void ApplyDamageToActor(AActor* TargetActor, const FHitResult& HitResult);

	/** Plays cosmetic impact effects locally and multicasts to simulated proxies */
	virtual void PlayImpactCosmetics(const FVector& Location);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayImpactCosmetics(const FVector_NetQuantize& Location);

private:
	int32 CurrentPierceCount = 0;
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
