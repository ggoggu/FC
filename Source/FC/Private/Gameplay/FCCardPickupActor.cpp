#include "Gameplay/FCCardPickupActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/UserWidget.h"
#include "UI/View/FCCardRewardWidget.h"
#include "Card/FCCardDeckComponent.h"
#include "Game/FCPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Net/UnrealNetwork.h"

AFCCardPickupActor::AFCCardPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// Root Overlap Sphere
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapSphere"));
	OverlapSphere->InitSphereRadius(150.0f);
	OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = OverlapSphere;

	// Visual Mesh
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Idle Rotating Movement
	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);
}

void AFCCardPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFCCardPickupActor, DropCardId);
	DOREPLIFETIME(AFCCardPickupActor, bIsClaimed);
}

void AFCCardPickupActor::BeginPlay()
{
	Super::BeginPlay();

	if (OverlapSphere)
	{
		OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &AFCCardPickupActor::OnOverlapBegin);
	}
}

void AFCCardPickupActor::SetDropCardId(FName InCardId)
{
	if (HasAuthority())
	{
		DropCardId = InCardId;
	}
}

void AFCCardPickupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsClaimed || !OtherActor)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn && Pawn->IsLocallyControlled())
	{
		ShowRewardWidgetForPlayer(Pawn);
	}
}

void AFCCardPickupActor::ShowRewardWidgetForPlayer(APawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(PlayerPawn->GetController());
	if (!PC)
	{
		return;
	}

	UFCCardRewardWidget* RewardWidget = nullptr;
	if (RewardWidgetClass)
	{
		RewardWidget = CreateWidget<UFCCardRewardWidget>(PC, RewardWidgetClass);
	}
	else
	{
		RewardWidget = CreateWidget<UFCCardRewardWidget>(PC, UFCCardRewardWidget::StaticClass());
	}

	if (RewardWidget)
	{
		RewardWidget->AddToViewport(100);
		RewardWidget->SetupRewardWidget(DropCardId, this);
	}
}

bool AFCCardPickupActor::Server_ClaimPickup_Validate(APawn* ClaimerPawn, EFCCardAddDestination Destination)
{
	return !bIsClaimed && !DropCardId.IsNone() && ClaimerPawn != nullptr;
}

void AFCCardPickupActor::Server_ClaimPickup_Implementation(APawn* ClaimerPawn, EFCCardAddDestination Destination)
{
	if (!HasAuthority() || bIsClaimed || !ClaimerPawn)
	{
		return;
	}

	bIsClaimed = true;

	if (AFCPlayerState* PS = ClaimerPawn->GetPlayerState<AFCPlayerState>())
	{
		if (UFCCardDeckComponent* DeckComp = PS->GetCardDeckComponent())
		{
			DeckComp->AddCardToDeck(DropCardId, Destination);
		}
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	if (PickupVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupVFX, GetActorLocation(), GetActorRotation());
	}

	Destroy();
}

void AFCCardPickupActor::OnRep_DropCardId()
{
	// Visual update hook when replicated to clients
}
