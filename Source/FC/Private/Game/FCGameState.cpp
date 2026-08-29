#include "Game/FCGameState.h"
#include "Net/UnrealNetwork.h"

AFCGameState::AFCGameState()
{
	bReplicates = true;
	NetUpdateFrequency = 30.0f;
}

void AFCGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
