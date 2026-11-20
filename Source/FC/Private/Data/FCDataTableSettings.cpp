#include "Data/FCDataTableSettings.h"
#include "Engine/DataTable.h"

UFCDataTableSettings::UFCDataTableSettings()
{
	CardDataTables.Add(TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Card/DT_CardCatalog.DT_CardCatalog"))));
	ElementCatalogTables.Add(TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_ElementCatalog.DT_ElementCatalog"))));
	ClassCatalogTables.Add(TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_ClassCatalog.DT_ClassCatalog"))));
	CardKeywordTables.Add(TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_CardKeywordCatalog.DT_CardKeywordCatalog"))));
	ElementReactionTables.Add(TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_ElementReactions.DT_ElementReactions"))));
}
