#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FCCardSubsystem.generated.h"

class UFCCardDataAsset;

/**
 * UFCCardSubsystem
 * 
 * GameInstance Subsystem managing card catalog lookup, caching, and registry resolution.
 * Runs on both Server and Client to resolve lightweight CardIds into full CardDataAssets.
 */
UCLASS()
class FC_API UFCCardSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Static helper to get the subsystem from any world context object */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem", meta = (WorldContext = "WorldContextObject"))
	static UFCCardSubsystem* GetCardSubsystem(const UObject* WorldContextObject);

	/** Finds a Card Data Asset by its unique CardId */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	UFCCardDataAsset* GetCardDataAsset(FName CardId) const;

	/** Manually registers or overrides a Card Data Asset in the runtime catalog */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void RegisterCardDataAsset(UFCCardDataAsset* DataAsset);

	/** Scans Asset Manager / Registry for all PrimaryDataAssets of type Card */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void LoadCardCatalog();

protected:
	/** Map from CardId to loaded PrimaryDataAsset */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UFCCardDataAsset>> CardCatalog;
};
