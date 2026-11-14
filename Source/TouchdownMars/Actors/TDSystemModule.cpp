// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDSystemModule.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"
#include "UI/TDDesignTokens.h"
#include "UObject/ConstructorHelpers.h"

ATDSystemModule::ATDSystemModule()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Mesh);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	Light->SetIntensityUnits(ELightUnits::Lumens);
	Light->SetAttenuationRadius(600.f);
}

void ATDSystemModule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const FLinearColor Color = TDTokens::System(System);
	Light->SetLightColor(Color);
	if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ATDSystemModule::BeginPlay()
{
	Super::BeginPlay();
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Mission->OnLevelsChanged.AddDynamic(this, &ATDSystemModule::HandleLevelsChanged);
		Mission->OnAllocationChanged.AddDynamic(this, &ATDSystemModule::HandleAllocation);
	}
	Light->SetIntensity(0.f);
	if (UTDShotDirector* Director = UTDShotDirector::Get(this))
	{
		Director->OnShotStarted.AddDynamic(this, &ATDSystemModule::HandleShotStarted);
		Director->OnShotEnded.AddDynamic(this, &ATDSystemModule::HandleShotEnded);
	}
}

void ATDSystemModule::HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState)
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const int32 Index = static_cast<int32>(System);
	if (!DimWhenUnpoweredShots.Contains(ShotId) || !Mission)
	{
		return;
	}
	const TArray<int32>& Allocation = Mission->GetState().Allocation;
	if (Allocation.IsValidIndex(Index) && Allocation[Index] == 0)
	{
		bShotDimmed = true;
		UpdateLight();
	}
}

void ATDSystemModule::HandleShotEnded(FName ShotId)
{
	if (bShotDimmed)
	{
		bShotDimmed = false;
		UpdateLight(); // after the storm the base looks normal again
	}
}

void ATDSystemModule::HandleAllocation(const FTDMissionState& State)
{
	const int32 Index = static_cast<int32>(System);
	// Dark base at the opening: the light comes on with this system's first cell, or from phase 2 on.
	const bool bShouldLight = State.PhaseIndex > 0 || (State.Allocation.IsValidIndex(Index) && State.Allocation[Index] > 0);
	if (bShouldLight && !bLit)
	{
		bLit = true;
		UpdateLight();
	}
	else if (!bShouldLight && bLit && State.PhaseIndex == 0 && State.SolsElapsed == 0)
	{
		bLit = false; // restart of the mission
		UpdateLight();
	}
}

void ATDSystemModule::UpdateLight()
{
	const float Intensity = bLit && IntensityByLevel.IsValidIndex(CurrentLevel) ? IntensityByLevel[CurrentLevel] : 0.f;
	Light->SetIntensity(bShotDimmed ? Intensity * ShotDimFactor : Intensity);
}

void ATDSystemModule::HandleLevelsChanged(const FTDMissionState& State, const TArray<ETDSystem>& Changed)
{
	const int32 Index = static_cast<int32>(System);
	if (State.Levels.IsValidIndex(Index))
	{
		ApplyLevel(State.Levels[Index]);
	}
}

void ATDSystemModule::ApplyLevel(int32 Level)
{
	if (Level == CurrentLevel)
	{
		return;
	}
	CurrentLevel = Level;
	UpdateLight();
	OnLevelChanged(Level);
}
