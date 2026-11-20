#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FCWallBase.generated.h"

class UStaticMeshComponent;

/**
 * AFCWallBase
 *
 * Standard non-destructible solid wall actor spawned along exterior
 * boundaries, doorway flanking segments, doorway lintels, and corner pillars.
 */
UCLASS()
class FC_API AFCWallBase : public AActor
{
	GENERATED_BODY()

public:
	AFCWallBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UStaticMeshComponent* GetWallMeshComponent() const { return WallMesh; }

	/** Sets exact box dimensions (X=Thickness, Y=Length, Z=Height in cm) and bottom Z elevation. Server authoritative. */
	UFUNCTION(BlueprintCallable, Category = "FC|Wall")
	void SetWallDimensions(const FVector& InDimensions, float InBaseZOffset = 0.0f);

	UFUNCTION(BlueprintPure, Category = "FC|Wall")
	FVector GetWallDimensions() const { return WallDimensions; }

	UFUNCTION(BlueprintPure, Category = "FC|Wall")
	float GetWallBaseZOffset() const { return WallBaseZOffset; }

	/** Scales and aligns WallMesh to match WallDimensions and WallBaseZOffset using the mesh's local bounding box. */
	UFUNCTION(BlueprintCallable, Category = "FC|Wall")
	void ApplyWallDimensions();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnRep_WallDimensions();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> WallMesh;

	/** Desired mesh dimensions in cm (X=Thickness, Y=Length, Z=Height). Zero preserves Blueprint default scale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FC|Wall", ReplicatedUsing = OnRep_WallDimensions)
	FVector WallDimensions = FVector::ZeroVector;

	/** Local Z elevation of the bottom face of the wall mesh in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FC|Wall", ReplicatedUsing = OnRep_WallDimensions)
	float WallBaseZOffset = 0.0f;
};
