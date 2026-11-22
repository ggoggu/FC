#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Data/Card/FCCardTypes.h"
#include "Combat/Interfaces/FCInterceptableAttack.h"
#include "Combat/Projectile/Behaviors/FCProjectileBehaviorBase.h"
#include "FCProjectileBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class USoundBase;
class UGameplayEffect;
class UFCProjectileDataAsset;

/**
 * AFCProjectileBase
 *
 * Extensible, server-authoritative projectile base actor implementing IFCInterceptableAttack
 * and composing polymorphic TInstancedStruct projectile behaviors (Explosion, Pierce, SpawnActor, Elements).
 */
UCLASS()
class FC_API AFCProjectileBase : public AActor, public IFCInterceptableAttack
{
	GENERATED_BODY()

public:
	AFCProjectileBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- IFCInterceptableAttack ---
	virtual FGameplayTagContainer GetAttackTags() const override;
	virtual bool CanBeInterceptedBy(const AActor* InterceptorActor) const override;
	virtual void OnInterceptedBy(AActor* InterceptorActor, const FVector& ImpactLocation) override;

	/** Initializes all motion, combat, behaviors, and presentation parameters from a data asset */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Data")
	virtual void InitializeFromDataAsset(const UFCProjectileDataAsset* InDataAsset);

	const UFCProjectileDataAsset* GetProjectileDataAsset() const { return ProjectileDataAsset; }
	void SetProjectileDataAsset(const UFCProjectileDataAsset* InDataAsset) { ProjectileDataAsset = InDataAsset; }

	// --- Getters & Setters ---
	USphereComponent* GetCollisionComponent() const { return CollisionComponent; }
	UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	float GetDamage() const { return Damage; }
	void SetDamage(float InDamage) { Damage = InDamage; }

	float GetAttackPowerScale() const { return AttackPowerScale; }
	void SetAttackPowerScale(float InScale) { AttackPowerScale = FMath::Max(0.0f, InScale); }

	float GetExplosionRadius() const { return ExplosionRadius; }
	void SetExplosionRadius(float InRadius) { ExplosionRadius = InRadius; }

	void SetDamageEffectClass(TSubclassOf<UGameplayEffect> InClass) { DamageEffectClass = InClass; }

	const TArray<FGameplayTag>& GetProjectileElementTags() const { return ProjectileElementTags; }
	void SetProjectileElementTags(const TArray<FGameplayTag>& InTags) { ProjectileElementTags = InTags; }

	FGameplayTag GetSourceClassTag() const { return SourceClassTag; }
	void SetSourceClassTag(FGameplayTag InClassTag) { SourceClassTag = InClassTag; }

	FGameplayTag GetSourceCardTypeTag() const { return SourceCardTypeTag; }
	void SetSourceCardTypeTag(FGameplayTag InTypeTag) { SourceCardTypeTag = InTypeTag; }

	void AddProjectileBehavior(const TInstancedStruct<FFCProjectileBehaviorBase>& InBehavior) { Behaviors.Add(InBehavior); }
	void SetAttackTags(const FGameplayTagContainer& InTags) { AttackTags = InTags; }

	int32 GetCurrentPierceCount() const { return CurrentPierceCount; }
	void IncrementPierceCount() { CurrentPierceCount++; }

	/** Applies authoritative damage via GAS ExecCalc_Damage -> IncomingDamage and triggers hit behaviors */
	virtual void ApplyDamageToActor(AActor* TargetActor, const FHitResult& HitResult);

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

	// --- Data-Driven Configuration ---
	UPROPERTY(ReplicatedUsing = OnRep_ProjectileDataAsset, EditAnywhere, BlueprintReadWrite, Category = "Projectile|Data")
	TObjectPtr<const UFCProjectileDataAsset> ProjectileDataAsset;

	UFUNCTION()
	void OnRep_ProjectileDataAsset();

	// --- Gameplay & Impact Configuration ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay", meta = (ClampMin = "0.0"))
	float Damage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay", meta = (ClampMin = "0.0"))
	float AttackPowerScale = 1.0f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay")
	TArray<FGameplayTag> ProjectileElementTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay")
	FGameplayTag SourceClassTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay")
	FGameplayTag SourceCardTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Gameplay")
	FGameplayTagContainer AttackTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Behaviors", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFCProjectileBehaviorBase>> Behaviors;

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

	virtual void Destroyed() override;

	/** Authoritative impact processor executing composable TInstancedStruct Behaviors */
	virtual void ProcessImpact(AActor* OtherActor, const FHitResult& HitResult);

	/** Plays cosmetic impact effects locally and multicasts to simulated proxies */
	virtual void PlayImpactCosmetics(const FVector& Location);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayImpactCosmetics(const FVector_NetQuantize& Location);

private:
	void EnsureNormalizedBehaviors();

	int32 CurrentPierceCount = 0;
	bool bHasPlayedCosmetics = false;
	bool bInitializedFromDataAsset = false;
	bool bBehaviorsNormalized = false;
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
