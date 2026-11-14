// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDSceneActors.h"

#include "Actors/TDSystemModule.h"
#include "CineCameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"
#include "TimerManager.h"
#include "UI/TDDesignTokens.h"
#include "UObject/ConstructorHelpers.h"

// ATDCameraRig

ATDCameraRig::ATDCameraRig()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Camera = CreateDefaultSubobject<UCineCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
}

void ATDCameraRig::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetViewTarget(this);
	}
	PlayPushIn();
}

void ATDCameraRig::PlayPushIn()
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if ((Mission && (Mission->IsReduceMotion() || Mission->bOpeningShown)) || PushInSeconds <= 0.f)
	{
		Camera->SetRelativeLocation(FVector::ZeroVector);
		return;
	}
	PushElapsed = 0.f;
	Camera->SetRelativeLocation(FVector(-PushInDistance, 0.f, 0.f));
	GetWorldTimerManager().SetTimer(PushTimer, this, &ATDCameraRig::StepPushIn, 1.f / 60.f, true);
}

void ATDCameraRig::StepPushIn()
{
	PushElapsed += 1.f / 60.f;
	const float Alpha = FMath::Clamp(PushElapsed / PushInSeconds, 0.f, 1.f);
	const float Eased = 1.f - FMath::Pow(1.f - Alpha, 3.f);
	Camera->SetRelativeLocation(FVector(-PushInDistance * (1.f - Eased), 0.f, 0.f));
	if (Alpha >= 1.f)
	{
		GetWorldTimerManager().ClearTimer(PushTimer);
	}
}

// ATDPowerUnit

ATDPowerUnit::ATDPowerUnit()
{
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	if (Cylinder.Succeeded())
	{
		Body->SetStaticMesh(Cylinder.Object);
	}
	Body->SetRelativeScale3D(FVector(2.f, 2.f, 3.f));
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		UStaticMeshComponent* Line = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Line%d"), Index));
		Line->SetupAttachment(RootComponent);
		Line->SetUsingAbsoluteScale(true);
		Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Cylinder.Succeeded())
		{
			Line->SetStaticMesh(Cylinder.Object);
		}
		Lines.Add(Line);
	}
}

void ATDPowerUnit::BeginPlay()
{
	Super::BeginPlay();
	Targets.Init(GetActorLocation(), TD::NumSystems);
	for (TActorIterator<ATDSystemModule> It(GetWorld()); It; ++It)
	{
		Targets[static_cast<int32>(It->System)] = It->GetActorLocation();
	}
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		if (UMaterialInstanceDynamic* Material = Lines[Index]->CreateAndSetMaterialInstanceDynamic(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), TDTokens::System(static_cast<ETDSystem>(Index)));
		}
	}
	LayOutLines();
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Mission->OnAllocationChanged.AddDynamic(this, &ATDPowerUnit::HandleAllocation);
	}
}

void ATDPowerUnit::LayOutLines()
{
	const FVector From = GetActorLocation() + FVector(0.f, 0.f, 20.f);
	for (int32 Index = 0; Index < TD::NumSystems; ++Index)
	{
		const FVector To = Targets[Index] + FVector(0.f, 0.f, 20.f);
		const FVector Mid = (From + To) * 0.5f;
		const FVector Dir = (To - From);
		Lines[Index]->SetWorldLocation(Mid);
		// The engine cylinder is 100 units tall along Z; rotate Z onto the line direction.
		Lines[Index]->SetWorldRotation(FRotationMatrix::MakeFromZ(Dir.GetSafeNormal()).Rotator());
		const FVector Scale = Lines[Index]->GetComponentScale();
		Lines[Index]->SetWorldScale3D(FVector(FMath::Max(Scale.X, MinThickness), FMath::Max(Scale.Y, MinThickness), Dir.Size() / 100.f));
	}
}

void ATDPowerUnit::HandleAllocation(const FTDMissionState& State)
{
	for (int32 Index = 0; Index < TD::NumSystems && State.Allocation.IsValidIndex(Index); ++Index)
	{
		const float Thickness = MinThickness + ThicknessPerCell * State.Allocation[Index];
		const FVector Scale = Lines[Index]->GetComponentScale();
		Lines[Index]->SetWorldScale3D(FVector(Thickness, Thickness, Scale.Z));
	}
}

// ATDCrewMember

ATDCrewMember::ATDCrewMember()
{
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Capsule(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	RootComponent = Body;
	if (Capsule.Succeeded())
	{
		Body->SetStaticMesh(Capsule.Object);
	}
	// 180 cm crew in suits (level-design.md scale standard).
	Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.8f));
}

void ATDCrewMember::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Material = Body->CreateAndSetMaterialInstanceDynamic(0);
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), TDTokens::System(System));
	}
}

void ATDCrewMember::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	IdleSeed = static_cast<float>(static_cast<int32>(System)) * 2.1f; // fixed phase per crew member, no random stream
	SetBasePose(GetActorLocation(), GetActorRotation());
	GetWorldTimerManager().SetTimer(IdleTimer, this, &ATDCrewMember::StepIdle, 1.f / 30.f, true);
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Mission->OnLevelsChanged.AddDynamic(this, &ATDCrewMember::HandleLevels);
		Mission->OnEventResolved.AddDynamic(this, &ATDCrewMember::HandleEventResolved);
		Mission->OnMissionEnded.AddDynamic(this, &ATDCrewMember::HandleEnded);
		Mission->OnPhaseStarted.AddDynamic(this, &ATDCrewMember::HandlePhaseStarted);
	}
	if (UTDShotDirector* Director = UTDShotDirector::Get(this))
	{
		Director->OnShotStarted.AddDynamic(this, &ATDCrewMember::HandleShotStarted);
		Director->OnShotEnded.AddDynamic(this, &ATDCrewMember::HandleShotEnded);
	}
}

void ATDCrewMember::HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState)
{
	const FString SystemName = StaticEnum<ETDSystem>()->GetNameStringByValue(static_cast<int64>(System));
	const FString Base = FString::Printf(TEXT("Stage_%s_%s"), *CameraTag.ToString(), *SystemName);
	TArray<FName> StageTags;
	if (OutcomeState == 0 || OutcomeState == 1)
	{
		StageTags.Add(FName(*(Base + (OutcomeState == 0 ? TEXT("_A") : TEXT("_B")))));
	}
	StageTags.Add(FName(*Base));
	for (const FName& Tag : StageTags)
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				PreShotTransform = FTransform(BaseRotation, BaseLocation);
				bStaged = true;
				SetBasePose(It->GetActorLocation(), It->GetActorRotation());
				return;
			}
		}
	}
}

void ATDCrewMember::HandleShotEnded(FName ShotId)
{
	if (bStaged)
	{
		bStaged = false;
		SetBasePose(PreShotTransform.GetLocation(), PreShotTransform.Rotator());
	}
}

void ATDCrewMember::HandlePhaseStarted(ETDPhase Phase)
{
	SetBasePose(HomeLocation, BaseRotation);
}

void ATDCrewMember::SetBasePose(const FVector& Location, const FRotator& Rotation)
{
	BaseLocation = Location;
	BaseRotation = Rotation;
	SetActorLocationAndRotation(Location, Rotation);
}

void ATDCrewMember::StepIdle()
{
	IdleTime += 1.f / 30.f;
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (Mission && Mission->IsReduceMotion())
	{
		SetActorLocationAndRotation(BaseLocation, BaseRotation);
		return;
	}
	float SwayDegrees = 7.f;
	float SwaySpeed = 0.5f;
	float LiftCm = 1.2f * FMath::Sin(IdleTime * 1.4f + IdleSeed); // breathing
	switch (Current)
	{
	case ETDCrewBehaviour::Worried: SwayDegrees = 12.f; SwaySpeed = 1.1f; break;
	case ETDCrewBehaviour::Alarm: SwayDegrees = 18.f; SwaySpeed = 2.0f; break;
	case ETDCrewBehaviour::Celebrate: LiftCm = 28.f * FMath::Abs(FMath::Sin(IdleTime * 3.2f + IdleSeed)); break; // small hops
	default: break;
	}
	const float Yaw = SwayDegrees * FMath::Sin(IdleTime * SwaySpeed + IdleSeed);
	SetActorLocationAndRotation(BaseLocation + FVector(0.f, 0.f, LiftCm), BaseRotation + FRotator(0.f, Yaw, 0.f));
}

void ATDCrewMember::HandleLevels(const FTDMissionState& State, const TArray<ETDSystem>& Changed)
{
	if (State.Result != ETDMissionResult::InProgress || Current == ETDCrewBehaviour::Shelter)
	{
		return;
	}
	const int32 Level = State.Levels.IsValidIndex(static_cast<int32>(System)) ? State.Levels[static_cast<int32>(System)] : 3;
	PlayBehaviour(Level >= 3 ? ETDCrewBehaviour::Working : (Level == 2 ? ETDCrewBehaviour::Worried : ETDCrewBehaviour::Alarm));
}

void ATDCrewMember::HandleEventResolved(FName EventId, const FText& Cause)
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const FTDEventRow* Event = Mission ? Mission->GetEvent(EventId) : nullptr;
	if (Event && Event->Family == ETDEventFamily::Radiation && Mission->GetState().EventChoice == 0)
	{
		PlayBehaviour(ETDCrewBehaviour::Shelter);
	}
}

void ATDCrewMember::HandleEnded(ETDMissionResult Result)
{
	PlayBehaviour(Result == ETDMissionResult::LostAir ? ETDCrewBehaviour::AtLander : ETDCrewBehaviour::Celebrate);
}

void ATDCrewMember::PlayBehaviour(ETDCrewBehaviour Behaviour)
{
	Current = Behaviour;
	FLinearColor Color = TDTokens::System(System);
	FVector Location = HomeLocation;
	switch (Behaviour)
	{
	case ETDCrewBehaviour::Worried: Color = TDTokens::SemLow(); break;
	case ETDCrewBehaviour::Alarm: Color = TDTokens::SemDanger(); break;
	case ETDCrewBehaviour::Shelter: Location = HomeLocation + FVector(0.f, 350.f, 0.f); break;
	case ETDCrewBehaviour::AtLander: Location = LanderLocation; break;
	case ETDCrewBehaviour::Celebrate: Color = TDTokens::CoreAccent(); break; // hops in StepIdle
	default: break;
	}
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
	if (!bStaged)
	{
		SetBasePose(Location, BaseRotation);
	}
	OnBehaviour(Behaviour);
}
