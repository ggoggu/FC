#include "Data/Class/FCClassSubsystem.h"
#include "Data/Class/FCClassDataAsset.h"
#include "Data/Class/Traits/FCClassTraitBehaviors.h"
#include "Data/FCDataTableSettings.h"
#include "Data/FCGameplayTags.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

static TWeakObjectPtr<UFCClassSubsystem> GLastActiveClassSubsystem = nullptr;

namespace
{
	const TMap<FGameplayTag, FFCElementDefinitionRow>& GetBuiltinElementMap()
	{
		static TMap<FGameplayTag, FFCElementDefinitionRow> Map;
		if (Map.Num() == 0)
		{
			auto AddElem = [&](FGameplayTag Tag, const FText& Name, const TCHAR* Code, const FLinearColor& Color)
			{
				FFCElementDefinitionRow Row;
				Row.ElementTag = Tag;
				Row.DisplayName = Name;
				Row.ShortCode = Code;
				Row.Color = Color;
				Map.Add(Tag, MoveTemp(Row));
			};

			AddElem(FCGameplayTags::Element_None,      NSLOCTEXT("FCElement", "ElementNone", "무속성"),      TEXT("-"), FLinearColor(0.5f, 0.5f, 0.5f, 0.5f));
			AddElem(FCGameplayTags::Element_Fire,      NSLOCTEXT("FCElement", "ElementFire", "화염"),        TEXT("F"), FLinearColor(1.0f, 0.28f, 0.08f, 1.0f));
			AddElem(FCGameplayTags::Element_Earth,     NSLOCTEXT("FCElement", "ElementEarth", "대지"),       TEXT("E"), FLinearColor(0.85f, 0.55f, 0.15f, 1.0f));
			AddElem(FCGameplayTags::Element_Water,     NSLOCTEXT("FCElement", "ElementWater", "빙결/물"),    TEXT("W"), FLinearColor(0.12f, 0.65f, 1.0f, 1.0f));
			AddElem(FCGameplayTags::Element_Wind,      NSLOCTEXT("FCElement", "ElementWind", "바람"),        TEXT("A"), FLinearColor(0.2f, 0.9f, 0.5f, 1.0f));
			AddElem(FCGameplayTags::Element_Lightning, NSLOCTEXT("FCElement", "ElementLightning", "번개"),   TEXT("L"), FLinearColor(1.0f, 0.88f, 0.15f, 1.0f));
			AddElem(FCGameplayTags::Element_Holy,      NSLOCTEXT("FCElement", "ElementHoly", "신성"),        TEXT("H"), FLinearColor(1.0f, 0.95f, 0.65f, 1.0f));
			AddElem(FCGameplayTags::Element_Dark,      NSLOCTEXT("FCElement", "ElementDark", "암흑"),        TEXT("D"), FLinearColor(0.68f, 0.25f, 0.95f, 1.0f));
		}
		return Map;
	}

	const TMap<FGameplayTag, FFCCharacterClassData>& GetBuiltinClassMap()
	{
		static TMap<FGameplayTag, FFCCharacterClassData> Map;
		if (Map.Num() == 0)
		{
			const TArray<FName> DefaultMageDeck = {
				FName("Card_Fireball"), FName("Card_Fireball"), FName("Card_Fireball"), FName("Card_Fireball"), FName("Card_Fireball"),
				FName("Card_AttackBuff"), FName("Card_AttackBuff"), FName("Card_AttackBuff"),
				FName("Card_MagicShield"), FName("Card_MagicShield")
			};

			const TArray<FName> DefaultNeutralDeck = {
				FName("Card_Fireball"), FName("Card_Fireball"), FName("Card_Fireball"), FName("Card_Fireball"), FName("Card_Fireball"),
				FName("Card_AttackBuff"), FName("Card_AttackBuff"), FName("Card_AttackBuff"), FName("Card_AttackBuff"), FName("Card_AttackBuff")
			};

			// Neutral
			{
				FFCCharacterClassData Row;
				Row.ClassTag = FCGameplayTags::Class_Neutral;
				Row.ClassDisplayName = NSLOCTEXT("FCClass", "ClassNeutral", "중립");
				Row.ClassDescription = NSLOCTEXT("FCClass", "ClassNeutralDesc", "모든 직업이 공용으로 사용할 수 있는 기본 설정입니다.");
				Row.TraitCategoryName = NSLOCTEXT("FCClassTrait", "TraitCategoryNeutral", "공용");
				Row.bIsSelectable = false;
				Row.bShowInClassSelect = false;
				Row.DisplayOrder = 99;
				Row.StartingDeck = DefaultNeutralDeck;
				Row.BaseMaxHealth = 100.0f;
				Row.BaseMaxMana = 50.0f;
				Row.BaseAttackPower = 0.0f;
				Row.TraitBehavior = TInstancedStruct<FFCClassTraitBehaviorBase>::Make<FFCClassTrait_Default>();
				Map.Add(Row.ClassTag, MoveTemp(Row));
			}

			// Mage
			{
				FFCCharacterClassData Row;
				Row.ClassTag = FCGameplayTags::Class_Mage;
				Row.ClassDisplayName = NSLOCTEXT("FCClass", "ClassMage", "마법사");
				Row.ClassDescription = NSLOCTEXT("FCClass", "ClassMageDesc", "강력한 원소 마법을 다루는 주문 시전자입니다.");
				Row.TraitCategoryName = NSLOCTEXT("FCClassTrait", "TraitCategoryMage", "속성");
				Row.bIsSelectable = true;
				Row.bShowInClassSelect = true;
				Row.DisplayOrder = 0;
				Row.AffinityElementTags = {
					FCGameplayTags::Element_Fire,
					FCGameplayTags::Element_Earth,
					FCGameplayTags::Element_Water,
					FCGameplayTags::Element_Wind,
					FCGameplayTags::Element_Lightning
				};
				Row.StartingDeck = DefaultMageDeck;
				Row.BaseMaxHealth = 80.0f;
				Row.BaseMaxMana = 100.0f;
				Row.BaseAttackPower = 0.0f;
				Row.ClassTags.AddTag(FCGameplayTags::Class_Mage);
				Row.TraitBehavior = TInstancedStruct<FFCClassTraitBehaviorBase>::Make<FFCClassTrait_Mage>();
				Map.Add(Row.ClassTag, MoveTemp(Row));
			}

			// Warrior
			{
				FFCCharacterClassData Row;
				Row.ClassTag = FCGameplayTags::Class_Warrior;
				Row.ClassDisplayName = NSLOCTEXT("FCClass", "ClassWarrior", "전사");
				Row.ClassDescription = NSLOCTEXT("FCClass", "WarriorDefaultDesc", "단단한 방어구와 근접 무기로 전열을 지키는 굳건한 투사입니다.");
				Row.TraitCategoryName = NSLOCTEXT("FCClassTrait", "TraitCategoryWarrior", "태세");
				Row.bIsSelectable = false;
				Row.LockReasonText = NSLOCTEXT("FCClassSelect", "ClassLocked", "준비 중 (현재 마법사만 선택 가능)");
				Row.bShowInClassSelect = true;
				Row.DisplayOrder = 1;
				Row.StartingDeck = DefaultNeutralDeck;
				Row.BaseMaxHealth = 150.0f;
				Row.BaseMaxMana = 30.0f;
				Row.BaseAttackPower = 15.0f;
				Row.ClassTags.AddTag(FCGameplayTags::Class_Warrior);
				Row.TraitBehavior = TInstancedStruct<FFCClassTraitBehaviorBase>::Make<FFCClassTrait_Default>();
				Map.Add(Row.ClassTag, MoveTemp(Row));
			}

			// Rogue
			{
				FFCCharacterClassData Row;
				Row.ClassTag = FCGameplayTags::Class_Rogue;
				Row.ClassDisplayName = NSLOCTEXT("FCClass", "ClassRogue", "도적");
				Row.ClassDescription = NSLOCTEXT("FCClass", "RogueDefaultDesc", "민첩한 몸놀림과 급소 공격으로 적을 기습하는 암살자입니다.");
				Row.TraitCategoryName = NSLOCTEXT("FCClassTrait", "TraitCategoryRogue", "연계");
				Row.bIsSelectable = false;
				Row.LockReasonText = NSLOCTEXT("FCClassSelect", "ClassLocked", "준비 중 (현재 마법사만 선택 가능)");
				Row.bShowInClassSelect = true;
				Row.DisplayOrder = 2;
				Row.StartingDeck = DefaultNeutralDeck;
				Row.BaseMaxHealth = 90.0f;
				Row.BaseMaxMana = 40.0f;
				Row.BaseAttackPower = 20.0f;
				Row.ClassTags.AddTag(FCGameplayTags::Class_Rogue);
				Row.TraitBehavior = TInstancedStruct<FFCClassTraitBehaviorBase>::Make<FFCClassTrait_Default>();
				Map.Add(Row.ClassTag, MoveTemp(Row));
			}

			// Priest
			{
				FFCCharacterClassData Row;
				Row.ClassTag = FCGameplayTags::Class_Priest;
				Row.ClassDisplayName = NSLOCTEXT("FCClass", "ClassPriest", "사제");
				Row.ClassDescription = NSLOCTEXT("FCClass", "PriestDefaultDesc", "신성한 권능으로 아군을 보호하고 치유하는 성직자입니다.");
				Row.TraitCategoryName = NSLOCTEXT("FCClassTrait", "TraitCategoryPriest", "권능");
				Row.bIsSelectable = false;
				Row.LockReasonText = NSLOCTEXT("FCClassSelect", "ClassLocked", "준비 중 (현재 마법사만 선택 가능)");
				Row.bShowInClassSelect = true;
				Row.DisplayOrder = 3;
				Row.StartingDeck = DefaultNeutralDeck;
				Row.BaseMaxHealth = 100.0f;
				Row.BaseMaxMana = 80.0f;
				Row.BaseAttackPower = 5.0f;
				Row.ClassTags.AddTag(FCGameplayTags::Class_Priest);
				Row.TraitBehavior = TInstancedStruct<FFCClassTraitBehaviorBase>::Make<FFCClassTrait_Default>();
				Map.Add(Row.ClassTag, MoveTemp(Row));
			}
		}
		return Map;
	}
}

// =========================================================================
// FCClassTraitUtils (Zero-Switch DataTable Facade)
// =========================================================================

FText FCClassTraitUtils::GetClassDisplayName(FGameplayTag ClassTag, const UObject* WorldContextObject)
{
	const UFCClassSubsystem* Subsystem = UFCClassSubsystem::GetClassSubsystem(WorldContextObject);
	const FFCCharacterClassData* Row = Subsystem ? Subsystem->FindClassRowPtr(ClassTag) : UFCClassSubsystem::FindBuiltinClassDefault(ClassTag);
	return Row ? Row->ClassDisplayName : FText::GetEmpty();
}

FText FCClassTraitUtils::GetElementDisplayName(FGameplayTag ElementTag, const UObject* WorldContextObject)
{
	const UFCClassSubsystem* Subsystem = UFCClassSubsystem::GetClassSubsystem(WorldContextObject);
	const FFCElementDefinitionRow* Row = Subsystem ? Subsystem->FindElementRowPtr(ElementTag) : UFCClassSubsystem::FindBuiltinElementDefault(ElementTag);
	if (Row)
	{
		return Row->DisplayName;
	}
	const FFCElementDefinitionRow* NoneRow = UFCClassSubsystem::FindBuiltinElementDefault(FCGameplayTags::Element_None);
	return NoneRow ? NoneRow->DisplayName : FText::GetEmpty();
}

FString FCClassTraitUtils::GetElementShortCode(FGameplayTag ElementTag, const UObject* WorldContextObject)
{
	const UFCClassSubsystem* Subsystem = UFCClassSubsystem::GetClassSubsystem(WorldContextObject);
	const FFCElementDefinitionRow* Row = Subsystem ? Subsystem->FindElementRowPtr(ElementTag) : UFCClassSubsystem::FindBuiltinElementDefault(ElementTag);
	return Row ? Row->ShortCode : TEXT("-");
}

FLinearColor FCClassTraitUtils::GetElementColor(FGameplayTag ElementTag, const UObject* WorldContextObject)
{
	const UFCClassSubsystem* Subsystem = UFCClassSubsystem::GetClassSubsystem(WorldContextObject);
	const FFCElementDefinitionRow* Row = Subsystem ? Subsystem->FindElementRowPtr(ElementTag) : UFCClassSubsystem::FindBuiltinElementDefault(ElementTag);
	return Row ? Row->Color : FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
}

FText FCClassTraitUtils::GetTraitCategoryName(FGameplayTag ClassTag, const UObject* WorldContextObject)
{
	const UFCClassSubsystem* Subsystem = UFCClassSubsystem::GetClassSubsystem(WorldContextObject);
	const FFCCharacterClassData* Row = Subsystem ? Subsystem->FindClassRowPtr(ClassTag) : UFCClassSubsystem::FindBuiltinClassDefault(ClassTag);
	if (Row && !Row->TraitCategoryName.IsEmpty())
	{
		return Row->TraitCategoryName;
	}
	return NSLOCTEXT("FCClassTrait", "TraitCategoryNeutral", "공용");
}

FText FCClassTraitUtils::FormatElementTagsText(const TArray<FGameplayTag>& ElementTags, const UObject* WorldContextObject)
{
	TArray<FString> ElementStrings;
	for (const FGameplayTag& ElemTag : ElementTags)
	{
		if (ElemTag.IsValid() && ElemTag != FCGameplayTags::Element_None)
		{
			ElementStrings.Add(GetElementDisplayName(ElemTag, WorldContextObject).ToString());
		}
	}

	if (ElementStrings.Num() == 0)
	{
		if (ElementTags.Contains(FCGameplayTags::Element_None))
		{
			return GetElementDisplayName(FCGameplayTags::Element_None, WorldContextObject);
		}
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Join(ElementStrings, TEXT(" / ")));
}

bool FCClassTraitUtils::CanCardBeUsedByClass(FGameplayTag CardClassTag, FGameplayTag PlayerClassTag)
{
	if (!CardClassTag.IsValid() || CardClassTag == FCGameplayTags::Class_Neutral)
	{
		return true;
	}
	return CardClassTag == PlayerClassTag;
}

bool FCClassTraitUtils::SupportsElementalAffinities(FGameplayTag ClassTag, const UObject* WorldContextObject)
{
	const UFCClassSubsystem* Subsystem = UFCClassSubsystem::GetClassSubsystem(WorldContextObject);
	const FFCCharacterClassData* Row = Subsystem ? Subsystem->FindClassRowPtr(ClassTag) : UFCClassSubsystem::FindBuiltinClassDefault(ClassTag);
	if (Row)
	{
		if (const FFCClassTraitBehaviorBase* Behavior = Row->TraitBehavior.GetPtr())
		{
			return Behavior->SupportsElements();
		}
		return Row->AffinityElementTags.Num() > 0;
	}
	return false;
}

// =========================================================================
// UFCClassSubsystem Implementation
// =========================================================================

void UFCClassSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GLastActiveClassSubsystem = this;
	LoadClassCatalog();
}

void UFCClassSubsystem::Deinitialize()
{
	if (GLastActiveClassSubsystem.Get() == this)
	{
		GLastActiveClassSubsystem = nullptr;
	}
	ClassCatalog.Empty();
	CachedClassRows.Empty();
	CachedElementRows.Empty();
	CachedReactionRules.Empty();
	ClassDataTables.Empty();
	ElementDataTables.Empty();
	ElementReactionTables.Empty();
	bCatalogLoaded = false;
	Super::Deinitialize();
}

UFCClassSubsystem* UFCClassSubsystem::GetClassSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return GLastActiveClassSubsystem.Get();
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GI)
	{
		for (const UObject* Curr = WorldContextObject; Curr; Curr = Curr->GetOuter())
		{
			if (const UGameInstance* OuterGI = Cast<UGameInstance>(Curr))
			{
				if (UFCClassSubsystem* Sub = const_cast<UGameInstance*>(OuterGI)->GetSubsystem<UFCClassSubsystem>())
				{
					return Sub;
				}
			}
			if (UFCClassSubsystem* Direct = Cast<UFCClassSubsystem>(const_cast<UObject*>(Curr)))
			{
				return Direct;
			}
		}

		if (GLastActiveClassSubsystem.IsValid())
		{
			return GLastActiveClassSubsystem.Get();
		}

		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.OwningGameInstance)
				{
					if (UFCClassSubsystem* Sub = Context.OwningGameInstance->GetSubsystem<UFCClassSubsystem>())
					{
						return Sub;
					}
				}
			}
		}
	}

	if (UFCClassSubsystem* Found = GI ? GI->GetSubsystem<UFCClassSubsystem>() : nullptr)
	{
		return Found;
	}

	return GLastActiveClassSubsystem.Get();
}

const FFCCharacterClassData* UFCClassSubsystem::FindBuiltinClassDefault(FGameplayTag ClassTag)
{
	return GetBuiltinClassMap().Find(ClassTag);
}

const FFCElementDefinitionRow* UFCClassSubsystem::FindBuiltinElementDefault(FGameplayTag ElementTag)
{
	return GetBuiltinElementMap().Find(ElementTag);
}

const FFCCharacterClassData* UFCClassSubsystem::FindClassRowPtr(FGameplayTag ClassTag) const
{
	if (!ClassTag.IsValid())
	{
		return nullptr;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCClassSubsystem*>(this)->LoadClassCatalog();
	}

	if (const FFCCharacterClassData* Found = CachedClassRows.Find(ClassTag))
	{
		return Found;
	}

	return FindBuiltinClassDefault(ClassTag);
}

UFCClassDataAsset* UFCClassSubsystem::GetClassDataAsset(FGameplayTag ClassTag) const
{
	if (!ClassTag.IsValid())
	{
		return nullptr;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCClassSubsystem*>(this)->LoadClassCatalog();
	}

	if (const TObjectPtr<UFCClassDataAsset>* Found = ClassCatalog.Find(ClassTag))
	{
		if (Found->Get())
		{
			return Found->Get();
		}
	}

	if (const FFCCharacterClassData* Row = FindClassRowPtr(ClassTag))
	{
		UFCClassDataAsset* Wrapper = NewObject<UFCClassDataAsset>(const_cast<UFCClassSubsystem*>(this));
		Wrapper->ClassData = *Row;
		const_cast<UFCClassSubsystem*>(this)->ClassCatalog.Add(ClassTag, Wrapper);
		return Wrapper;
	}

	return nullptr;
}

TArray<FName> UFCClassSubsystem::GetStartingDeckForClass(FGameplayTag ClassTag) const
{
	if (const FFCCharacterClassData* Row = FindClassRowPtr(ClassTag))
	{
		if (Row->StartingDeck.Num() > 0)
		{
			return Row->StartingDeck;
		}
	}

	if (const FFCCharacterClassData* NeutralRow = FindClassRowPtr(FCGameplayTags::Class_Neutral))
	{
		return NeutralRow->StartingDeck;
	}

	return {};
}

TArray<FGameplayTag> UFCClassSubsystem::GetAffinityElementsForClass(FGameplayTag ClassTag) const
{
	if (const FFCCharacterClassData* Row = FindClassRowPtr(ClassTag))
	{
		return Row->AffinityElementTags;
	}
	return {};
}

TArray<FGameplayTag> UFCClassSubsystem::GetPlayableClassTags() const
{
	if (!bCatalogLoaded)
	{
		const_cast<UFCClassSubsystem*>(this)->LoadClassCatalog();
	}

	TArray<const FFCCharacterClassData*> SortedRows;
	for (const auto& Pair : CachedClassRows)
	{
		if (Pair.Value.bShowInClassSelect && Pair.Key.IsValid())
		{
			SortedRows.Add(&Pair.Value);
		}
	}

	SortedRows.Sort([](const FFCCharacterClassData& A, const FFCCharacterClassData& B)
	{
		if (A.DisplayOrder != B.DisplayOrder)
		{
			return A.DisplayOrder < B.DisplayOrder;
		}
		return A.ClassTag.GetTagName().LexicalLess(B.ClassTag.GetTagName());
	});

	TArray<FGameplayTag> Result;
	Result.Reserve(SortedRows.Num());
	for (const FFCCharacterClassData* Row : SortedRows)
	{
		Result.Add(Row->ClassTag);
	}
	return Result;
}

void UFCClassSubsystem::RegisterClassDataAsset(UFCClassDataAsset* DataAsset)
{
	if (!DataAsset || !DataAsset->GetClassTag().IsValid())
	{
		return;
	}

	GLastActiveClassSubsystem = this;
	if (!bCatalogLoaded)
	{
		LoadClassCatalog();
	}

	ClassCatalog.Add(DataAsset->GetClassTag(), DataAsset);
	CachedClassRows.Add(DataAsset->GetClassTag(), DataAsset->ClassData);
}

void UFCClassSubsystem::RegisterClassRow(const FFCCharacterClassData& InClassRow)
{
	if (!InClassRow.ClassTag.IsValid())
	{
		return;
	}

	GLastActiveClassSubsystem = this;
	if (!bCatalogLoaded)
	{
		LoadClassCatalog();
	}
	CachedClassRows.Add(InClassRow.ClassTag, InClassRow);
	if (TObjectPtr<UFCClassDataAsset>* Existing = ClassCatalog.Find(InClassRow.ClassTag))
	{
		if (Existing->Get())
		{
			(*Existing)->ClassData = InClassRow;
		}
	}
}

void UFCClassSubsystem::AddClassDataTable(UDataTable* InTable)
{
	if (!InTable)
	{
		return;
	}
	GLastActiveClassSubsystem = this;
	if (!bCatalogLoaded)
	{
		LoadClassCatalog();
	}
	ClassDataTables.AddUnique(InTable);
	IngestClassDataTable(InTable);
}

const FFCElementDefinitionRow* UFCClassSubsystem::FindElementRowPtr(FGameplayTag ElementTag) const
{
	if (!ElementTag.IsValid())
	{
		return nullptr;
	}

	if (!bCatalogLoaded)
	{
		const_cast<UFCClassSubsystem*>(this)->LoadClassCatalog();
	}

	if (const FFCElementDefinitionRow* Found = CachedElementRows.Find(ElementTag))
	{
		return Found;
	}

	return FindBuiltinElementDefault(ElementTag);
}

void UFCClassSubsystem::RegisterElementRow(const FFCElementDefinitionRow& InElementRow)
{
	if (!InElementRow.ElementTag.IsValid())
	{
		return;
	}
	GLastActiveClassSubsystem = this;
	if (!bCatalogLoaded)
	{
		LoadClassCatalog();
	}
	CachedElementRows.Add(InElementRow.ElementTag, InElementRow);
}

void UFCClassSubsystem::AddElementDataTable(UDataTable* InTable)
{
	if (!InTable)
	{
		return;
	}
	GLastActiveClassSubsystem = this;
	if (!bCatalogLoaded)
	{
		LoadClassCatalog();
	}
	ElementDataTables.AddUnique(InTable);
	IngestElementDataTable(InTable);
}

const TArray<FFCElementReactionRow>& UFCClassSubsystem::GetElementReactionRules() const
{
	if (!bCatalogLoaded)
	{
		const_cast<UFCClassSubsystem*>(this)->LoadClassCatalog();
	}
	return CachedReactionRules;
}

void UFCClassSubsystem::RegisterElementReactionRow(const FFCElementReactionRow& InReactionRow)
{
	if (InReactionRow.RequiredElementTags.Num() == 0)
	{
		return;
	}
	GLastActiveClassSubsystem = this;
	if (!bCatalogLoaded)
	{
		LoadClassCatalog();
	}

	for (FFCElementReactionRow& Existing : CachedReactionRules)
	{
		if (!InReactionRow.ReactionId.IsNone() && Existing.ReactionId == InReactionRow.ReactionId)
		{
			Existing = InReactionRow;
			SortReactionRules();
			return;
		}
	}

	CachedReactionRules.Add(InReactionRow);
	SortReactionRules();
}

void UFCClassSubsystem::AddElementReactionDataTable(UDataTable* InTable)
{
	if (!InTable)
	{
		return;
	}
	GLastActiveClassSubsystem = this;
	if (!bCatalogLoaded)
	{
		LoadClassCatalog();
	}
	ElementReactionTables.AddUnique(InTable);
	IngestReactionDataTable(InTable);
}

void UFCClassSubsystem::RegisterBuiltinDefaults()
{
	for (const auto& Pair : GetBuiltinElementMap())
	{
		if (!CachedElementRows.Contains(Pair.Key))
		{
			CachedElementRows.Add(Pair.Key, Pair.Value);
		}
	}

	for (const auto& Pair : GetBuiltinClassMap())
	{
		if (!CachedClassRows.Contains(Pair.Key))
		{
			CachedClassRows.Add(Pair.Key, Pair.Value);
		}
	}
}

void UFCClassSubsystem::IngestClassDataTable(UDataTable* Table)
{
	if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FFCCharacterClassData::StaticStruct()))
	{
		return;
	}

	for (const auto& RowPair : Table->GetRowMap())
	{
		if (const FFCCharacterClassData* Row = reinterpret_cast<const FFCCharacterClassData*>(RowPair.Value))
		{
			RegisterClassRow(*Row);
		}
	}
}

void UFCClassSubsystem::IngestElementDataTable(UDataTable* Table)
{
	if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FFCElementDefinitionRow::StaticStruct()))
	{
		return;
	}

	for (const auto& RowPair : Table->GetRowMap())
	{
		if (const FFCElementDefinitionRow* Row = reinterpret_cast<const FFCElementDefinitionRow*>(RowPair.Value))
		{
			RegisterElementRow(*Row);
		}
	}
}

void UFCClassSubsystem::IngestReactionDataTable(UDataTable* Table)
{
	if (!Table || !Table->GetRowStruct() || !Table->GetRowStruct()->IsChildOf(FFCElementReactionRow::StaticStruct()))
	{
		return;
	}

	for (const auto& RowPair : Table->GetRowMap())
	{
		if (const FFCElementReactionRow* Row = reinterpret_cast<const FFCElementReactionRow*>(RowPair.Value))
		{
			FFCElementReactionRow Copy = *Row;
			if (Copy.ReactionId.IsNone())
			{
				Copy.ReactionId = RowPair.Key;
			}
			RegisterElementReactionRow(Copy);
		}
	}
}

void UFCClassSubsystem::SortReactionRules()
{
	CachedReactionRules.Sort([](const FFCElementReactionRow& A, const FFCElementReactionRow& B)
	{
		if (A.Priority != B.Priority)
		{
			return A.Priority > B.Priority;
		}
		return A.RequiredElementTags.Num() > B.RequiredElementTags.Num();
	});
}

void UFCClassSubsystem::LoadClassCatalog()
{
	bCatalogLoaded = true;
	GLastActiveClassSubsystem = this;
	RegisterBuiltinDefaults();

	if (const UFCDataTableSettings* Settings = GetDefault<UFCDataTableSettings>())
	{
		for (const TSoftObjectPtr<UDataTable>& SoftTable : Settings->ElementCatalogTables)
		{
			if (!SoftTable.IsNull())
			{
				if (UDataTable* Loaded = SoftTable.LoadSynchronous())
				{
					ElementDataTables.AddUnique(Loaded);
					IngestElementDataTable(Loaded);
				}
			}
		}

		for (const TSoftObjectPtr<UDataTable>& SoftTable : Settings->ClassCatalogTables)
		{
			if (!SoftTable.IsNull())
			{
				if (UDataTable* Loaded = SoftTable.LoadSynchronous())
				{
					ClassDataTables.AddUnique(Loaded);
					IngestClassDataTable(Loaded);
				}
			}
		}

		for (const TSoftObjectPtr<UDataTable>& SoftTable : Settings->ElementReactionTables)
		{
			if (!SoftTable.IsNull())
			{
				if (UDataTable* Loaded = SoftTable.LoadSynchronous())
				{
					ElementReactionTables.AddUnique(Loaded);
					IngestReactionDataTable(Loaded);
				}
			}
		}
	}

	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> AssetIdList;
		AssetManager->GetPrimaryAssetIdList(FPrimaryAssetType("CharacterClass"), AssetIdList);

		for (const FPrimaryAssetId& AssetId : AssetIdList)
		{
			if (UObject* LoadedObj = AssetManager->GetPrimaryAssetObject(AssetId))
			{
				if (UFCClassDataAsset* ClassAsset = Cast<UFCClassDataAsset>(LoadedObj))
				{
					RegisterClassDataAsset(ClassAsset);
				}
			}
		}
	}
}
