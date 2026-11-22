#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Data/Class/FCClassTypes.h"
#include "FCPlayerState.generated.h"

class UFCCardDeckComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerClassChangedSignature, AFCPlayerState*, PlayerState, FGameplayTag, NewClassTag);

/**
 * AFCPlayerState
 * 
 * Replicated player state containing player combat, character class (FGameplayTag), and deck data,
 * holding the modular UFCCardDeckComponent for authoritative card & zone logic.
 */
UCLASS()
class FC_API AFCPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFCPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* NewPlayerState) override;

	/** Delegate fired when player character class changes */
	UPROPERTY(BlueprintAssignable, Category = "Class|Events")
	FOnPlayerClassChangedSignature OnPlayerClassChanged;

	UFUNCTION(BlueprintPure, Category = "Card")
	UFCCardDeckComponent* GetCardDeckComponent() const { return CardDeckComponent; }

	UFUNCTION(BlueprintPure, Category = "Class")
	FGameplayTag GetCharacterClass() const { return CharacterClass; }

	/** Server-authoritative character class setter (Class.*) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Class")
	void SetCharacterClass(FGameplayTag NewClassTag);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCCardDeckComponent> CardDeckComponent;

	/** Replicated player class GameplayTag (Class.*) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CharacterClass, Category = "Class")
	FGameplayTag CharacterClass;

	UFUNCTION()
	virtual void OnRep_CharacterClass();
};
