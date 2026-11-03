#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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
