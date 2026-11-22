#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FCDataTableSettings.generated.h"

class UDataTable;

/**
 * UFCDataTableSettings
 *
 * Project settings registering split DataTables for parallel, merge-conflict-free authoring
 * across cards, elements, classes, card keywords, and element reactions.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "FC DataTable Registry"))
class FC_API UFCDataTableSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFCDataTableSettings();

	/** Split Card Catalog DataTables (supports UDataTable and UCompositeDataTable, merged in order) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "FC|DataTables")
	TArray<TSoftObjectPtr<UDataTable>> CardDataTables;

	/** Element Definition DataTables (RowStruct: FFCElementDefinitionRow) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "FC|DataTables")
	TArray<TSoftObjectPtr<UDataTable>> ElementCatalogTables;

	/** Character Class Definition DataTables (RowStruct: FFCClassDefinitionRow) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "FC|DataTables")
	TArray<TSoftObjectPtr<UDataTable>> ClassCatalogTables;

	/** Card Keyword Definition DataTables (RowStruct: FFCCardKeywordRow) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "FC|DataTables")
	TArray<TSoftObjectPtr<UDataTable>> CardKeywordTables;

	/** Element Reaction Rule DataTables (RowStruct: FFCElementReactionRow) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "FC|DataTables")
	TArray<TSoftObjectPtr<UDataTable>> ElementReactionTables;

	virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
};
