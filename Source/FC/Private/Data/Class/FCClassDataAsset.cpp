#include "Data/Class/FCClassDataAsset.h"

UFCClassDataAsset::UFCClassDataAsset()
{
	ClassData.ClassType = EFCCharacterClass::Neutral;
	ClassData.BaseMaxHealth = 100.0f;
	ClassData.BaseMaxMana = 50.0f;
	ClassData.BaseAttackPower = 0.0f;
}

FPrimaryAssetId UFCClassDataAsset::GetPrimaryAssetId() const
{
	FString ClassName = StaticEnum<EFCCharacterClass>()->GetNameStringByValue((int64)ClassData.ClassType);
	return FPrimaryAssetId(FPrimaryAssetType("CharacterClass"), FName(*ClassName));
}
