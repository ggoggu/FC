#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "FCPlayerState.generated.h"

class UFCCardDeckComponent;

/**
 * AFCPlayerState
 * 
 * Replicated player state containing player combat and deck data,
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

	UFUNCTION(BlueprintPure, Category = "Card")
	UFCCardDeckComponent* GetCardDeckComponent() const { return CardDeckComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UFCCardDeckComponent> CardDeckComponent;
};
