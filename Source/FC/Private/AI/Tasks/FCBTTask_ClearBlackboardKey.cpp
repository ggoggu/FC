#include "AI/Tasks/FCBTTask_ClearBlackboardKey.h"
#include "BehaviorTree/BlackboardComponent.h"

UFCBTTask_ClearBlackboardKey::UFCBTTask_ClearBlackboardKey()
{
	NodeName = TEXT("Clear Blackboard Key");
}

EBTNodeResult::Type UFCBTTask_ClearBlackboardKey::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	if (KeyToClear.SelectedKeyName != NAME_None)
	{
		BlackboardComp->ClearValue(KeyToClear.SelectedKeyName);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

FString UFCBTTask_ClearBlackboardKey::GetStaticDescription() const
{
	return FString::Printf(TEXT("Clear Blackboard Key: %s"), 
		KeyToClear.SelectedKeyName != NAME_None ? *KeyToClear.SelectedKeyName.ToString() : TEXT("None"));
}
