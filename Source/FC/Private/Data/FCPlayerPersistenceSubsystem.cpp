#include "Data/FCPlayerPersistenceSubsystem.h"
#include "Card/FCCardDeckComponent.h"
#include "Character/FCCharacterBase.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

void UFCPlayerPersistenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PersistentRecords.Empty();
}

void UFCPlayerPersistenceSubsystem::Deinitialize()
{
	PersistentRecords.Empty();
	Super::Deinitialize();
}

UFCPlayerPersistenceSubsystem* UFCPlayerPersistenceSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GI ? GI->GetSubsystem<UFCPlayerPersistenceSubsystem>() : nullptr;
}

FString UFCPlayerPersistenceSubsystem::GetPlayerKey(const APlayerController* PC) const
{
	if (!PC)
	{
		return TEXT("InvalidPlayer");
	}

	if (const APlayerState* PS = PC->GetPlayerState<APlayerState>())
	{
		const int32 PlayerId = PS->GetPlayerId();
		if (PlayerId > 0)
		{
			return FString::Printf(TEXT("Player_%d"), PlayerId);
		}

		const FString PlayerName = PS->GetPlayerName();
		if (!PlayerName.IsEmpty())
		{
			return FString::Printf(TEXT("Player_%s"), *PlayerName);
		}
	}

	return TEXT("LocalPlayer_0");
}

bool UFCPlayerPersistenceSubsystem::HasPlayerData(const FString& PlayerKey) const
{
	if (PlayerKey.IsEmpty())
	{
		return false;
	}

	if (const FFCPlayerPersistentData* Found = PersistentRecords.Find(PlayerKey))
	{
		return Found->bHasValidData;
	}

	return false;
}

void UFCPlayerPersistenceSubsystem::SavePlayerData(const FString& PlayerKey, const FFCCardDeckSaveData& DeckData, const FFCPlayerStatSaveData& StatData)
{
	if (PlayerKey.IsEmpty())
	{
		return;
	}

	FFCPlayerPersistentData& Record = PersistentRecords.FindOrAdd(PlayerKey);
	Record.PlayerKey = PlayerKey;
	Record.DeckData = DeckData;
	Record.StatData = StatData;
	Record.bHasValidData = true;
}

bool UFCPlayerPersistenceSubsystem::LoadPlayerData(const FString& PlayerKey, FFCCardDeckSaveData& OutDeckData, FFCPlayerStatSaveData& OutStatData) const
{
	if (const FFCPlayerPersistentData* Found = PersistentRecords.Find(PlayerKey))
	{
		if (Found->bHasValidData)
		{
			OutDeckData = Found->DeckData;
			OutStatData = Found->StatData;
			return true;
		}
	}

	return false;
}

bool UFCPlayerPersistenceSubsystem::SaveFromPlayer(APlayerController* PC)
{
	if (!PC)
	{
		return false;
	}

	const FString Key = GetPlayerKey(PC);

	FFCCardDeckSaveData DeckData;
	bool bHasDeckData = false;
	if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
	{
		if (UFCCardDeckComponent* DeckComp = PS->FindComponentByClass<UFCCardDeckComponent>())
		{
			DeckData = DeckComp->ExportDeckSaveData();
			bHasDeckData = true;
		}
	}

	FFCPlayerStatSaveData StatData;
	bool bHasStatData = false;
	if (APawn* Pawn = PC->GetPawn())
	{
		if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(Pawn))
		{
			if (UFCAttributeSet* AttrSet = Char->GetAttributeSet())
			{
				StatData = AttrSet->ExportStatSaveData();
				bHasStatData = true;
			}
		}
	}

	if (bHasDeckData || bHasStatData)
	{
		SavePlayerData(Key, DeckData, StatData);
		return true;
	}

	return false;
}

bool UFCPlayerPersistenceSubsystem::RestoreToPlayer(APlayerController* PC)
{
	if (!PC)
	{
		return false;
	}

	const FString Key = GetPlayerKey(PC);
	FFCCardDeckSaveData DeckData;
	FFCPlayerStatSaveData StatData;

	if (!LoadPlayerData(Key, DeckData, StatData))
	{
		return false;
	}

	if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
	{
		if (UFCCardDeckComponent* DeckComp = PS->FindComponentByClass<UFCCardDeckComponent>())
		{
			DeckComp->RestoreFromDeckSaveData(DeckData);
		}
	}

	if (APawn* Pawn = PC->GetPawn())
	{
		if (AFCCharacterBase* Char = Cast<AFCCharacterBase>(Pawn))
		{
			if (UFCAttributeSet* AttrSet = Char->GetAttributeSet())
			{
				AttrSet->RestoreFromStatSaveData(StatData);
			}
		}
	}

	return true;
}

void UFCPlayerPersistenceSubsystem::ClearPlayerData(const FString& PlayerKey)
{
	PersistentRecords.Remove(PlayerKey);
}

void UFCPlayerPersistenceSubsystem::ClearAllPlayerData()
{
	PersistentRecords.Empty();
}
