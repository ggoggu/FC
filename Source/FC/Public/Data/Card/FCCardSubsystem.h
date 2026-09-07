#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "FCCardSubsystem.generated.h"

class UFCCardDataAsset;
class UDataTable;
struct FStreamableHandle;

/**
 * UFCCardSubsystem
 * 
 * GameInstance Subsystem managing hybrid card catalog lookup, DataTable metadata caching,
 * and on-demand async asset streaming.
 * Runs on both Server and Client to resolve lightweight CardIds into full card definitions.
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

	/** Finds a Card Data Asset by its unique CardId (backward compatible adapter) */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	UFCCardDataAsset* GetCardDataAsset(FName CardId) const;

	/** High-speed, zero-allocation lookup of raw card table data */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	bool FindCardRow(FName CardId, FFCCardTableRow& OutRow) const;

	/** Retrieves all registered Card IDs in the catalog */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem")
	void GetAllCardIds(TArray<FName>& OutCardIds) const;

	/** Checks if a card ID exists in the catalog */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem")
	bool HasCard(FName CardId) const;

	/** Manually registers or overrides a Card Data Asset in the runtime catalog */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void RegisterCardDataAsset(UFCCardDataAsset* DataAsset);

	/** Manually registers or overrides a Card Table Row in the runtime catalog */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void RegisterCardRow(FName CardId, const FFCCardTableRow& InRow);

	/** Preloads all soft references (Abilities, Effects, Projectiles, Icons, VFX) for a list of CardIds asynchronously */
	TSharedPtr<FStreamableHandle> PreloadCardAssetsAsync(const TArray<FName>& CardIds, FSimpleDelegate OnComplete = FSimpleDelegate());

	/** Loads Card Catalog from DataTable and Asset Registry (zero-hitch) */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void LoadCardCatalog();

	/** Gets active Card Data Table */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem")
	UDataTable* GetCardDataTable() const { return CardDataTable; }

	/** Sets active Card Data Table and reloads catalog */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void SetCardDataTable(UDataTable* InDataTable);

protected:
	/** Populates built-in baseline cards into CachedCardRows to guarantee zero missing assets */
	void PopulateDefaultCatalog();

	/** Active Card DataTable (optional external source of truth) */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CardDataTable;

	/** High-speed in-memory metadata cache of all card definitions */
	UPROPERTY(Transient)
	TMap<FName, FFCCardTableRow> CachedCardRows;

	/** Map from CardId to instantiated/cached PrimaryDataAsset wrappers for backward compatibility */
	UPROPERTY(Transient)
	mutable TMap<FName, TObjectPtr<UFCCardDataAsset>> CardCatalog;
};
