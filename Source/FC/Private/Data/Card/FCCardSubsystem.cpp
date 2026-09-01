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
}
