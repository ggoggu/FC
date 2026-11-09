#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "UI/Data/FCStatusToggleTypes.h"
#include "FCStatusPresenterComponent.generated.h"

class UAbilitySystemComponent;
class UFCStatusToggleContainerViewModel;
class UDataTable;

/**
 * UFCStatusPresenterComponent
 * 
 * Client-side Presentation Adapter Component.
 * Observes Gameplay Ability System attributes, tags, and effects,
 * translates them into UI status toggle metadata via DT_StatusToggleCatalog,
 * and pushes updates to UFCStatusToggleContainerViewModel.
 */
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class FC_API UFCStatusPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFCStatusPresenterComponent();

	/** Binds this presenter to target ASC and target ViewModel container */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	void BindToAbilitySystem(UAbilitySystemComponent* InASC, UFCStatusToggleContainerViewModel* InTargetViewModel);

	/** Unbinds all attribute and tag listeners from active ASC */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	void UnbindFromAbilitySystem();

	/** Looks up status definition row by unique status ID */
	UFUNCTION(BlueprintPure, Category = "FC|Status|Presenter")
	bool FindDefinitionById(FName InStatusId, FFCStatusToggleDefinitionRow& OutRow) const;

	/** Looks up status definition row by GameplayTag */
	UFUNCTION(BlueprintPure, Category = "FC|Status|Presenter")
	bool FindDefinitionByTag(const FGameplayTag& InTag, FFCStatusToggleDefinitionRow& OutRow) const;

	/** Adds or updates a status toggle using metadata from DT_StatusToggleCatalog */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	bool AddStatusFromCatalog(FName InStatusId, int32 InStackCount = 1, float InRemaining = 0.0f, float InTotal = 0.0f, bool bInActive = true);

	/** Adds or updates a status toggle by tag using metadata from DT_StatusToggleCatalog */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	bool AddStatusFromCatalogByTag(const FGameplayTag& InTag, int32 InStackCount = 1, float InRemaining = 0.0f, float InTotal = 0.0f, bool bInActive = true);

	/** Removes a status toggle by StatusId */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	bool RemoveStatusById(FName InStatusId);

	/** Removes a status toggle by GameplayTag */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	bool RemoveStatusByTag(const FGameplayTag& InTag);

	/** Directly pushes explicit status data to the target ViewModel */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	void PushStatusData(const FFCStatusToggleData& InData);

	/** Sets catalog data table reference */
	UFUNCTION(BlueprintCallable, Category = "FC|Status|Presenter")
	void SetCatalogDataTable(UDataTable* InTable) { CatalogDataTable = InTable; }

	UFUNCTION(BlueprintPure, Category = "FC|Status|Presenter")
	UDataTable* GetCatalogDataTable() const { return CatalogDataTable; }

	UFUNCTION(BlueprintPure, Category = "FC|Status|Presenter")
	UFCStatusToggleContainerViewModel* GetBoundViewModel() const { return TargetViewModel; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Status catalog data table defining visual properties (SSOT) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Status|Config")
	TObjectPtr<UDataTable> CatalogDataTable;

	/** Fallback status catalog table path if not assigned */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Status|Config")
	FSoftObjectPath DefaultCatalogTablePath;

	/** Whether to automatically manage Shield guard toggle from Shield attribute */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Status|Config")
	bool bAutoManageShieldToggle = true;

	/** Whether to automatically manage Attack Power buff/debuff toggle */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FC|Status|Config")
	bool bAutoManageAttackBuffToggle = true;

	/** Active target presentation ViewModel */
	UPROPERTY(Transient)
	TObjectPtr<UFCStatusToggleContainerViewModel> TargetViewModel;

	/** Active bound AbilitySystemComponent */
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	/** Registered attribute listener delegate handles */
	TArray<TTuple<FGameplayAttribute, FDelegateHandle>> BoundAttributeHandles;

	/** Registered tag listener delegate handles */
	TMap<FGameplayTag, FDelegateHandle> BoundTagHandles;

	/** Ensures CatalogDataTable is loaded and valid */
	void EnsureCatalogLoaded();

	/** Internal handler for Shield attribute changes */
	void HandleShieldChanged(float NewShield, float OldShield);

	/** Internal handler for AttackPower attribute changes */
	void HandleAttackPowerChanged(float NewAttackPower, float OldAttackPower);

	/** Internal handler for registered GameplayTags */
	void HandleGameplayTagChanged(const FGameplayTag Tag, int32 NewCount);
};
