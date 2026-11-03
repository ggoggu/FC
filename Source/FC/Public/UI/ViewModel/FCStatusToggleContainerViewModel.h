#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "UI/Data/FCStatusToggleTypes.h"
#include "FCStatusToggleContainerViewModel.generated.h"

class UFCStatusToggleItemViewModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatusItemsListChangedSignature);

/**
 * UFCStatusToggleContainerViewModel
 * 
 * Manages the dynamic list of active status toggle ViewModels.
 * Dispatches FieldNotify and broadcast events when items are added, updated, or removed.
 */
UCLASS(BlueprintType)
class FC_API UFCStatusToggleContainerViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Optional dynamic delegate when status collection changes */
	UPROPERTY(BlueprintAssignable, Category = "FC|Status")
	FOnStatusItemsListChangedSignature OnStatusListUpdated;

	/** Collection of active status toggle item ViewModels */
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "FC|Status")
	TArray<TObjectPtr<UFCStatusToggleItemViewModel>> StatusItems;

public:
	/** Adds a new status or updates an existing status with matching StatusTag or StatusId */
	UFUNCTION(BlueprintCallable, Category = "FC|Status")
	UFCStatusToggleItemViewModel* AddOrUpdateStatus(const FFCStatusToggleData& InData);

	/** Removes a status by StatusId */
	UFUNCTION(BlueprintCallable, Category = "FC|Status")
	bool RemoveStatusById(FName InStatusId);

	/** Removes a status by GameplayTag */
	UFUNCTION(BlueprintCallable, Category = "FC|Status")
	bool RemoveStatusByTag(const FGameplayTag& InTag);

	/** Clears all active status toggles */
	UFUNCTION(BlueprintCallable, Category = "FC|Status")
	void ClearAllStatuses();

	/** Finds an item ViewModel by StatusId */
	UFUNCTION(BlueprintPure, Category = "FC|Status")
	UFCStatusToggleItemViewModel* FindStatusById(FName InStatusId) const;

	/** Finds an item ViewModel by GameplayTag */
	UFUNCTION(BlueprintPure, Category = "FC|Status")
	UFCStatusToggleItemViewModel* FindStatusByTag(const FGameplayTag& InTag) const;

	/** Sets active toggle flag for an existing status */
	UFUNCTION(BlueprintCallable, Category = "FC|Status")
	bool SetStatusActive(FName InStatusId, bool bActive);

	/** Updates stack count for an existing status */
	UFUNCTION(BlueprintCallable, Category = "FC|Status")
	bool SetStatusStack(FName InStatusId, int32 InStack);

	/** Updates remaining and total duration for an existing status */
	UFUNCTION(BlueprintCallable, Category = "FC|Status")
	bool SetStatusDuration(FName InStatusId, float InRemaining, float InTotal);

	/** Number of active status items */
	UFUNCTION(BlueprintPure, Category = "FC|Status")
	int32 GetStatusCount() const { return StatusItems.Num(); }
};
