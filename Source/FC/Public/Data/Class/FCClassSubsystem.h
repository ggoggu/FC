#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassSubsystem.generated.h"

class UFCClassDataAsset;
class UDataTable;

/**
 * UFCClassSubsystem
 *
 * Unified DataTable-backed GameInstanceSubsystem serving as the single SSOT for:
 * - Character Classes (DT_ClassCatalog -> FFCCharacterClassData + TInstancedStruct TraitBehavior)
 * - Elements (DT_ElementCatalog -> FFCElementDefinitionRow)
 * - Elemental Reactions (DT_ElementReactions -> FFCElementReactionRow)
 */
UCLASS()
class FC_API UFCClassSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Class|Subsystem", meta = (WorldContext = "WorldContextObject"))
	static UFCClassSubsystem* GetClassSubsystem(const UObject* WorldContextObject);

	// --- Class Catalog APIs (FGameplayTag) ---
	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	UFCClassDataAsset* GetClassDataAsset(FGameplayTag ClassTag) const;

	const FFCCharacterClassData* FindClassRowPtr(FGameplayTag ClassTag) const;

	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	TArray<FName> GetStartingDeckForClass(FGameplayTag ClassTag) const;

	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	TArray<FGameplayTag> GetAffinityElementsForClass(FGameplayTag ClassTag) const;

	UFUNCTION(BlueprintPure, Category = "Class|Subsystem")
	TArray<FGameplayTag> GetPlayableClassTags() const;

	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	void RegisterClassDataAsset(UFCClassDataAsset* DataAsset);

	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	void RegisterClassRow(const FFCCharacterClassData& InClassRow);

	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	void AddClassDataTable(UDataTable* InTable);

	// --- Element Catalog APIs (FGameplayTag) ---
	const FFCElementDefinitionRow* FindElementRowPtr(FGameplayTag ElementTag) const;

	UFUNCTION(BlueprintCallable, Category = "Element|Subsystem")
	void RegisterElementRow(const FFCElementDefinitionRow& InElementRow);

	UFUNCTION(BlueprintCallable, Category = "Element|Subsystem")
	void AddElementDataTable(UDataTable* InTable);

	// --- Element Reaction Registry APIs ---
	const TArray<FFCElementReactionRow>& GetElementReactionRules() const;

	UFUNCTION(BlueprintCallable, Category = "Element|Reaction")
	void RegisterElementReactionRow(const FFCElementReactionRow& InReactionRow);

	UFUNCTION(BlueprintCallable, Category = "Element|Reaction")
	void AddElementReactionDataTable(UDataTable* InTable);

	UFUNCTION(BlueprintCallable, Category = "Class|Subsystem")
	void LoadClassCatalog();

	// --- Built-in Default Accessors (used when subsystem has not yet initialized in pure static contexts) ---
	static const FFCCharacterClassData* FindBuiltinClassDefault(FGameplayTag ClassTag);
	static const FFCElementDefinitionRow* FindBuiltinElementDefault(FGameplayTag ElementTag);

protected:
	UPROPERTY(Transient)
	bool bCatalogLoaded = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDataTable>> ClassDataTables;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDataTable>> ElementDataTables;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDataTable>> ElementReactionTables;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FFCCharacterClassData> CachedClassRows;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FFCElementDefinitionRow> CachedElementRows;

	UPROPERTY(Transient)
	TArray<FFCElementReactionRow> CachedReactionRules;

	UPROPERTY(Transient)
	mutable TMap<FGameplayTag, TObjectPtr<UFCClassDataAsset>> ClassCatalog;

private:
	void RegisterBuiltinDefaults();
	void IngestClassDataTable(UDataTable* Table);
	void IngestElementDataTable(UDataTable* Table);
	void IngestReactionDataTable(UDataTable* Table);
	void SortReactionRules();
};
