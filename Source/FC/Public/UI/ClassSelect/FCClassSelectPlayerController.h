#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/Class/FCClassTypes.h"
#include "FCClassSelectPlayerController.generated.h"

class UFCClassSelectWidget;
class UFCClassSelectViewModel;

/**
 * AFCClassSelectPlayerController
 * 
 * Specialized PlayerController for the Class Selection Level.
 * Enforces UI-only input mode with mouse cursor, instantiates the master WBP_ClassSelect widget,
 * and handles transition triggers when class selection is confirmed.
 */
UCLASS()
class FC_API AFCClassSelectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFCClassSelectPlayerController();

	virtual void BeginPlay() override;

	/** Widget class to instantiate for the Class Selection screen (WBP_ClassSelect) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|ClassSelect|UI")
	TSubclassOf<UFCClassSelectWidget> ClassSelectWidgetClass;

	/** Returns the active Class Selection Widget instance */
	UFUNCTION(BlueprintPure, Category = "FC|ClassSelect|UI")
	UFCClassSelectWidget* GetClassSelectWidget() const { return ClassSelectWidget; }

	/** Returns the active Class Selection ViewModel instance */
	UFUNCTION(BlueprintPure, Category = "FC|ClassSelect|UI")
	UFCClassSelectViewModel* GetClassSelectViewModel() const { return ClassSelectViewModel; }

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FC|ClassSelect|UI")
	TObjectPtr<UFCClassSelectWidget> ClassSelectWidget;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "FC|ClassSelect|UI")
	TObjectPtr<UFCClassSelectViewModel> ClassSelectViewModel;

	UFUNCTION()
	void HandleClassConfirmed(EFCCharacterClass ConfirmedClass, const TArray<FName>& StartingDeck);
};
