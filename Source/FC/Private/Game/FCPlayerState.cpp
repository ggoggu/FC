#include "Game/FCPlayerState.h"
#include "Card/FCCardDeckComponent.h"
#include "Net/UnrealNetwork.h"

AFCPlayerState::AFCPlayerState()
{
	bReplicates = true;
	NetUpdateFrequency = 30.0f;

	CardDeckComponent = CreateDefaultSubobject<UFCCardDeckComponent>(TEXT("CardDeckComponent"));
}

void AFCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AFCPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);

	if (AFCPlayerState* NewFCPlayerState = Cast<AFCPlayerState>(NewPlayerState))
	{
		if (CardDeckComponent && NewFCPlayerState->CardDeckComponent)
		{
			FFCCardDeckSaveData DeckData = CardDeckComponent->ExportDeckSaveData();
			NewFCPlayerState->CardDeckComponent->RestoreFromDeckSaveData(DeckData);
		}
	}
}
