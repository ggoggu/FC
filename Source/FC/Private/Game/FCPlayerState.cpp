#include "Game/FCPlayerState.h"
#include "Net/UnrealNetwork.h"

AFCPlayerState::AFCPlayerState()
{
	bReplicates = true;
	NetUpdateFrequency = 30.0f;
}

void AFCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
