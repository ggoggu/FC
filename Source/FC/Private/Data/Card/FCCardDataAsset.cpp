#include "Data/Card/FCCardDataAsset.h"

UFCCardDataAsset::UFCCardDataAsset()
{
}

FPrimaryAssetId UFCCardDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType("Card"), GetFName());
}
