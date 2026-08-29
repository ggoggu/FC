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
