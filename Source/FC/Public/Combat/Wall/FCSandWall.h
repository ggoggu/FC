#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "FCSandWall.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UNiagaraSystem;
class USoundBase;

/**
 * AFCSandWall
 * 
 * Authoritative defensive sand barrier actor that absorbs incoming attacks (projectiles)
 * for a fixed duration (defaults to 1.0 second) before expiring.
 * Intercepts hostile projectiles, destroying them on impact to protect characters behind the wall.
 */
UCLASS()
class FC_API AFCSandWall : public AActor
{
	GENERATED_BODY()

public:
	AFCSandWall();

	/** Returns the primary collision component */
	UBoxComponent* GetCollisionComponent() const { return CollisionComponent; }

	/** Returns the visual mesh component */
	UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

	/** Configures wall lifetime (in seconds) */
	void SetWallDuration(float InDuration);

	/** Returns total number of attacks absorbed during lifetime */
	int32 GetAbsorbedAttackCount() const { return AbsorbedAttackCount; }

	/** Evaluates whether an attacker or incoming projectile can be absorbed */
	virtual bool CanAbsorbAttackFrom(const AActor* Attacker) const;

	/** Authoritative attack absorption handler */
	virtual void AbsorbIncomingAttack(AActor* AttackingActor, const FVector& ImpactLocation);

protected:
	virtual void BeginPlay() override;

	/** Root box collision providing physical blocking against projectiles */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SandWall|Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionComponent;

	/** Visual static mesh representing the sand barrier */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SandWall|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Ambient point light giving glowing earthen/sand feedback */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SandWall|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> SandLight;

	/** Duration before wall automatically expires (1.0s by default) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SandWall|Gameplay", meta = (ClampMin = "0.1"))
	float WallDuration = 1.0f;

	/** Sound cue played when absorbing an attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SandWall|Presentation")
	TObjectPtr<USoundBase> AbsorbSound;

	/** Particle system spawned on impact point when absorbing an attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SandWall|Presentation")
	TObjectPtr<UNiagaraSystem> AbsorbVFX;

	/** Collision hit callback */
	UFUNCTION()
	virtual void OnWallHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	/** Overlap callback to ensure high-speed projectiles are caught */
	UFUNCTION()
	virtual void OnWallBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	/** Multicasts cosmetic hit/absorb visual and audio feedback to all clients */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAbsorbCosmetics(const FVector_NetQuantize& ImpactLocation);

private:
	int32 AbsorbedAttackCount = 0;
	TSet<TWeakObjectPtr<AActor>> HandledAttackers;
};
