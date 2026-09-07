#include "Game/FCPlayerState.h"
#include "Card/FCCardDeckComponent.h"
#include "Net/UnrealNetwork.h"

AFCPlayerState::AFCPlayerState()
{
	bReplicates = true;
	SetNetUpdateFrequency(30.0f);

	CardDeckComponent = CreateDefaultSubobject<UFCCardDeckComponent>(TEXT("CardDeckComponent"));
}

void AFCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFCPlayerState, CharacterClass);
}

void AFCPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);

	if (AFCPlayerState* NewFCPlayerState = Cast<AFCPlayerState>(NewPlayerState))
	{
		NewFCPlayerState->CharacterClass = CharacterClass;

		if (CardDeckComponent && NewFCPlayerState->CardDeckComponent)
		{
			FFCCardDeckSaveData DeckData = CardDeckComponent->ExportDeckSaveData();
			NewFCPlayerState->CardDeckComponent->RestoreFromDeckSaveData(DeckData);
		}
	}
}

void AFCPlayerState::SetCharacterClass(EFCCharacterClass NewClass)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CharacterClass != NewClass)
	{
		CharacterClass = NewClass;
		OnPlayerClassChanged.Broadcast(this, CharacterClass);
	}
}

void AFCPlayerState::OnRep_CharacterClass()
{
	OnPlayerClassChanged.Broadcast(this, CharacterClass);
}
