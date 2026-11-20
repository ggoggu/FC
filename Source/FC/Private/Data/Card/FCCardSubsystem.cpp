#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/FCDataTableSettings.h"
#include "Data/FCGameplayTags.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"

static TWeakObjectPtr<UFCCardSubsystem> GLastActiveCardSubsystem = nullptr;

namespace
{
	const TMap<FGameplayTag, FFCCardKeywordRow>& GetBuiltinKeywordDefaults()
	{
		static TMap<FGameplayTag, FFCCardKeywordRow> Defaults;
		if (Defaults.Num() == 0)
		{
			auto AddDefault = [&](FGameplayTag Tag, const FText& Name, const FText& Desc, TInstancedStruct<FFCCardKeywordRuleBase> Rule)
			{
				FFCCardKeywordRow Row;
				Row.KeywordTag = Tag;
				Row.DisplayName = Name;
				Row.Description = Desc;
				Row.Rule = MoveTemp(Rule);
				Defaults.Add(Tag, MoveTemp(Row));
			};

			AddDefault(
				FCGameplayTags::Card_Keyword_Exhaust,
				NSLOCTEXT("FC_Card", "Keyword_Exhaust", "소멸"),
				NSLOCTEXT("FC_Card", "Keyword_Exhaust_Desc", "사용 후 소멸 더미로 이동합니다."),
				TInstancedStruct<FFCCardKeywordRuleBase>::Make<FFCCardKeywordRule_Exhaust>());

			AddDefault(
				FCGameplayTags::Card_Keyword_Retain,
				NSLOCTEXT("FC_Card", "Keyword_Retain", "보존"),
				NSLOCTEXT("FC_Card", "Keyword_Retain_Desc", "턴 사이클 종료 시 버리지 않고 손패에 유지됩니다."),
				TInstancedStruct<FFCCardKeywordRuleBase>::Make<FFCCardKeywordRule_Retain>());

			AddDefault(
				FCGameplayTags::Card_Keyword_Innate,
				NSLOCTEXT("FC_Card", "Keyword_Innate", "선천성"),
				NSLOCTEXT("FC_Card", "Keyword_Innate_Desc", "전투 시작 시 우선적으로 손패에 들어옵니다."),
				TInstancedStruct<FFCCardKeywordRuleBase>::Make<FFCCardKeywordRule_Innate>());

			AddDefault(
				FCGameplayTags::Card_Keyword_Ethereal,
				NSLOCTEXT("FC_Card", "Keyword_Ethereal", "휘발성"),
				NSLOCTEXT("FC_Card", "Keyword_Ethereal_Desc", "턴 사이클 종료 시 손패에 남아있으면 소멸합니다."),
				TInstancedStruct<FFCCardKeywordRuleBase>::Make<FFCCardKeywordRule_Ethereal>());

			AddDefault(
				FCGameplayTags::Card_Keyword_Unplayable,
				NSLOCTEXT("FC_Card", "Keyword_Unplayable", "사용 불가"),
				NSLOCTEXT("FC_Card", "Keyword_Unplayable_Desc", "직접 사용할 수 없는 카드입니다."),
				TInstancedStruct<FFCCardKeywordRuleBase>::Make<FFCCardKeywordRule_Unplayable>());
		}
		return Defaults;
	}
}

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
	AliasToCardIdMap.Empty();
	KeywordCatalog.Empty();
	CardDataTables.Empty();
	KeywordDataTables.Empty();
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

		if (GLastActiveCardSubsystem.IsValid())
		{
			return GLastActiveCardSubsystem.Get();
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

FName UFCCardSubsystem::ResolveCanonicalCardId(FName CardIdOrAlias) const
{
	if (CardIdOrAlias.IsNone())
	{
		return NAME_None;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCCardSubsystem*>(this)->LoadCardCatalog();
	}

	if (CachedCardRows.Contains(CardIdOrAlias))
	{
		return CardIdOrAlias;
	}

	if (const FName* Resolved = AliasToCardIdMap.Find(CardIdOrAlias))
	{
		return *Resolved;
	}

	const FString CardStr = CardIdOrAlias.ToString();
	if (CardStr.StartsWith(TEXT("DA_")))
	{
		const FName StrippedId(*CardStr.RightChop(3));
		if (CachedCardRows.Contains(StrippedId))
		{
			return StrippedId;
		}
		if (const FName* Resolved = AliasToCardIdMap.Find(StrippedId))
		{
			return *Resolved;
		}
	}

	if (CardStr.StartsWith(TEXT("Card_")))
	{
		const FName StrippedCard(*CardStr.RightChop(5));
		if (const FName* Resolved = AliasToCardIdMap.Find(StrippedCard))
		{
			return *Resolved;
		}
	}

	return NAME_None;
}

const FFCCardTableRow* UFCCardSubsystem::FindCardRowPtr(FName CardId) const
{
	if (CardId.IsNone())
	{
		return nullptr;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCCardSubsystem*>(this)->LoadCardCatalog();
	}

	if (const FFCCardTableRow* DirectRow = CachedCardRows.Find(CardId))
	{
		return DirectRow;
	}

	const FName CanonicalId = ResolveCanonicalCardId(CardId);
	if (!CanonicalId.IsNone())
	{
		return CachedCardRows.Find(CanonicalId);
	}

	return nullptr;
}

bool UFCCardSubsystem::FindCardRow(FName CardId, FFCCardTableRow& OutRow) const
{
	if (const FFCCardTableRow* FoundRow = FindCardRowPtr(CardId))
	{
		OutRow = *FoundRow;
		return true;
	}

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
		if (Pair.Value && !Pair.Value->GetCardId().IsNone())
		{
			UniqueKeys.Add(Pair.Value->GetCardId());
		}
		else
		{
			UniqueKeys.Add(Pair.Key);
		}
	}
	OutCardIds = UniqueKeys.Array();
}

bool UFCCardSubsystem::HasCard(FName CardId) const
{
	if (CardId.IsNone())
	{
		return false;
	}
	return FindCardRowPtr(CardId) != nullptr || CardCatalog.Contains(CardId);
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

	const FName CanonicalId = ResolveCanonicalCardId(CardId);
	const FName LookupKey = CanonicalId.IsNone() ? CardId : CanonicalId;

	if (const TObjectPtr<UFCCardDataAsset>* Found = CardCatalog.Find(LookupKey))
	{
		if (Found->Get())
		{
			return Found->Get();
		}
	}

	if (const FFCCardTableRow* FoundRow = FindCardRowPtr(LookupKey))
	{
		UFCCardDataAsset* NewAsset = NewObject<UFCCardDataAsset>(const_cast<UFCCardSubsystem*>(this));
		NewAsset->GameplayData = FoundRow->GameplayData;
		NewAsset->DisplayData = FoundRow->DisplayData;
		if (NewAsset->GameplayData.CardId.IsNone())
		{
			NewAsset->GameplayData.CardId = LookupKey;
		}

		const_cast<UFCCardSubsystem*>(this)->CardCatalog.Add(LookupKey, NewAsset);
		if (CardId != LookupKey)
		{
			const_cast<UFCCardSubsystem*>(this)->CardCatalog.Add(CardId, NewAsset);
		}
		return NewAsset;
	}

	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		FPrimaryAssetId AssetId(FPrimaryAssetType("Card"), LookupKey);
		UObject* LoadedObj = AssetManager->GetPrimaryAssetObject(AssetId);
		if (!LoadedObj)
		{
			const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(AssetId);
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
	}

	return nullptr;
}

void UFCCardSubsystem::RegisterCardDataAsset(UFCCardDataAsset* DataAsset)
{
	if (!DataAsset)
	{
		return;
	}

	if (!bCatalogLoaded)
	{
		LoadCardCatalog();
	}

	GLastActiveCardSubsystem = this;

	DataAsset->GameplayData.EnsureNormalizedPipeline();
	const FName CanonicalId = !DataAsset->GetCardId().IsNone() ? DataAsset->GetCardId() : DataAsset->GetFName();
	CardCatalog.Add(CanonicalId, DataAsset);
	if (DataAsset->GetFName() != CanonicalId)
	{
		AliasToCardIdMap.Add(DataAsset->GetFName(), CanonicalId);
	}

	FFCCardTableRow Row;
	Row.GameplayData = DataAsset->GameplayData;
	Row.DisplayData = DataAsset->DisplayData;
	if (Row.GameplayData.CardId.IsNone())
	{
		Row.GameplayData.CardId = CanonicalId;
	}

	CachedCardRows.Add(CanonicalId, Row);
	IndexCardAliases(CanonicalId, Row);
}

void UFCCardSubsystem::RegisterCardRow(FName CardId, const FFCCardTableRow& InRow)
{
	if (CardId.IsNone())
	{
		return;
	}

	GLastActiveCardSubsystem = this;

	FFCCardTableRow RowCopy = InRow;
	if (RowCopy.GameplayData.CardId.IsNone())
	{
		RowCopy.GameplayData.CardId = CardId;
	}
	RowCopy.GameplayData.EnsureNormalizedPipeline();

	CachedCardRows.Add(CardId, RowCopy);
	IndexCardAliases(CardId, RowCopy);

	if (TObjectPtr<UFCCardDataAsset>* Found = CardCatalog.Find(CardId))
	{
		if (Found->Get())
		{
			(*Found)->GameplayData = RowCopy.GameplayData;
			(*Found)->DisplayData = RowCopy.DisplayData;
		}
	}
}

void UFCCardSubsystem::RegisterKeywordRow(const FFCCardKeywordRow& InKeywordRow)
{
	if (!InKeywordRow.KeywordTag.IsValid())
	{
		return;
	}
	KeywordCatalog.Add(InKeywordRow.KeywordTag, InKeywordRow);
}

const FFCCardKeywordRow* UFCCardSubsystem::FindKeywordRowPtr(FGameplayTag KeywordTag) const
{
	if (!KeywordTag.IsValid())
	{
		return nullptr;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCCardSubsystem*>(this)->LoadCardCatalog();
	}

	if (const FFCCardKeywordRow* Found = KeywordCatalog.Find(KeywordTag))
	{
		return Found;
	}

	return GetBuiltinKeywordDefaults().Find(KeywordTag);
}

const FFCCardKeywordRuleBase* UFCCardSubsystem::FindKeywordRule(FGameplayTag KeywordTag) const
{
	if (const FFCCardKeywordRow* Row = FindKeywordRowPtr(KeywordTag))
	{
		return Row->Rule.GetPtr();
	}
	return nullptr;
}

FText UFCCardSubsystem::FormatKeywordTagsText(const FGameplayTagContainer& InCardTags, const UObject* WorldContextObject)
{
	if (InCardTags.IsEmpty())
	{
		return FText::GetEmpty();
	}

	const UFCCardSubsystem* Subsystem = GetCardSubsystem(WorldContextObject);
	TArray<FText> KeywordTexts;

	for (const FGameplayTag& Tag : InCardTags)
	{
		if (!Tag.MatchesTag(FCGameplayTags::Card_Keyword_Root) || Tag == FCGameplayTags::Card_Keyword_Root)
		{
			continue;
		}

		const FFCCardKeywordRow* Row = Subsystem ? Subsystem->FindKeywordRowPtr(Tag) : GetBuiltinKeywordDefaults().Find(Tag);
		if (Row && !Row->DisplayName.IsEmpty())
		{
			KeywordTexts.Add(Row->DisplayName);
		}
		else
		{
			KeywordTexts.Add(FText::FromName(Tag.GetTagName()));
		}
	}

	if (KeywordTexts.Num() == 0)
	{
		return FText::GetEmpty();
	}

	return FText::Join(FText::FromString(TEXT(", ")), KeywordTexts);
}

void UFCCardSubsystem::IndexCardAliases(FName CanonicalId, const FFCCardTableRow& Row)
{
	const FString CanonicalStr = CanonicalId.ToString();
	if (CanonicalStr.StartsWith(TEXT("Card_")))
	{
		AliasToCardIdMap.Add(FName(*(TEXT("DA_") + CanonicalStr)), CanonicalId);
	}

	for (const FName& Alias : Row.GameplayData.LegacyAliases)
	{
		if (!Alias.IsNone() && Alias != CanonicalId)
		{
			AliasToCardIdMap.Add(Alias, CanonicalId);
		}
	}

	const FString DisplayStr = Row.DisplayData.CardName.ToString();
	if (!DisplayStr.IsEmpty())
	{
		const FString DisplayNoSpaces = DisplayStr.Replace(TEXT(" "), TEXT(""));
		AliasToCardIdMap.Add(FName(*DisplayStr), CanonicalId);
		AliasToCardIdMap.Add(FName(*DisplayNoSpaces), CanonicalId);
		AliasToCardIdMap.Add(FName(*(TEXT("Card_") + DisplayStr)), CanonicalId);
		AliasToCardIdMap.Add(FName(*(TEXT("Card_") + DisplayNoSpaces)), CanonicalId);
	}
}

TSharedPtr<FStreamableHandle> UFCCardSubsystem::PreloadCardAssetsAsync(const TArray<FName>& CardIds, FSimpleDelegate OnComplete)
{
	TArray<FSoftObjectPath> AssetsToStream;

	for (const FName& CardId : CardIds)
	{
		if (const FFCCardTableRow* Row = FindCardRowPtr(CardId))
		{
			Row->GameplayData.GatherSoftAssetPaths(AssetsToStream);

			if (!Row->DisplayData.CardIcon.IsNull())
			{
				AssetsToStream.AddUnique(Row->DisplayData.CardIcon.ToSoftObjectPath());
			}
			if (!Row->DisplayData.CardFrame.IsNull())
			{
				AssetsToStream.AddUnique(Row->DisplayData.CardFrame.ToSoftObjectPath());
			}
			if (!Row->DisplayData.PlaySound.IsNull())
			{
				AssetsToStream.AddUnique(Row->DisplayData.PlaySound.ToSoftObjectPath());
			}
			if (!Row->DisplayData.PlayVFX.IsNull())
			{
				AssetsToStream.AddUnique(Row->DisplayData.PlayVFX.ToSoftObjectPath());
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
	GLastActiveCardSubsystem = this;
	CardDataTables.Reset();
	CachedCardRows.Empty();
	AliasToCardIdMap.Empty();
	CardCatalog.Empty();

	if (InDataTable)
	{
		CardDataTables.Add(InDataTable);
	}

	bCatalogLoaded = true;
	RegisterDefaultCoreKeywords();
	IngestDataTable(InDataTable);
}

void UFCCardSubsystem::AddCardDataTable(UDataTable* InDataTable)
{
	if (!InDataTable)
	{
		return;
	}

	if (!bCatalogLoaded)
	{
		LoadCardCatalog();
	}

	if (!CardDataTables.Contains(InDataTable))
	{
		CardDataTables.Add(InDataTable);
	}

	IngestDataTable(InDataTable);
}

void UFCCardSubsystem::AddKeywordDataTable(UDataTable* InKeywordTable)
{
	if (!InKeywordTable)
	{
		return;
	}

	if (!bCatalogLoaded)
	{
		LoadCardCatalog();
	}

	if (!KeywordDataTables.Contains(InKeywordTable))
	{
		KeywordDataTables.Add(InKeywordTable);
	}

	IngestKeywordDataTable(InKeywordTable);
}

void UFCCardSubsystem::IngestDataTable(UDataTable* Table)
{
	if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FFCCardTableRow::StaticStruct()))
	{
		return;
	}

	for (const auto& RowPair : Table->GetRowMap())
	{
		const FName RowName = RowPair.Key;
		const FFCCardTableRow* TableRow = reinterpret_cast<const FFCCardTableRow*>(RowPair.Value);
		if (!TableRow)
		{
			continue;
		}

		if (CachedCardRows.Contains(RowName))
		{
			UE_LOG(LogTemp, Warning, TEXT("[UFCCardSubsystem] Duplicate CardId '%s' detected while loading DataTable '%s'; overriding previous row."),
				*RowName.ToString(), *Table->GetName());
		}

		RegisterCardRow(RowName, *TableRow);
	}
}

void UFCCardSubsystem::IngestKeywordDataTable(UDataTable* Table)
{
	if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FFCCardKeywordRow::StaticStruct()))
	{
		return;
	}

	for (const auto& RowPair : Table->GetRowMap())
	{
		if (const FFCCardKeywordRow* KeywordRow = reinterpret_cast<const FFCCardKeywordRow*>(RowPair.Value))
		{
			RegisterKeywordRow(*KeywordRow);
		}
	}
}

void UFCCardSubsystem::RegisterDefaultCoreKeywords()
{
	for (const auto& Pair : GetBuiltinKeywordDefaults())
	{
		if (!KeywordCatalog.Contains(Pair.Key))
		{
			KeywordCatalog.Add(Pair.Key, Pair.Value);
		}
	}
}

bool UFCCardSubsystem::CompareCardDisplayOrder(
	EFCCardType LeftType, int32 LeftCost, const FText& LeftName, FName LeftId,
	EFCCardType RightType, int32 RightCost, const FText& RightName, FName RightId)
{
	if (LeftType != RightType)
	{
		return static_cast<uint8>(LeftType) < static_cast<uint8>(RightType);
	}

	if (LeftCost != RightCost)
	{
		return LeftCost < RightCost;
	}

	const FString LeftStr = LeftName.ToString();
	const FString RightStr = RightName.ToString();
	const int32 NameCmp = LeftStr.Compare(RightStr, ESearchCase::IgnoreCase);
	if (NameCmp != 0)
	{
		return NameCmp < 0;
	}

	return LeftId.LexicalLess(RightId);
}

void UFCCardSubsystem::SortCardIdsForDisplay(TArray<FName>& InOutCardIds) const
{
	InOutCardIds.Sort([this](const FName& A, const FName& B)
	{
		const FFCCardTableRow* RowA = FindCardRowPtr(A);
		const FFCCardTableRow* RowB = FindCardRowPtr(B);

		if (RowA && RowB)
		{
			const FText NameA = !RowA->DisplayData.CardName.IsEmpty() ? RowA->DisplayData.CardName : FText::FromName(A);
			const FText NameB = !RowB->DisplayData.CardName.IsEmpty() ? RowB->DisplayData.CardName : FText::FromName(B);
			return CompareCardDisplayOrder(
				RowA->GameplayData.CardType, RowA->GameplayData.BaseManaCost, NameA, A,
				RowB->GameplayData.CardType, RowB->GameplayData.BaseManaCost, NameB, B);
		}

		if ((RowA != nullptr) != (RowB != nullptr))
		{
			return RowA != nullptr;
		}

		return A.LexicalLess(B);
	});
}

void UFCCardSubsystem::LoadCardCatalog()
{
	bCatalogLoaded = true;
	RegisterDefaultCoreKeywords();

	// 1. Load configured split DataTables from UFCDataTableSettings
	if (const UFCDataTableSettings* Settings = GetDefault<UFCDataTableSettings>())
	{
		if (CardDataTables.Num() == 0)
		{
			for (const TSoftObjectPtr<UDataTable>& SoftTable : Settings->CardDataTables)
			{
				if (!SoftTable.IsNull())
				{
					if (UDataTable* LoadedTable = SoftTable.LoadSynchronous())
					{
						CardDataTables.AddUnique(LoadedTable);
					}
				}
			}
		}

		if (KeywordDataTables.Num() == 0)
		{
			for (const TSoftObjectPtr<UDataTable>& SoftKeywordTable : Settings->CardKeywordTables)
			{
				if (!SoftKeywordTable.IsNull())
				{
					if (UDataTable* LoadedTable = SoftKeywordTable.LoadSynchronous())
					{
						KeywordDataTables.AddUnique(LoadedTable);
					}
				}
			}
		}
	}

	if (CardDataTables.Num() == 0)
	{
		const TArray<FString> DataTableCandidatePaths = {
			TEXT("/Game/Data/Card/DT_CardCatalog.DT_CardCatalog"),
			TEXT("/Game/Card/DT_CardCatalog.DT_CardCatalog")
		};

		for (const FString& PathStr : DataTableCandidatePaths)
		{
			if (UDataTable* LoadedTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *PathStr)))
			{
				CardDataTables.AddUnique(LoadedTable);
				break;
			}
		}
	}

	for (UDataTable* KeywordTable : KeywordDataTables)
	{
		IngestKeywordDataTable(KeywordTable);
	}

	for (UDataTable* Table : CardDataTables)
	{
		IngestDataTable(Table);
	}

	// 2. AssetManager metadata scan
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
