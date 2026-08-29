#include "Game/FCGameMode.h"
#include "Game/FCGameState.h"
#include "Game/FCPlayerState.h"
#include "Controller/Player/FCPlayerController.h"
#include "Character/Player/FCPlayerCharacter.h"

AFCGameMode::AFCGameMode()
{
	// Default class assignments for the FC Framework
	GameStateClass = AFCGameState::StaticClass();
	PlayerStateClass = AFCPlayerState::StaticClass();
	PlayerControllerClass = AFCPlayerController::StaticClass();
	DefaultPawnClass = AFCPlayerCharacter::StaticClass();
}

void AFCGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AFCGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void AFCGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
}
