#include "Data/Card/FCCardDataAsset.h"

UFCCardDataAsset::UFCCardDataAsset()
{
}

FPrimaryAssetId UFCCardDataAsset::GetPrimaryAssetId() const
{
	FName AssetName = GameplayData.CardId.IsNone() ? GetFName() : GameplayData.CardId;
	return FPrimaryAssetId(FPrimaryAssetType("Card"), AssetName);
}
