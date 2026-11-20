#include "Data/Class/FCClassDataAsset.h"
#include "Data/FCGameplayTags.h"

UFCClassDataAsset::UFCClassDataAsset()
{
	ClassData.ClassTag = FCGameplayTags::Class_Neutral;
	ClassData.BaseMaxHealth = 100.0f;
	ClassData.BaseMaxMana = 50.0f;
	ClassData.BaseAttackPower = 0.0f;
}

FPrimaryAssetId UFCClassDataAsset::GetPrimaryAssetId() const
{
	const FName AssetName = ClassData.ClassTag.IsValid() ? ClassData.ClassTag.GetTagName() : GetFName();
	return FPrimaryAssetId(FPrimaryAssetType("CharacterClass"), AssetName);
}
