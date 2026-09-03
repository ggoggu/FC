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

	StartingHandSize = 5;
	DefaultCycleInterval = 30.0f;
	DefaultCycleDrawCount = 5;
	bEnableAutoHandCycle = true;
	bUseClassStartingDeck = false;
	DefaultStartingDeck = {
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_Fireball"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff"),
		FName("Card_AttackBuff")
	};
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

			if (AFCPlayerState* PS = NewPlayer->GetPlayerState<AFCPlayerState>())
			{
				if (UFCCardDeckComponent* DeckComp = PS->GetCardDeckComponent())
				{
					DeckComp->SetCycleInterval(DefaultCycleInterval);
					DeckComp->SetCycleDrawCount(DefaultCycleDrawCount);
					DeckComp->SetAutoCycleEnabled(bEnableAutoHandCycle);
					if (bEnableAutoHandCycle)
					{
						DeckComp->StartCycleTimer();
					}
				}
			}
		}
		else
		{
			// Initialize starting deck and hand if new player session
			if (AFCPlayerState* PS = NewPlayer->GetPlayerState<AFCPlayerState>())
			{
				if (UFCCardDeckComponent* DeckComp = PS->GetCardDeckComponent())
				{
					// Configure cycle parameters
					DeckComp->SetCycleInterval(DefaultCycleInterval);
					DeckComp->SetCycleDrawCount(DefaultCycleDrawCount);
					DeckComp->SetAutoCycleEnabled(bEnableAutoHandCycle);

					// Initialize deck: Use Class starting deck if configured, otherwise use GameMode DefaultStartingDeck
					if (bUseClassStartingDeck)
					{
						DeckComp->InitializeDeckForClass(PS->GetCharacterClass());
						if (DeckComp->GetDrawPileCount() == 0 && DefaultStartingDeck.Num() > 0)
						{
							DeckComp->InitializeDeck(DefaultStartingDeck);
						}
					}
					else
					{
						if (DefaultStartingDeck.Num() > 0)
						{
							DeckComp->InitializeDeck(DefaultStartingDeck);
						}
						else
						{
							DeckComp->InitializeDeckForClass(PS->GetCharacterClass());
						}
					}

					// Draw starting hand (5 cards)
					DeckComp->DrawCards(StartingHandSize);

					if (bEnableAutoHandCycle)
					{
						DeckComp->StartCycleTimer();
					}
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
