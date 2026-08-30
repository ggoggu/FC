#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
#include "AbilitySystem/Abilities/FCGA_Fireball.h"
#include "AbilitySystem/Effects/FCGE_AttackBuff.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UFCCardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadCardCatalog();
}

void UFCCardSubsystem::Deinitialize()
{
	CardCatalog.Empty();
	Super::Deinitialize();
}

UFCCardSubsystem* UFCCardSubsystem::GetCardSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!GI)
	{
		for (const UObject* Curr = WorldContextObject; Curr; Curr = Curr->GetOuter())
		{
			if (const UGameInstance* OuterGI = Cast<UGameInstance>(Curr))
			{
				return const_cast<UGameInstance*>(OuterGI)->GetSubsystem<UFCCardSubsystem>();
			}
		}

		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.OwningGameInstance)
				{
					return Context.OwningGameInstance->GetSubsystem<UFCCardSubsystem>();
				}
			}
		}
	}

	return GI ? GI->GetSubsystem<UFCCardSubsystem>() : nullptr;
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
		if (UObject* LoadedObj = AssetManager->GetPrimaryAssetObject(AssetId))
		{
			if (UFCCardDataAsset* CardAsset = Cast<UFCCardDataAsset>(LoadedObj))
			{
				const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(CardAsset);
				return CardAsset;
			}
		}
	}

	// Fallback dynamic creation for built-in cards (e.g. Card_Fireball, Card_AttackBuff)
	if (CardId == FName("Card_Fireball"))
	{
		UFCCardDataAsset* FireballAsset = NewObject<UFCCardDataAsset>(const_cast<UFCCardSubsystem*>(this));
		FireballAsset->GameplayData.CardId = FName("Card_Fireball");
		FireballAsset->GameplayData.BaseManaCost = 1;
		FireballAsset->GameplayData.CardType = EFCCardType::Attack;
		FireballAsset->GameplayData.TargetType = EFCCardTargetType::DirectionalAoE;
		FireballAsset->GameplayData.BaseValue = 1.0f;
		FireballAsset->GameplayData.CardAbilityClass = UFCGA_Fireball::StaticClass();
		FireballAsset->GameplayData.RequiredClass = EFCCharacterClass::Mage;
		FireballAsset->GameplayData.Elements = { EFCElement::Fire, EFCElement::Earth };

		FireballAsset->DisplayData.CardName = FText::FromString(TEXT("파이어 볼"));
		FireballAsset->DisplayData.CardDescription = FText::FromString(TEXT("전방으로 화염구를 직선 발사하여 적중한 대상에게 1의 피해를 입힙니다."));
		FireballAsset->DisplayData.Rarity = EFCCardRarity::Common;

		const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(FireballAsset);
		return FireballAsset;
	}
	else if (CardId == FName("Card_AttackBuff"))
	{
		UFCCardDataAsset* AttackBuffAsset = NewObject<UFCCardDataAsset>(const_cast<UFCCardSubsystem*>(this));
		AttackBuffAsset->GameplayData.CardId = FName("Card_AttackBuff");
		AttackBuffAsset->GameplayData.BaseManaCost = 1;
		AttackBuffAsset->GameplayData.CardType = EFCCardType::Skill;
		AttackBuffAsset->GameplayData.TargetType = EFCCardTargetType::Self;
		AttackBuffAsset->GameplayData.BaseValue = 1.0f;
		AttackBuffAsset->GameplayData.CardEffectClasses.Add(UFCGE_AttackBuff::StaticClass());
		AttackBuffAsset->GameplayData.RequiredClass = EFCCharacterClass::Neutral;
		AttackBuffAsset->GameplayData.Elements.Empty();

		AttackBuffAsset->DisplayData.CardName = FText::FromString(TEXT("공격력 강화"));
		AttackBuffAsset->DisplayData.CardDescription = FText::FromString(TEXT("1분 동안 자신의 공격력을 1 증가시킵니다."));
		AttackBuffAsset->DisplayData.Rarity = EFCCardRarity::Common;

		const_cast<UFCCardSubsystem*>(this)->RegisterCardDataAsset(AttackBuffAsset);
		return AttackBuffAsset;
	}

	return nullptr;
}

void UFCCardSubsystem::RegisterCardDataAsset(UFCCardDataAsset* DataAsset)
{
	if (!DataAsset)
	{
		return;
	}

	FName CardId = DataAsset->GetCardId();
	if (CardId.IsNone())
	{
		CardId = DataAsset->GetFName();
	}

	CardCatalog.Add(CardId, DataAsset);
}

void UFCCardSubsystem::LoadCardCatalog()
{
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
