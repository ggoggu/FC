#include "UI/ViewModel/FCCardRewardViewModel.h"
#include "UI/ViewModel/FCCardViewModel.h"
#include "Data/Card/FCCardDataAsset.h"
#include "Data/Card/FCCardSubsystem.h"
#include "Data/Card/FCCardHandContainer.h"

UFCCardRewardViewModel::UFCCardRewardViewModel()
{
	PromptTitle = FText::FromString(TEXT("새로운 카드 발견!"));
	PromptDescription = FText::FromString(TEXT("카드를 덱에 추가하거나 버릴 수 있습니다."));
	AcquireActionText = FText::FromString(TEXT("[SpaceBar] 획득"));
	DiscardActionText = FText::FromString(TEXT("[Escape] 버리기"));
	bIsVisible = false;
}

void UFCCardRewardViewModel::SetCardViewModel(UFCCardViewModel* InVM)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(CardViewModel, InVM))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CardViewModel);
	}
}

void UFCCardRewardViewModel::SetupReward(FName InCardId, UFCCardSubsystem* Subsystem, const FKey& InAcquireKey, const FKey& InDiscardKey)
{
	if (!Subsystem)
	{
		return;
	}

	UFCCardDataAsset* DataAsset = Subsystem->GetCardDataAsset(InCardId);
	if (!DataAsset)
	{
		return;
	}

	// Create child CardViewModel with this as Outer for automatic lifecycle management
	UFCCardViewModel* NewCardVM = NewObject<UFCCardViewModel>(this);
	if (NewCardVM)
	{
		FFCCardItem DummyItem(FGuid::NewGuid(), InCardId, 0, false);
		NewCardVM->InitializeFromCardItem(DummyItem, DataAsset);
		SetCardViewModel(NewCardVM);
	}

	SetPromptTitle(FText::FromString(TEXT("새로운 카드 발견!")));
	SetPromptDescription(FText::FromString(TEXT("카드를 덱에 추가하거나 버릴 수 있습니다.")));
	SetAcquireActionText(FormatActionShortcut(InAcquireKey, FText::FromString(TEXT("획득"))));
	SetDiscardActionText(FormatActionShortcut(InDiscardKey, FText::FromString(TEXT("버리기"))));
	SetIsVisible(true);
}

FText UFCCardRewardViewModel::FormatActionShortcut(const FKey& InKey, const FText& ActionName)
{
	if (InKey.IsValid())
	{
		return FText::Format(FText::FromString(TEXT("[{0}] {1}")), InKey.GetDisplayName(), ActionName);
	}
	return ActionName;
}
