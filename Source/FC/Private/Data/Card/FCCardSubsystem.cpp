#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "AbilitySystem/Effects/FCGE_MagicShield.h"
#include "AbilitySystem/Abilities/FCGA_Ignite.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
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

UFCCardDataAsset* UFCCardSubsystem::GetCardDataAsset(FName CardId) const
{
	if (CardId.IsNone())
	{
		return nullptr;
	}

	if (const TObjectPtr<UFCCardDataAsset>* Found = CardCatalog.Find(CardId))
	{
		return Found->Get();
	}

	// Try finding via AssetManager if not yet cached
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

	// Fallback: Direct object loading by conventional content paths
	// Guarantees asset resolution even before AssetManager completes its async initial scan
	const FString CleanName = CardId.ToString();
	const FString PrefixedName = CleanName.StartsWith(TEXT("DA_")) ? CleanName : FString::Printf(TEXT("DA_%s"), *CleanName);

	const TArray<FString> CandidatePaths = {
		FString::Printf(TEXT("/Game/Card/%s.%s"), *PrefixedName, *PrefixedName),
		FString::Printf(TEXT("/Game/Card/%s.%s"), *CleanName, *CleanName),
		FString::Printf(TEXT("/Game/Data/Card/%s.%s"), *PrefixedName, *PrefixedName),
		FString::Printf(TEXT("/Game/Data/Card/%s.%s"), *CleanName, *CleanName),
		FString::Printf(TEXT("/Game/Data/Cards/%s.%s"), *PrefixedName, *PrefixedName)
	};

	for (const FString& PathStr : CandidatePaths)
	{
		if (UFCCardDataAsset* LoadedAsset = Cast<UFCCardDataAsset>(StaticLoadObject(UFCCardDataAsset::StaticClass(), nullptr, *PathStr)))
		{
			const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(LoadedAsset);
			return LoadedAsset;
		}
	}

	// Built-in fallback definitions
	if (CardId == FName("Card_MagicShield"))
	{
		UFCCardDataAsset* MagicShieldAsset = NewObject<UFCCardDataAsset>(const_cast<UFCCardSubsystem*>(this));
		MagicShieldAsset->GameplayData.CardId = FName("Card_MagicShield");
		MagicShieldAsset->GameplayData.BaseManaCost = 1;
		MagicShieldAsset->GameplayData.CardType = EFCCardType::Skill;
		MagicShieldAsset->GameplayData.TargetType = EFCCardTargetType::Self;
		MagicShieldAsset->GameplayData.BaseValue = 20.0f;
		MagicShieldAsset->GameplayData.CardEffectClasses.Add(UFCGE_MagicShield::StaticClass());
		MagicShieldAsset->GameplayData.RequiredClass = EFCCharacterClass::Mage;
		MagicShieldAsset->GameplayData.Elements.Empty();
		MagicShieldAsset->DisplayData.CardName = NSLOCTEXT("FCCard", "Card_MagicShield_Name", "매직실드");
		MagicShieldAsset->DisplayData.CardDescription = NSLOCTEXT("FCCard", "Card_MagicShield_Desc", "자신에게 전신체 보호막을 생성하여 피해를 흡수합니다.");
		MagicShieldAsset->DisplayData.Rarity = EFCCardRarity::Common;

		const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(MagicShieldAsset);
		return MagicShieldAsset;
	}

	if (CardId == FName("Card_Ignite") || CardId == FName("Card_점화") || CardId == FName("DA_Card_Ignite"))
	{
		UFCCardDataAsset* IgniteAsset = NewObject<UFCCardDataAsset>(const_cast<UFCCardSubsystem*>(this));
		IgniteAsset->GameplayData.CardId = FName("Card_Ignite");
		IgniteAsset->GameplayData.BaseManaCost = 1;
		IgniteAsset->GameplayData.CardType = EFCCardType::Attack;
		IgniteAsset->GameplayData.TargetType = EFCCardTargetType::AllEnemies;
		IgniteAsset->GameplayData.BaseValue = 10.0f;
		IgniteAsset->GameplayData.CardAbilityClass = UFCGA_Ignite::StaticClass();
		IgniteAsset->GameplayData.RequiredClass = EFCCharacterClass::Mage;
		IgniteAsset->GameplayData.Elements = { EFCElement::None };
		IgniteAsset->DisplayData.CardName = NSLOCTEXT("FCCard", "Card_Ignite_Name", "점화");
		IgniteAsset->DisplayData.CardDescription = NSLOCTEXT("FCCard", "Card_Ignite_Desc", "근처에 있는 모든 적에게 불스택 1개당 10의 피해를 입힙니다.");
		IgniteAsset->DisplayData.Rarity = EFCCardRarity::Uncommon;

		const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(IgniteAsset);
		if (CardId == FName("Card_점화"))
		{
			const_cast<UFCCardSubsystem*>(this)->CardCatalog.Add(FName("Card_점화"), IgniteAsset);
		}
		return IgniteAsset;
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
}

void UFCCardSubsystem::LoadCardCatalog()
{
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> AssetIdList;
		AssetManager->GetPrimaryAssetIdList(FPrimaryAssetType("Card"), AssetIdList);

		for (const FPrimaryAssetId& AssetId : AssetIdList)
		{
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
				RegisterCardDataAsset(CardAsset);
			}
		}
	}

	// Pre-cache known starter cards as an early fallback
	GetCardDataAsset(FName("Card_Fireball"));
	GetCardDataAsset(FName("Card_FireArrow"));
	GetCardDataAsset(FName("Card_AttackBuff"));
	GetCardDataAsset(FName("Card_MagicShield"));
	GetCardDataAsset(FName("Card_Ignite"));
}
