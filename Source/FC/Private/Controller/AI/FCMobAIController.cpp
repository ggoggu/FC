#include "Controller/AI/FCMobAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Character/Mob/FCMobCharacter.h"
#include "Character/Player/FCPlayerCharacter.h"
#include "GameFramework/PlayerController.h"

AFCMobAIController::AFCMobAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize AI Perception Component
	MobPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("MobPerceptionComponent"));

	// Initialize and configure Sight Sense
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	if (SightConfig)
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(5.0f);

		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		MobPerceptionComponent->ConfigureSense(*SightConfig);
		MobPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}

void AFCMobAIController::BeginPlay()
{
	Super::BeginPlay();

	if (MobPerceptionComponent)
	{
		MobPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AFCMobAIController::HandleTargetPerceptionUpdated);
	}
}

void AFCMobAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = Blackboard;

	// Resolve Blackboard asset
	UBlackboardData* DesiredBlackboard = BlackboardAsset;
	if (!DesiredBlackboard && BehaviorTreeAsset)
	{
		DesiredBlackboard = BehaviorTreeAsset->BlackboardAsset;
	}

	if (DesiredBlackboard)
	{
		UseBlackboard(DesiredBlackboard, BlackboardComp);
		Blackboard = BlackboardComp;
	}

	// Initialize Blackboard values
	if (Blackboard)
	{
		Blackboard->SetValueAsVector(FFCMobBlackboardKeys::HomeLocation, InPawn->GetActorLocation());
	}
	SetAIState(EFCMobAIState::Patrol);

	// Run Behavior Tree
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}

	if (AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(InPawn))
	{
		MobChar->SetPatrolSpeed();
	}
}

void AFCMobAIController::OnUnPossess()
{
	Super::OnUnPossess();

	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Mob UnPossessed"));
	}
}

bool AFCMobAIController::IsTargetViablePlayer(AActor* InActor) const
{
	if (!InActor || InActor == GetPawn())
	{
		return false;
	}

	// Explicit Player Character type check
	if (InActor->IsA<AFCPlayerCharacter>())
	{
		return true;
	}

	// Tag check
	if (InActor->ActorHasTag(TEXT("Player")))
	{
		return true;
	}

	// Generic Player controlled pawn check
	if (const APawn* AsPawn = Cast<APawn>(InActor))
	{
		if (AsPawn->IsPlayerControlled())
		{
			return true;
		}
	}

	return false;
}

void AFCMobAIController::SetAIState(EFCMobAIState NewState)
{
	CurrentAIState = NewState;
	if (Blackboard)
	{
		Blackboard->SetValueAsEnum(FFCMobBlackboardKeys::AIState, static_cast<uint8>(NewState));
	}

	if (AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(GetPawn()))
	{
		MobChar->SetAIState(NewState);
	}
}

void AFCMobAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || Actor == GetPawn())
	{
		return;
	}

	if (!IsTargetViablePlayer(Actor))
	{
		return;
	}

	AFCMobCharacter* MobChar = Cast<AFCMobCharacter>(GetPawn());

	if (Stimulus.WasSuccessfullySensed())
	{
		// Player detected in sight: start chasing
		if (Blackboard)
		{
			Blackboard->SetValueAsObject(FFCMobBlackboardKeys::TargetActor, Actor);
			Blackboard->SetValueAsVector(FFCMobBlackboardKeys::LastKnownLocation, Actor->GetActorLocation());
		}
		if (CurrentAIState != EFCMobAIState::Attacking)
		{
			SetAIState(EFCMobAIState::Chasing);
		}

		if (MobChar)
		{
			MobChar->SetChaseSpeed();
		}
	}
	else
	{
		// Player left sight: save last known location & direction
		if (Blackboard)
		{
			UObject* CurrentTarget = Blackboard->GetValueAsObject(FFCMobBlackboardKeys::TargetActor);
			if (CurrentTarget == Actor)
			{
				Blackboard->ClearValue(FFCMobBlackboardKeys::TargetActor);

				FVector LastLocation = Stimulus.StimulusLocation;
				if (LastLocation.IsNearlyZero())
				{
					LastLocation = Actor->GetActorLocation();
				}

				// Project slightly towards player velocity to head where the player went
				const FVector Velocity = Actor->GetVelocity();
				if (!Velocity.IsNearlyZero())
				{
					LastLocation += Velocity.GetSafeNormal() * 300.0f;
				}

				Blackboard->SetValueAsVector(FFCMobBlackboardKeys::LastKnownLocation, LastLocation);
			}
		}
		SetAIState(EFCMobAIState::Investigating);
	}
}
