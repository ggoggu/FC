#include "Game/FCPlayerState.h"
#include "Card/FCCardDeckComponent.h"
#include "Data/FCGameplayTags.h"
#include "Net/UnrealNetwork.h"

AFCPlayerState::AFCPlayerState()
{
	bReplicates = true;
	SetNetUpdateFrequency(30.0f);

	CharacterClass = FCGameplayTags::Class_Mage;
	CardDeckComponent = CreateDefaultSubobject<UFCCardDeckComponent>(TEXT("CardDeckComponent"));
}

void AFCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AFCPlayerState, CharacterClass, COND_None);
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

void AFCPlayerState::SetCharacterClass(FGameplayTag NewClassTag)
{
	if (!HasAuthority())
	{
		return;
	}

	const FGameplayTag EffectiveTag = NewClassTag.IsValid() ? NewClassTag : FCGameplayTags::Class_Mage.GetTag();
	if (CharacterClass != EffectiveTag)
	{
		CharacterClass = EffectiveTag;
		OnPlayerClassChanged.Broadcast(this, CharacterClass);
	}
}

void AFCPlayerState::OnRep_CharacterClass()
{
	OnPlayerClassChanged.Broadcast(this, CharacterClass);
}
