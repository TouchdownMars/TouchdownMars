// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDShotProps.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Systems/TDMissionSubsystem.h"
#include "Systems/TDShotDirector.h"
#include "UObject/ConstructorHelpers.h"

// ATDShotProp

ATDShotProp::ATDShotProp()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}
	if (ShapeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ShapeMaterial.Object); // the "Color" parameter lives here
	}
}

void ATDShotProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ATDShotProp::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	SetActorHiddenInGame(bOnlyDuringShot);
	if (UTDShotDirector* Director = UTDShotDirector::Get(this))
	{
		Director->OnShotStarted.AddDynamic(this, &ATDShotProp::HandleShotStarted);
		Director->OnShotEnded.AddDynamic(this, &ATDShotProp::HandleShotEnded);
	}
}

void ATDShotProp::HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState)
{
	if (!ShotIds.Contains(ShotId))
	{
		return;
	}
	bActive = true;
	Elapsed = 0.f;
	SetActorLocation(HomeLocation);
	SetActorHiddenInGame(false);
	SetActorTickEnabled(!TravelOffset.IsNearlyZero());
}

void ATDShotProp::HandleShotEnded(FName ShotId)
{
	if (!bActive)
	{
		return;
	}
	bActive = false;
	SetActorTickEnabled(false);
	SetActorLocation(HomeLocation);
	SetActorHiddenInGame(bOnlyDuringShot);
}

void ATDShotProp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	// Path time with a pause: travel to PauseStart, wait, then finish the path (the reply never arrives in the shot).
	const float T = FMath::Clamp(Elapsed / FMath::Max(TravelSeconds, 0.1f), 0.f, 1.f);
	const float Moving = FMath::Max(1.f - (PauseEnd - PauseStart), 0.1f);
	float Progress;
	if (T < PauseStart)
	{
		Progress = T / Moving;
	}
	else if (T < PauseEnd)
	{
		Progress = PauseStart / Moving;
	}
	else
	{
		Progress = (T - (PauseEnd - PauseStart)) / Moving;
	}
	SetActorLocation(HomeLocation + TravelOffset * FMath::Clamp(Progress, 0.f, 1.f));
}

// ATDPlantRack

namespace
{
	const FLinearColor HealthyGreen(0.10f, 0.42f, 0.12f, 1.f);
	const FLinearColor StressedYellow(0.55f, 0.42f, 0.10f, 1.f);
	constexpr int32 ShelfCount = 2;
	constexpr int32 PlantsPerShelf = 5;
}

ATDPlantRack::ATDPlantRack()
{
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	for (int32 Shelf = 0; Shelf < ShelfCount; ++Shelf)
	{
		// 200 x 50 cm trays at 70 and 140 cm (scale: 1 unit = 1 cm).
		UStaticMeshComponent* Tray = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Shelf%d"), Shelf));
		Tray->SetupAttachment(Root);
		Tray->SetRelativeLocation(FVector(0.f, 0.f, 70.f + Shelf * 70.f));
		Tray->SetRelativeScale3D(FVector(0.5f, 2.f, 0.06f));
		if (Cube.Succeeded()) { Tray->SetStaticMesh(Cube.Object); }
		if (ShapeMaterial.Succeeded()) { Tray->SetMaterial(0, ShapeMaterial.Object); }
		Shelves.Add(Tray);
		for (int32 Index = 0; Index < PlantsPerShelf; ++Index)
		{
			UStaticMeshComponent* Plant = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Plant%d_%d"), Shelf, Index));
			Plant->SetupAttachment(Root);
			Plant->SetRelativeLocation(FVector(0.f, -80.f + Index * 40.f, 70.f + Shelf * 70.f + 18.f));
			Plant->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.32f));
			Plant->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (Sphere.Succeeded()) { Plant->SetStaticMesh(Sphere.Object); }
			if (ShapeMaterial.Succeeded()) { Plant->SetMaterial(0, ShapeMaterial.Object); }
			Plants.Add(Plant);
		}
	}
	RackVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RackVisual"));
	RackVisual->SetupAttachment(Root);
	RackVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GrowLight"));
	GrowLight->SetupAttachment(Root);
	GrowLight->SetRelativeLocation(FVector(-60.f, 0.f, 200.f));
	GrowLight->SetIntensityUnits(ELightUnits::Lumens);
	GrowLight->SetIntensity(250.f);
	GrowLight->SetAttenuationRadius(450.f);
	GrowLight->SetLightColor(FLinearColor(1.f, 0.78f, 0.9f));
}

void ATDPlantRack::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	for (UStaticMeshComponent* Tray : Shelves)
	{
		if (UMaterialInstanceDynamic* Material = Tray->CreateAndSetMaterialInstanceDynamic(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.56f, 0.58f, 1.f));
		}
	}
	for (UStaticMeshComponent* Plant : Plants)
	{
		Plant->CreateAndSetMaterialInstanceDynamic(0);
	}
	RackVisual->SetStaticMesh(RackMesh);
	RackVisual->SetVisibility(RackMesh != nullptr);
	for (UStaticMeshComponent* Tray : Shelves) { Tray->SetVisibility(RackMesh == nullptr); }
	for (UStaticMeshComponent* Plant : Plants) { Plant->SetVisibility(RackMesh == nullptr); }
	if (RackMesh)
	{
		const FBoxSphereBounds Bounds = RackMesh->GetBounds();
		const float Scale = RackHeight / FMath::Max(1.f, Bounds.BoxExtent.Z * 2.f);
		RackVisual->SetRelativeScale3D(FVector(Scale));
		RackVisual->SetRelativeLocation(FVector(0.f, 0.f, (Bounds.BoxExtent.Z - Bounds.Origin.Z) * Scale));
		RackMaterial = RackVisual->CreateAndSetMaterialInstanceDynamic(0);
	}
	ShowHealth(true);
}

void ATDPlantRack::BeginPlay()
{
	Super::BeginPlay();
	ShowHealth(true);
	if (UTDShotDirector* Director = UTDShotDirector::Get(this))
	{
		Director->OnShotStarted.AddDynamic(this, &ATDPlantRack::HandleShotStarted);
	}
}

void ATDPlantRack::HandleShotStarted(FName ShotId, FName CameraTag, int32 OutcomeState)
{
	if (!ShotIds.Contains(ShotId))
	{
		return;
	}
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!Mission || !Mission->HasMission())
	{
		ShowHealth(true);
		return;
	}
	const int32 Index = static_cast<int32>(ETDSystem::Greenhouse);
	const TArray<int32> Demand = Mission->GetDemand();
	const TArray<int32>& Allocation = Mission->GetState().Allocation;
	ShowHealth(Allocation.IsValidIndex(Index) && Demand.IsValidIndex(Index) && Allocation[Index] >= Demand[Index]);
}

void ATDPlantRack::ShowHealth(bool bHealthy)
{
	if (RackMaterial)
	{
		RackMaterial->SetVectorParameterValue(TEXT("BaseColorFactor"), bHealthy ? FLinearColor::White : FLinearColor(1.f, 0.78f, 0.35f, 1.f));
	}
	// Same size either way: stress shows as color and droop only, plants never grow inside one turn.
	for (int32 Index = 0; Index < Plants.Num(); ++Index)
	{
		UStaticMeshComponent* Plant = Plants[Index];
		if (UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Plant->GetMaterial(0)))
		{
			Material->SetVectorParameterValue(TEXT("Color"), bHealthy ? HealthyGreen : StressedYellow);
		}
		const float Side = (Index % 2 == 0) ? 1.f : -1.f;
		Plant->SetRelativeRotation(bHealthy ? FRotator::ZeroRotator : FRotator(18.f * Side, 0.f, 12.f));
		Plant->SetRelativeScale3D(bHealthy ? FVector(0.28f, 0.28f, 0.32f) : FVector(0.30f, 0.30f, 0.24f));
	}
}
