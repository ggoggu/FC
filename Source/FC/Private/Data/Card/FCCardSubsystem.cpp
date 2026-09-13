#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"

static TWeakObjectPtr<UFCCardSubsystem> GLastActiveCardSubsystem = nullptr;

void UFCCardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GLastActiveCardSubsystem = this;
	LoadCardCatalog();
}

void UFCCardSubsystem::Deinitialize()
{
	if (GLastActiveCardSubsystem.Get() == this)
	{
		GLastActiveCardSubsystem = nullptr;
	}
	CardCatalog.Empty();
	CachedCardRows.Empty();
	CardDataTable = nullptr;
	bCatalogLoaded = false;
	Super::Deinitialize();
}

UFCCardSubsystem* UFCCardSubsystem::GetCardSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return GLastActiveCardSubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GI)
	{
		for (const UObject* Curr = WorldContextObject; Curr; Curr = Curr->GetOuter())
		{
			if (const UGameInstance* OuterGI = Cast<UGameInstance>(Curr))
			{
				if (UFCCardSubsystem* Subsystem = const_cast<UGameInstance*>(OuterGI)->GetSubsystem<UFCCardSubsystem>())
				{
					return Subsystem;
				}
			}
			if (UFCCardSubsystem* DirectSubsystem = Cast<UFCCardSubsystem>(const_cast<UObject*>(Curr)))
			{
				return DirectSubsystem;
			}
		}

		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.OwningGameInstance)
				{
					if (UFCCardSubsystem* Subsystem = Context.OwningGameInstance->GetSubsystem<UFCCardSubsystem>())
					{
						return Subsystem;
					}
				}
			}
		}
	}

	UFCCardSubsystem* Found = GI ? GI->GetSubsystem<UFCCardSubsystem>() : nullptr;
	if (Found)
	{
		return Found;
	}

	return GLastActiveCardSubsystem.Get();
}

bool UFCCardSubsystem::FindCardRow(FName CardId, FFCCardTableRow& OutRow) const
{
	if (CardId.IsNone())
	{
		return false;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCCardSubsystem*>(this)->LoadCardCatalog();
	}

	// 1. Fast in-memory struct lookup
	if (const FFCCardTableRow* FoundRow = CachedCardRows.Find(CardId))
	{
		OutRow = *FoundRow;
		return true;
	}

	// Also check without or with DA_ prefix
	FString CardStr = CardId.ToString();
	if (CardStr.StartsWith(TEXT("DA_")))
	{
		FName StrippedId(*(CardStr.RightChop(3)));
		if (const FFCCardTableRow* FoundRow = CachedCardRows.Find(StrippedId))
		{
			OutRow = *FoundRow;
			return true;
		}
	}
	else
	{
		FName PrefixedId(*(TEXT("DA_") + CardStr));
		if (const FFCCardTableRow* FoundRow = CachedCardRows.Find(PrefixedId))
		{
			OutRow = *FoundRow;
			return true;
		}
	}

	// Check by localized / display name alias (e.g. Card_점화 -> 점화, Card_센드워 -> 센드 워)
	FString SearchName = CardStr;
	if (SearchName.StartsWith(TEXT("Card_")))
	{
		SearchName = SearchName.RightChop(5);
	}
	for (const auto& Pair : CachedCardRows)
	{
		FString DisplayNameNoSpaces = Pair.Value.DisplayData.CardName.ToString().Replace(TEXT(" "), TEXT(""));
		if (DisplayNameNoSpaces.Equals(SearchName, ESearchCase::IgnoreCase) ||
			Pair.Value.DisplayData.CardName.ToString().Equals(SearchName, ESearchCase::IgnoreCase))
		{
			OutRow = Pair.Value;
			return true;
		}
	}

	// 2. Check in-memory PrimaryDataAsset catalog
	if (const TObjectPtr<UFCCardDataAsset>* FoundAsset = CardCatalog.Find(CardId))
	{
		if (FoundAsset->Get())
		{
			OutRow.GameplayData = (*FoundAsset)->GameplayData;
			OutRow.DisplayData = (*FoundAsset)->DisplayData;
			return true;
		}
	}

	return false;
}

void UFCCardSubsystem::GetAllCardIds(TArray<FName>& OutCardIds) const
{
	if (!bCatalogLoaded)
	{
		const_cast<UFCCardSubsystem*>(this)->LoadCardCatalog();
	}

	OutCardIds.Reset();
	TSet<FName> UniqueKeys;
	CachedCardRows.GetKeys(UniqueKeys);
	for (const auto& Pair : CardCatalog)
	{
		UniqueKeys.Add(Pair.Key);
	}
	OutCardIds = UniqueKeys.Array();
}

bool UFCCardSubsystem::HasCard(FName CardId) const
{
	if (CardId.IsNone())
	{
		return false;
	}
	FFCCardTableRow DummyRow;
	return FindCardRow(CardId, DummyRow);
}

UFCCardDataAsset* UFCCardSubsystem::GetCardDataAsset(FName CardId) const
{
	if (CardId.IsNone())
	{
		return nullptr;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCCardSubsystem*>(this)->LoadCardCatalog();
	}

	// 1. Return already instantiated/cached DataAsset wrapper
	if (const TObjectPtr<UFCCardDataAsset>* Found = CardCatalog.Find(CardId))
	{
		if (Found->Get())
		{
			return Found->Get();
		}
	}

	// 2. On-demand instantiation from cached row (Zero-Hitch adapter)
	FFCCardTableRow FoundRow;
	if (FindCardRow(CardId, FoundRow))
	{
		UFCCardDataAsset* NewAsset = NewObject<UFCCardDataAsset>(const_cast<UFCCardSubsystem*>(this));
		NewAsset->GameplayData = FoundRow.GameplayData;
		NewAsset->DisplayData = FoundRow.DisplayData;
		if (NewAsset->GameplayData.CardId.IsNone())
		{
			NewAsset->GameplayData.CardId = CardId;
		}

		const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(NewAsset);
		return NewAsset;
	}

	// 3. Fallback: Standalone PrimaryDataAsset via AssetManager if exists
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		FPrimaryAssetId AssetId(FPrimaryAssetType("Card"), CardId);
		UObject* LoadedObj = AssetManager->GetPrimaryAssetObject(AssetId);
		if (!LoadedObj)
		{
			FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(AssetId);
			if (AssetPath.IsValid())
			{
				LoadedObj = AssetPath.TryLoad();
			}
		}

		if (UFCCardDataAsset* CardAsset = Cast<UFCCardDataAsset>(LoadedObj))
		{
			const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(CardAsset);
			return CardAsset;
		}

		// Also try DA_ prefix if queried without prefix (e.g. Card_Fireball -> DA_Card_Fireball)
		FString CardStr = CardId.ToString();
		if (!CardStr.StartsWith(TEXT("DA_")))
		{
			FPrimaryAssetId PrefixedAssetId(FPrimaryAssetType("Card"), FName(*(TEXT("DA_") + CardStr)));
			UObject* PrefixedObj = AssetManager->GetPrimaryAssetObject(PrefixedAssetId);
			if (!PrefixedObj)
			{
				FSoftObjectPath PrefixedPath = AssetManager->GetPrimaryAssetPath(PrefixedAssetId);
				if (PrefixedPath.IsValid())
				{
					PrefixedObj = PrefixedPath.TryLoad();
				}
			}

			if (UFCCardDataAsset* CardAsset = Cast<UFCCardDataAsset>(PrefixedObj))
			{
				const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(CardAsset);
				return CardAsset;
			}
		}
	}

	return nullptr;
}

void UFCCardSubsystem::RegisterCardDataAsset(UFCCardDataAsset* DataAsset)
{
	if (!DataAsset)
	{
		return;
	}

	GLastActiveCardSubsystem = this;

	FName CardId = DataAsset->GetCardId();
	if (!CardId.IsNone())
	{
		CardCatalog.Add(CardId, DataAsset);
	}
	CardCatalog.Add(DataAsset->GetFName(), DataAsset);

	// Also sync into CachedCardRows
	FFCCardTableRow Row;
	Row.GameplayData = DataAsset->GameplayData;
	Row.DisplayData = DataAsset->DisplayData;
	if (!CardId.IsNone())
	{
		CachedCardRows.Add(CardId, Row);
	}
	CachedCardRows.Add(DataAsset->GetFName(), Row);
}

void UFCCardSubsystem::RegisterCardRow(FName CardId, const FFCCardTableRow& InRow)
{
	if (CardId.IsNone())
	{
		return;
	}

	CachedCardRows.Add(CardId, InRow);
	// If a cached wrapper already exists, update its payload
	if (TObjectPtr<UFCCardDataAsset>* Found = CardCatalog.Find(CardId))
	{
		if (Found->Get())
		{
			(*Found)->GameplayData = InRow.GameplayData;
			(*Found)->DisplayData = InRow.DisplayData;
		}
	}
}

TSharedPtr<FStreamableHandle> UFCCardSubsystem::PreloadCardAssetsAsync(const TArray<FName>& CardIds, FSimpleDelegate OnComplete)
{
	TArray<FSoftObjectPath> AssetsToStream;

	for (const FName& CardId : CardIds)
	{
		FFCCardTableRow Row;
		if (FindCardRow(CardId, Row))
		{
			if (!Row.GameplayData.CardAbilityClass.IsNull())
			{
				AssetsToStream.AddUnique(Row.GameplayData.CardAbilityClass.ToSoftObjectPath());
			}
			for (const TSoftClassPtr<UGameplayEffect>& EffectClass : Row.GameplayData.CardEffectClasses)
			{
				if (!EffectClass.IsNull())
				{
					AssetsToStream.AddUnique(EffectClass.ToSoftObjectPath());
				}
			}
			if (!Row.GameplayData.ProjectileDataAsset.IsNull())
			{
				AssetsToStream.AddUnique(Row.GameplayData.ProjectileDataAsset.ToSoftObjectPath());
			}
			if (!Row.DisplayData.CardIcon.IsNull())
			{
				AssetsToStream.AddUnique(Row.DisplayData.CardIcon.ToSoftObjectPath());
			}
			if (!Row.DisplayData.CardFrame.IsNull())
			{
				AssetsToStream.AddUnique(Row.DisplayData.CardFrame.ToSoftObjectPath());
			}
			if (!Row.DisplayData.PlaySound.IsNull())
			{
				AssetsToStream.AddUnique(Row.DisplayData.PlaySound.ToSoftObjectPath());
			}
			if (!Row.DisplayData.PlayVFX.IsNull())
			{
				AssetsToStream.AddUnique(Row.DisplayData.PlayVFX.ToSoftObjectPath());
			}
		}
	}

	if (AssetsToStream.Num() == 0)
	{
		OnComplete.ExecuteIfBound();
		return nullptr;
	}

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	return Streamable.RequestAsyncLoad(AssetsToStream, FStreamableDelegate::CreateLambda([OnComplete]()
	{
		OnComplete.ExecuteIfBound();
	}));
}

void UFCCardSubsystem::SetCardDataTable(UDataTable* InDataTable)
{
	CardDataTable = InDataTable;
	bCatalogLoaded = false;
	LoadCardCatalog();
}

void UFCCardSubsystem::LoadCardCatalog()
{
	bCatalogLoaded = true;

	// 1. Load DataTable if not already bound
	if (!CardDataTable)
	{
		const TArray<FString> DataTableCandidatePaths = {
			TEXT("/Game/Data/Card/DT_CardCatalog.DT_CardCatalog"),
			TEXT("/Game/Card/DT_CardCatalog.DT_CardCatalog")
		};

		for (const FString& PathStr : DataTableCandidatePaths)
		{
			if (UDataTable* LoadedTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *PathStr)))
			{
				CardDataTable = LoadedTable;
				break;
			}
		}
	}

	// 2. Headless / Standalone Fallback: If uasset DataTable could not be loaded, load from project Content CSV
	if (!CardDataTable || CardDataTable->GetRowMap().Num() == 0)
	{
		const FString CsvPath = FPaths::ProjectContentDir() / TEXT("Card/DT_CardCatalog.csv");
		if (FPaths::FileExists(CsvPath))
		{
			FString CsvContent;
			if (FFileHelper::LoadFileToString(CsvContent, *CsvPath))
			{
				if (!CardDataTable)
				{
					CardDataTable = NewObject<UDataTable>(this, FName("DT_CardCatalog_Fallback"));
					CardDataTable->RowStruct = FFCCardTableRow::StaticStruct();
				}
				CardDataTable->CreateTableFromCSVString(CsvContent);
			}
		}
	}

	// 3. Register rows from DataTable
	if (CardDataTable)
	{
		for (const auto& RowPair : CardDataTable->GetRowMap())
		{
			const FName RowName = RowPair.Key;
			const FFCCardTableRow* TableRow = reinterpret_cast<const FFCCardTableRow*>(RowPair.Value);
			if (TableRow)
			{
				RegisterCardRow(RowName, *TableRow);
			}
		}
	}

	// 4. AssetManager metadata scan: Only register already-in-memory PrimaryDataAssets without blocking disk loads
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> AssetIdList;
		AssetManager->GetPrimaryAssetIdList(FPrimaryAssetType("Card"), AssetIdList);

		for (const FPrimaryAssetId& AssetId : AssetIdList)
		{
			if (UObject* LoadedObj = AssetManager->GetPrimaryAssetObject(AssetId))
			{
				if (UFCCardDataAsset* CardAsset = Cast<UFCCardDataAsset>(LoadedObj))
				{
					RegisterCardDataAsset(CardAsset);
				}
			}
		}
	}
}

