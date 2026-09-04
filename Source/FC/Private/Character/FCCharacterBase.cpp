#include "Character/FCCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FCAttributeSet.h"
#include "Combat/Element/FCElementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AFCCharacterBase::AFCCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UFCAttributeSet>(TEXT("AttributeSet"));

	ElementComponent = CreateDefaultSubobject<UFCElementComponent>(TEXT("ElementComponent"));
}

UAbilitySystemComponent* AFCCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UFCAttributeSet* AFCCharacterBase::GetAttributeSet() const
{
	return AttributeSet;
}

UFCElementComponent* AFCCharacterBase::GetElementComponent() const
{
	return ElementComponent;
}

void AFCCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AFCCharacterBase::Die(AActor* Killer)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	OnDeath.Broadcast(this, Killer);

	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
}

