#include "UI/ClassSelect/FCClassSelectGameMode.h"
#include "UI/ClassSelect/FCClassSelectPlayerController.h"
#include "Engine/World.h"

AFCClassSelectGameMode::AFCClassSelectGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AFCClassSelectPlayerController::StaticClass();
}

void AFCClassSelectGameMode::TransitionToNextLevel()
{
	if (!NextLevelMapURL.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(NextLevelMapURL, false);
		}
	}
}

