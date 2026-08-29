#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardDataAsset.h"
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
