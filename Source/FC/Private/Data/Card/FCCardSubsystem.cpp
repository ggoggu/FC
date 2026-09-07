#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "AbilitySystem/Abilities/FCGA_Ignite.h"
#include "AbilitySystem/Effects/FCGE_MagicShield.h"
#include "AbilitySystem/Effects/FCGE_AttackBuff.h"
#include "Combat/Projectile/FCProjectileDataAsset.h"
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

	if (CachedCardRows.Num() == 0)
	{
		const_cast<UFCCardSubsystem*>(this)->PopulateDefaultCatalog();
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
	if (CachedCardRows.Num() == 0)
	{
		const_cast<UFCCardSubsystem*>(this)->PopulateDefaultCatalog();
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

	if (CachedCardRows.Num() == 0)
	{
		const_cast<UFCCardSubsystem*>(this)->PopulateDefaultCatalog();
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
	LoadCardCatalog();
}

void UFCCardSubsystem::PopulateDefaultCatalog()
{
	// 1. Card_Fireball
	{
		FFCCardTableRow Fireball;
		Fireball.GameplayData.CardId = FName("Card_Fireball");
		Fireball.GameplayData.BaseManaCost = 2;
		Fireball.GameplayData.CardType = EFCCardType::Attack;
		Fireball.GameplayData.TargetType = EFCCardTargetType::DirectionalAoE;
		Fireball.GameplayData.BaseValue = 1.0f;
		Fireball.GameplayData.bSpawnsProjectile = true;
		Fireball.GameplayData.CardAbilityClass = UFCGA_Fireball::StaticClass();
		Fireball.GameplayData.RequiredClass = EFCCharacterClass::Mage;
		Fireball.GameplayData.Elements = { EFCElement::Fire, EFCElement::Earth };
		Fireball.DisplayData.CardName = NSLOCTEXT("FCCard", "Card_Fireball_Name", "파이어 볼");
		Fireball.DisplayData.CardDescription = NSLOCTEXT("FCCard", "Card_Fireball_Desc", "전방으로 화염구를 직선 발사하여 적중한 대상에게 1의 피해를 입힙니다.");
		Fireball.DisplayData.Rarity = EFCCardRarity::Common;

		RegisterCardRow(FName("Card_Fireball"), Fireball);
		RegisterCardRow(FName("DA_Card_Fireball"), Fireball);
	}

	// 2. Card_FireArrow
	{
		FFCCardTableRow FireArrow;
		FireArrow.GameplayData.CardId = FName("Card_FireArrow");
		FireArrow.GameplayData.BaseManaCost = 1;
		FireArrow.GameplayData.CardType = EFCCardType::Attack;
		FireArrow.GameplayData.TargetType = EFCCardTargetType::SingleTarget;
		FireArrow.GameplayData.BaseValue = 10.0f;
		FireArrow.GameplayData.bSpawnsProjectile = true;
		FireArrow.GameplayData.ProjectileDataAsset = TSoftObjectPtr<UFCProjectileDataAsset>(FSoftObjectPath(TEXT("/Game/Card/DA_Projectile_FireArrow.DA_Projectile_FireArrow")));
		FireArrow.GameplayData.RequiredClass = EFCCharacterClass::Mage;
		FireArrow.GameplayData.Elements = { EFCElement::Fire };
		FireArrow.DisplayData.CardName = NSLOCTEXT("FCCard", "Card_FireArrow_Name", "파이어 애로우");
		FireArrow.DisplayData.CardDescription = NSLOCTEXT("FCCard", "Card_FireArrow_Desc", "화염 화살을 발사하여 10의 피해를 입힙니다.");
		FireArrow.DisplayData.Rarity = EFCCardRarity::Common;

		RegisterCardRow(FName("Card_FireArrow"), FireArrow);
		RegisterCardRow(FName("DA_Card_FireArrow"), FireArrow);
	}

	// 3. Card_AttackBuff
	{
		FFCCardTableRow AttackBuff;
		AttackBuff.GameplayData.CardId = FName("Card_AttackBuff");
		AttackBuff.GameplayData.BaseManaCost = 1;
		AttackBuff.GameplayData.CardType = EFCCardType::Skill;
		AttackBuff.GameplayData.TargetType = EFCCardTargetType::Self;
		AttackBuff.GameplayData.BaseValue = 5.0f;
		AttackBuff.GameplayData.CardEffectClasses.Add(UFCGE_AttackBuff::StaticClass());
		AttackBuff.GameplayData.RequiredClass = EFCCharacterClass::Neutral;
		AttackBuff.DisplayData.CardName = NSLOCTEXT("FCCard", "Card_AttackBuff_Name", "공격력 강화");
		AttackBuff.DisplayData.CardDescription = NSLOCTEXT("FCCard", "Card_AttackBuff_Desc", "자신의 공격력을 증가시킵니다.");
		AttackBuff.DisplayData.Rarity = EFCCardRarity::Common;

		RegisterCardRow(FName("Card_AttackBuff"), AttackBuff);
		RegisterCardRow(FName("DA_Card_AttackBuff"), AttackBuff);
	}

	// 4. Card_MagicShield
	{
		FFCCardTableRow MagicShield;
		MagicShield.GameplayData.CardId = FName("Card_MagicShield");
		MagicShield.GameplayData.BaseManaCost = 1;
		MagicShield.GameplayData.CardType = EFCCardType::Skill;
		MagicShield.GameplayData.TargetType = EFCCardTargetType::Self;
		MagicShield.GameplayData.BaseValue = 20.0f;
		MagicShield.GameplayData.CardEffectClasses.Add(UFCGE_MagicShield::StaticClass());
		MagicShield.GameplayData.RequiredClass = EFCCharacterClass::Mage;
		MagicShield.DisplayData.CardName = NSLOCTEXT("FCCard", "Card_MagicShield_Name", "매직실드");
		MagicShield.DisplayData.CardDescription = NSLOCTEXT("FCCard", "Card_MagicShield_Desc", "자신에게 전신체 보호막을 생성하여 피해를 흡수합니다.");
		MagicShield.DisplayData.Rarity = EFCCardRarity::Common;

		RegisterCardRow(FName("Card_MagicShield"), MagicShield);
		RegisterCardRow(FName("DA_Card_MagicShield"), MagicShield);
	}

	// 5. Card_Ignite
	{
		FFCCardTableRow Ignite;
		Ignite.GameplayData.CardId = FName("Card_Ignite");
		Ignite.GameplayData.BaseManaCost = 1;
		Ignite.GameplayData.CardType = EFCCardType::Attack;
		Ignite.GameplayData.TargetType = EFCCardTargetType::AllEnemies;
		Ignite.GameplayData.BaseValue = 10.0f;
		Ignite.GameplayData.CardAbilityClass = UFCGA_Ignite::StaticClass();
		Ignite.GameplayData.RequiredClass = EFCCharacterClass::Mage;
		Ignite.GameplayData.Elements = { EFCElement::None };
		Ignite.DisplayData.CardName = NSLOCTEXT("FCCard", "Card_Ignite_Name", "점화");
		Ignite.DisplayData.CardDescription = NSLOCTEXT("FCCard", "Card_Ignite_Desc", "근처에 있는 모든 적에게 불스택 1개당 10의 피해를 입힙니다.");
		Ignite.DisplayData.Rarity = EFCCardRarity::Uncommon;

		RegisterCardRow(FName("Card_Ignite"), Ignite);
		RegisterCardRow(FName("Card_점화"), Ignite);
		RegisterCardRow(FName("DA_Card_Ignite"), Ignite);
	}
}

void UFCCardSubsystem::LoadCardCatalog()
{
	// 1. Establish baseline fallback catalog (zero-hitch, guarantees baseline stability)
	PopulateDefaultCatalog();

	// 2. Load DataTable if not already bound
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

	// 3. Overlay rows from DataTable if present
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

