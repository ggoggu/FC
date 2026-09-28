#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FCWallBase.generated.h"

class UStaticMeshComponent;

/**
 * AFCWallBase
 * 
 * Standard non-destructible solid wall actor spawned along exterior
 * boundaries and non-connected room faces.
 */
UCLASS()
class FC_API AFCWallBase : public AActor
{
	GENERATED_BODY()

public:
	AFCWallBase();

	UStaticMeshComponent* GetWallMeshComponent() const { return WallMesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FC|Components")
	TObjectPtr<UStaticMeshComponent> WallMesh;
};
