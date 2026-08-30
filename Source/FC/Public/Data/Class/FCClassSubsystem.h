#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassSubsystem.generated.h"

class UFCClassDataAsset;

/**
 * UFCClassSubsystem
 * 
 * GameInstance Subsystem managing character class catalog lookup, caching, and fallback resolution.
 * Runs on both Server and Client to resolve class configurations and starter decks.
 */
UCLASS()
class FC_API UFCClassSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Static helper to get the class subsystem from any world context object */
	UFUNCTION(BlueprintPure, Category = "Class|Subsystem", meta = (WorldContext = "WorldContextObject"))
	static UFCClassSubsystem* GetClassSubsystem(const UObject* WorldContextObject);

	/** Finds a Class Data Asset by its class enum */
	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	UFCClassDataAsset* GetClassDataAsset(EFCCharacterClass ClassType) const;

	/** Retrieves default starter deck for a given class */
	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	TArray<FName> GetStartingDeckForClass(EFCCharacterClass ClassType) const;

	/** Retrieves affinity elements for a given class */
	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	TArray<EFCElement> GetAffinityElementsForClass(EFCCharacterClass ClassType) const;

	/** Manually registers or overrides a Class Data Asset in the runtime catalog */
	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	void RegisterClassDataAsset(UFCClassDataAsset* DataAsset);

	/** Scans Asset Manager for all PrimaryDataAssets of type CharacterClass */
	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	void LoadClassCatalog();

protected:
	/** Map from ClassType to loaded PrimaryDataAsset */
	UPROPERTY(Transient)
	TMap<EFCCharacterClass, TObjectPtr<UFCClassDataAsset>> ClassCatalog;
};
