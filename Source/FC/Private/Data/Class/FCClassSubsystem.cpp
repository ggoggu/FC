#include "Data/Class/FCClassSubsystem.h"
#include "Data/Class/FCClassDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UFCClassSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadClassCatalog();
}

void UFCClassSubsystem::Deinitialize()
{
	ClassCatalog.Empty();
	Super::Deinitialize();
}

UFCClassSubsystem* UFCClassSubsystem::GetClassSubsystem(const UObject* WorldContextObject)
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
				return const_cast<UGameInstance*>(OuterGI)->GetSubsystem<UFCClassSubsystem>();
			}
		}

		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.OwningGameInstance)
				{
					return Context.OwningGameInstance->GetSubsystem<UFCClassSubsystem>();
				}
			}
		}
	}

	return GI ? GI->GetSubsystem<UFCClassSubsystem>() : nullptr;
}

UFCClassDataAsset* UFCClassSubsystem::GetClassDataAsset(EFCCharacterClass ClassType) const
{
	if (const TObjectPtr<UFCClassDataAsset>* Found = ClassCatalog.Find(ClassType))
	{
		return Found->Get();
	}

	// Try finding via AssetManager if not yet cached
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		FString ClassName = StaticEnum<EFCCharacterClass>()->GetNameStringByValue((int64)ClassType);
		FPrimaryAssetId AssetId(FPrimaryAssetType("CharacterClass"), FName(*ClassName));
		UObject* LoadedObj = AssetManager->GetPrimaryAssetObject(AssetId);
		if (!LoadedObj)
		{
			FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(AssetId);
			if (AssetPath.IsValid())
			{
				LoadedObj = AssetPath.TryLoad();
			}
		}

		if (UFCClassDataAsset* ClassAsset = Cast<UFCClassDataAsset>(LoadedObj))
		{
			const_cast<UFCClassSubsystem*>(this)->RegisterClassDataAsset(ClassAsset);
			return ClassAsset;
		}
	}

	// Built-in fallback definitions
	if (ClassType == EFCCharacterClass::Mage)
	{
		UFCClassDataAsset* MageAsset = NewObject<UFCClassDataAsset>(const_cast<UFCClassSubsystem*>(this));
		MageAsset->ClassData.ClassType = EFCCharacterClass::Mage;
		MageAsset->ClassData.ClassDisplayName = NSLOCTEXT("FCClass", "ClassMage", "마법사");
		MageAsset->ClassData.ClassDescription = NSLOCTEXT("FCClass", "ClassMageDesc", "강력한 원소 마법을 다루는 주문 시전자입니다.");
		MageAsset->ClassData.AffinityElements = {
			EFCElement::Fire,
			EFCElement::Earth,
			EFCElement::Water,
			EFCElement::Wind,
			EFCElement::Lightning
		};
		MageAsset->ClassData.StartingDeck = {
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_MagicShield"),
			FName("Card_MagicShield")
		};
		MageAsset->ClassData.BaseMaxHealth = 80.0f;
		MageAsset->ClassData.BaseMaxMana = 100.0f;
		MageAsset->ClassData.BaseAttackPower = 0.0f;

		const_cast<UFCClassSubsystem*>(this)->RegisterClassDataAsset(MageAsset);
		return MageAsset;
	}
	else if (ClassType == EFCCharacterClass::Neutral)
	{
		UFCClassDataAsset* NeutralAsset = NewObject<UFCClassDataAsset>(const_cast<UFCClassSubsystem*>(this));
		NeutralAsset->ClassData.ClassType = EFCCharacterClass::Neutral;
		NeutralAsset->ClassData.ClassDisplayName = NSLOCTEXT("FCClass", "ClassNeutral", "중립");
		NeutralAsset->ClassData.ClassDescription = NSLOCTEXT("FCClass", "ClassNeutralDesc", "모든 직업이 공용으로 사용할 수 있는 기본 설정입니다.");
		NeutralAsset->ClassData.AffinityElements.Empty();
		NeutralAsset->ClassData.StartingDeck = {
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_Fireball"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff"),
			FName("Card_AttackBuff")
		};
		NeutralAsset->ClassData.BaseMaxHealth = 100.0f;
		NeutralAsset->ClassData.BaseMaxMana = 50.0f;
		NeutralAsset->ClassData.BaseAttackPower = 0.0f;

		const_cast<UFCClassSubsystem*>(this)->RegisterClassDataAsset(NeutralAsset);
		return NeutralAsset;
	}

	return nullptr;
}

TArray<FName> UFCClassSubsystem::GetStartingDeckForClass(EFCCharacterClass ClassType) const
{
	if (const UFCClassDataAsset* Asset = GetClassDataAsset(ClassType))
	{
		return Asset->ClassData.StartingDeck;
	}

	// Default fallback
	return {
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff")
	};
}

TArray<EFCElement> UFCClassSubsystem::GetAffinityElementsForClass(EFCCharacterClass ClassType) const
{
	if (const UFCClassDataAsset* Asset = GetClassDataAsset(ClassType))
	{
		return Asset->ClassData.AffinityElements;
	}

	return {};
}

void UFCClassSubsystem::RegisterClassDataAsset(UFCClassDataAsset* DataAsset)
{
	if (!DataAsset)
	{
		return;
	}

	ClassCatalog.Add(DataAsset->GetClassType(), DataAsset);
}

void UFCClassSubsystem::LoadClassCatalog()
{
	if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> AssetIdList;
		AssetManager->GetPrimaryAssetIdList(FPrimaryAssetType("CharacterClass"), AssetIdList);

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

			if (UFCClassDataAsset* ClassAsset = Cast<UFCClassDataAsset>(LoadedObj))
			{
				RegisterClassDataAsset(ClassAsset);
			}
		}
	}
}
