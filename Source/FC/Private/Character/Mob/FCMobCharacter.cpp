#include "Character/Mob/FCMobCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"

AFCMobCharacter::AFCMobCharacter()
{
	// Mobs generally use Minimal replication mode for AI
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}
}

void AFCMobCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitAbilityActorInfo();
}

void AFCMobCharacter::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}
