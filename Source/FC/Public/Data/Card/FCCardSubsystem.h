#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Card/FCCardTypes.h"
#include "Card/Keywords/FCCardKeywordRules.h"
#include "FCCardSubsystem.generated.h"

class UFCCardDataAsset;
class UDataTable;
struct FStreamableHandle;

/**
 * UFCCardSubsystem
 *
 * GameInstance Subsystem managing split multi-DataTable card catalog lookup,
 * DataTable-driven keyword rule registry, zero-copy pointer caching,
 * data-driven alias resolution, and async asset streaming.
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

	/** Finds a Card Data Asset by its unique CardId or registered alias */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	UFCCardDataAsset* GetCardDataAsset(FName CardId) const;

	/** Copy-out lookup for Blueprint callers */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	bool FindCardRow(FName CardId, FFCCardTableRow& OutRow) const;

	/** Zero-copy pointer lookup for C++ hot paths (Server_PlayCard, sorting, ViewModel sync) */
	const FFCCardTableRow* FindCardRowPtr(FName CardId) const;

	/** Resolves an alias or DA_-prefixed name to its canonical CardId, or returns NAME_None if not found */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem")
	FName ResolveCanonicalCardId(FName CardIdOrAlias) const;

	/** Retrieves all registered canonical Card IDs in the catalog */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem")
	void GetAllCardIds(TArray<FName>& OutCardIds) const;

	/** Checks if a card ID or alias exists in the catalog */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem")
	bool HasCard(FName CardId) const;

	/** Manually registers or overrides a Card Data Asset in the runtime catalog */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void RegisterCardDataAsset(UFCCardDataAsset* DataAsset);

	/** Manually registers or overrides a Card Table Row in the runtime catalog */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void RegisterCardRow(FName CardId, const FFCCardTableRow& InRow);

	/** Registers or overrides a keyword definition row (DataTable-driven OCP keyword hooks) */
	UFUNCTION(BlueprintCallable, Category = "Card|Keywords")
	void RegisterKeywordRow(const FFCCardKeywordRow& InKeywordRow);

	/** Zero-copy lookup of a keyword definition row by GameplayTag */
	const FFCCardKeywordRow* FindKeywordRowPtr(FGameplayTag KeywordTag) const;

	/** Returns the TInstancedStruct keyword rule pointer for a GameplayTag, if registered */
	const FFCCardKeywordRuleBase* FindKeywordRule(FGameplayTag KeywordTag) const;

	/** Formats all Card.Keyword.* tags present in InCardTags using the Keyword DataTable registry */
	static FText FormatKeywordTagsText(const FGameplayTagContainer& InCardTags, const UObject* WorldContextObject = nullptr);

	/** Preloads all soft references (Abilities, Effects, Projectiles, InstancedStructs, Icons, VFX) asynchronously */
	TSharedPtr<FStreamableHandle> PreloadCardAssetsAsync(const TArray<FName>& CardIds, FSimpleDelegate OnComplete = FSimpleDelegate());

	/** Loads Card Catalog & Keyword Catalog from all configured split DataTables */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void LoadCardCatalog();

	/** Gets primary Card Data Table (first loaded table) */
	UFUNCTION(BlueprintPure, Category = "Card|Subsystem")
	UDataTable* GetCardDataTable() const { return CardDataTables.Num() > 0 ? CardDataTables[0].Get() : nullptr; }

	/** Gets all loaded split Card DataTables */
	const TArray<TObjectPtr<UDataTable>>& GetCardDataTables() const { return CardDataTables; }

	/** Replaces active Card DataTables with a single table and reloads catalog */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void SetCardDataTable(UDataTable* InDataTable);

	/** Appends an additional split Card DataTable into the runtime registry and merges its rows */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void AddCardDataTable(UDataTable* InDataTable);

	/** Appends a Keyword DataTable (RowStruct: FFCCardKeywordRow) into the keyword registry */
	UFUNCTION(BlueprintCallable, Category = "Card|Keywords")
	void AddKeywordDataTable(UDataTable* InKeywordTable);

	/** Canonical display comparator: CardType -> ManaCost -> DisplayName -> CardId */
	static bool CompareCardDisplayOrder(
		EFCCardType LeftType, int32 LeftCost, const FText& LeftName, FName LeftId,
		EFCCardType RightType, int32 RightCost, const FText& RightName, FName RightId);

	/** Sorts an array of CardIds using canonical display order (falls back to CardId if missing) */
	UFUNCTION(BlueprintCallable, Category = "Card|Subsystem")
	void SortCardIdsForDisplay(UPARAM(ref) TArray<FName>& InOutCardIds) const;

protected:
	UPROPERTY(Transient)
	bool bCatalogLoaded = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDataTable>> CardDataTables;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDataTable>> KeywordDataTables;

	UPROPERTY(Transient)
	TMap<FName, FFCCardTableRow> CachedCardRows;

	UPROPERTY(Transient)
	TMap<FName, FName> AliasToCardIdMap;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FFCCardKeywordRow> KeywordCatalog;

	UPROPERTY(Transient)
	mutable TMap<FName, TObjectPtr<UFCCardDataAsset>> CardCatalog;

private:
	void IngestDataTable(UDataTable* Table);
	void IngestKeywordDataTable(UDataTable* Table);
	void RegisterDefaultCoreKeywords();
	void IndexCardAliases(FName CanonicalId, const FFCCardTableRow& Row);
};
