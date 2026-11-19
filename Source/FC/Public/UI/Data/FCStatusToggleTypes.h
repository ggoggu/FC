#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "FCStatusToggleTypes.generated.h"

/**
 * FFCStatusToggleData
 * 
 * Data-driven representation of a circular status toggle (shield, buff, debuff, passive, mode).
 * Decoupled from gameplay logic, focusing strictly on presentation parameters.
 */
USTRUCT(BlueprintType)
struct FC_API FFCStatusToggleData
{
	GENERATED_BODY()

	/** Unique status identifier or GameplayTag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FGameplayTag StatusTag;

	/** Fallback unique Name if GameplayTags are not used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FName StatusId = NAME_None;

	/** Localized display title / tooltip header */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FText DisplayName;

	/** Circular status icon (soft referenced to avoid synchronous stalls) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Outer circular ring / glow tint color (e.g. Cyan for Shield, Red for Debuff, Gold for Buff) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FLinearColor BorderColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f);

	/** Initial active state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bIsActive = true;

	/** Current stack or charge count (0 or negative hides the number) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	int32 StackCount = 0;

	/** Current remaining duration in seconds (<= 0.0f means permanent or untimed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float RemainingDuration = 0.0f;

	/** Total duration in seconds for progress ratio calculation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float TotalDuration = 0.0f;

	/** Detailed tooltip explanation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FText Description;
};

/**
 * FFCStatusToggleDefinitionRow
 * 
 * Data table row definition for status toggles (SSOT catalog).
 * Defines design visual attributes and metadata.
 */
USTRUCT(BlueprintType)
struct FC_API FFCStatusToggleDefinitionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique GameplayTag associated with this status effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FGameplayTag StatusTag;

	/** Fallback status identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FName StatusId = NAME_None;

	/** Display title for UI tooltip header */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FText DisplayName;

	/** Soft pointer to status icon texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Circular border ring / glow tint color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FLinearColor BorderColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);

	/** Whether circular progress gauge is shown for duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bShowCircularProgress = false;

	/** Whether stack count text is shown */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bShowStackCount = true;

	/** Localized tooltip description text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FText DefaultDescription;

	/** Helper converting catalog row to instance data payload */
	FFCStatusToggleData ToStatusToggleData(int32 InStack = 1, float InRemaining = 0.0f, float InTotal = 0.0f, bool bInActive = true) const
	{
		FFCStatusToggleData Data;
		Data.StatusTag = StatusTag;
		Data.StatusId = StatusId;
		Data.DisplayName = DisplayName;
		Data.Icon = Icon;
		Data.BorderColor = BorderColor;
		Data.bIsActive = bInActive;
		Data.StackCount = bShowStackCount ? InStack : 0;
		Data.RemainingDuration = InRemaining;
		Data.TotalDuration = InTotal;
		Data.Description = DefaultDescription;
		return Data;
	}
};
