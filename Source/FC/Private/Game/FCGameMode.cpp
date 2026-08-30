#include "Game/FCGameMode.h"
#include "Game/FCGameState.h"
#include "Game/FCPlayerState.h"
#include "Controller/Player/FCPlayerController.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/FCPlayerPersistenceSubsystem.h"
#include "Kismet/GameplayStatics.h"

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

	if (!NewPlayer || !HasAuthority())
	{
		return;
	}

	if (UFCPlayerPersistenceSubsystem* PersistenceSubsystem = UFCPlayerPersistenceSubsystem::Get(this))
	{
		const FString Key = PersistenceSubsystem->GetPlayerKey(NewPlayer);
		if (PersistenceSubsystem->HasPlayerData(Key))
		{
			PersistenceSubsystem->RestoreToPlayer(NewPlayer);
		}
		else
		{
			// Initialize default starting deck if new player session
			if (AFCPlayerState* PS = NewPlayer->GetPlayerState<AFCPlayerState>())
			{
				if (UFCCardDeckComponent* DeckComp = PS->GetCardDeckComponent())
				{
					const TArray<FName> DefaultStartingDeck = {
						FName("Card_Fireball"),
						FName("Card_Fireball"),
						FName("Card_Fireball")
					};
					DeckComp->InitializeDeck(DefaultStartingDeck);
				}
			}
		}
	}
}

void AFCGameMode::Logout(AController* Exiting)
{
	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		if (UFCPlayerPersistenceSubsystem* PersistenceSubsystem = UFCPlayerPersistenceSubsystem::Get(this))
		{
			PersistenceSubsystem->SaveFromPlayer(PC);
		}
	}

	Super::Logout(Exiting);
}

void AFCGameMode::TransitionToLevel(const FString& MapURL, bool bSeamless)
{
	if (!HasAuthority() || MapURL.IsEmpty())
	{
		return;
	}

	// Save all connected players' state to persistence subsystem before map unload
	if (UFCPlayerPersistenceSubsystem* PersistenceSubsystem = UFCPlayerPersistenceSubsystem::Get(this))
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				PersistenceSubsystem->SaveFromPlayer(PC);
			}
		}
	}

	if (bSeamless)
	{
		bUseSeamlessTravel = true;
		GetWorld()->ServerTravel(MapURL, false);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, FName(*MapURL));
	}
}
