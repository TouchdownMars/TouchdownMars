// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDWorkSpot.h"
#include "Misc/ScopeExit.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Model/TDMissionRules.h"
#include "Systems/TDMissionSubsystem.h"
#include "UI/TDDesignTokens.h"
#include "UI/TDScreenWidget.h"
#include "Components/WidgetComponent.h"

namespace
{
	UStaticMesh* Shape(const TCHAR* Name)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
	}

	UMaterialInterface* ProjectMat(const TCHAR* Name)
	{
		return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Materials/%s.%s"), Name, Name));
	}

	UMaterialInterface* ShapeMat()
	{
		return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	const FLinearColor LampRed(0.9f, 0.08f, 0.05f);
	const FLinearColor LampAmber(1.f, 0.55f, 0.05f);
	const FLinearColor LampGreen(0.1f, 0.85f, 0.3f);
	const FLinearColor MarkerOrange(0.94f, 0.5f, 0.18f);
}

FTDHandsOnSpec TDHandsOn::SpecFor(FName TaskId)
{
	// [DESIGNER VALUE] Spots x beats x seconds: every task 3-6 s of hands-on work in total, in 3-5 beat chunks
	// (research note §3.6: holds near 2 s per object, a hit per beat, verbs vary between tasks).
	static const TMap<FName, FTDHandsOnSpec> Specs = {
		{ TEXT("InspectCargo"), { ETDWorkVerb::Scan, 3, 2, 0.6f } },
		{ TEXT("SuitCheck"), { ETDWorkVerb::Switch, 3, 2, 0.4f } },
		{ TEXT("OffloadRover"), { ETDWorkVerb::Wrench, 4, 3, 0.45f } },
		{ TEXT("OffloadPayload"), { ETDWorkVerb::Latch, 4, 2, 0.4f } },
		{ TEXT("SetUpGreenhouse"), { ETDWorkVerb::Plug, 3, 3, 0.5f } },
		{ TEXT("DeployMeda"), { ETDWorkVerb::Hammer, 3, 3, 0.55f } },
		{ TEXT("ScienceTraverse"), { ETDWorkVerb::Drill, 2, 4, 0.6f } },
		{ TEXT("SurveyClays"), { ETDWorkVerb::Hammer, 4, 3, 0.55f } },
		{ TEXT("ExtractWater"), { ETDWorkVerb::Scoop, 3, 3, 0.6f } },
		{ TEXT("BuildBerm"), { ETDWorkVerb::Scoop, 4, 3, 0.6f } },
		{ TEXT("CleanSolar"), { ETDWorkVerb::Brush, 4, 5, 0.3f } },
		{ TEXT("Maintenance"), { ETDWorkVerb::Switch, 3, 2, 0.4f } },
		{ TEXT("Logistics"), { ETDWorkVerb::Latch, 3, 2, 0.4f } },
		{ TEXT("Harvest"), { ETDWorkVerb::Scan, 3, 2, 0.5f } },
		{ TEXT("RoverExpedition"), { ETDWorkVerb::Plug, 1, 3, 0.5f } },
		{ TEXT("DeploySeis"), { ETDWorkVerb::Wrench, 3, 3, 0.45f } },
		{ TEXT("StudyQuake"), { ETDWorkVerb::Scan, 1, 3, 0.6f } },
	};
	if (const FTDHandsOnSpec* Found = Specs.Find(TaskId))
	{
		return *Found;
	}
	return FTDHandsOnSpec();
}

const TCHAR* TDHandsOn::VerbKey(ETDWorkVerb Verb)
{
	switch (Verb)
	{
	case ETDWorkVerb::Hammer: return TEXT("Work_Hammer");
	case ETDWorkVerb::Plug: return TEXT("Work_Plug");
	case ETDWorkVerb::Wrench: return TEXT("Work_Wrench");
	case ETDWorkVerb::Latch: return TEXT("Work_Latch");
	case ETDWorkVerb::Brush: return TEXT("Work_Brush");
	case ETDWorkVerb::Drill: return TEXT("Work_Drill");
	case ETDWorkVerb::Scan: return TEXT("Work_Scan");
	case ETDWorkVerb::Scoop: return TEXT("Work_Scoop");
	default: return TEXT("Work_Switch");
	}
}

// ATDWorkSpot

ATDWorkSpot::ATDWorkSpot()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);
}

UStaticMeshComponent* ATDWorkSpot::AddPart(UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FRotator& Rot, UMaterialInterface* Mat)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetMobility(EComponentMobility::Movable);
	Part->SetStaticMesh(Mesh);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCastShadow(true);
	Part->SetupAttachment(Root);
	Part->SetRelativeLocation(Loc);
	Part->SetRelativeRotation(Rot);
	Part->SetRelativeScale3D(Scale);
	if (Mat)
	{
		Part->SetMaterial(0, Mat);
	}
	Part->RegisterComponent();
	return Part;
}

void ATDWorkSpot::Setup(ETDWorkVerb InVerb, int32 InBeats, float InFaceYaw)
{
	Verb = InVerb;
	Beats = FMath::Max(1, InBeats);
	SetActorRotation(FRotator(0.f, InFaceYaw, 0.f));

	UStaticMesh* Cube = Shape(TEXT("Cube"));
	UStaticMesh* Cyl = Shape(TEXT("Cylinder"));
	UStaticMesh* Sphere = Shape(TEXT("Sphere"));
	UMaterialInterface* Dark = ProjectMat(TEXT("MI_TD_Base_Dark"));
	UMaterialInterface* Hull = ProjectMat(TEXT("MI_TD_Base_Hull"));
	UMaterialInterface* Rib = ProjectMat(TEXT("MI_TD_Base_Rib"));
	UMaterialInterface* Accent = ProjectMat(TEXT("MI_TD_Base_Accent"));
	UMaterialInterface* Regolith = ProjectMat(TEXT("MI_TD_Base_Regolith"));
	UMaterialInterface* Solar = ProjectMat(TEXT("MI_TD_Base_Solar"));
	UMaterialInterface* White = ProjectMat(TEXT("MI_TD_Plain_White"));
	FVector LampAt(0.f, 0.f, 60.f);
	// Tripo round 2 meshes (AI-generated, sources.csv) where they are imported; basic shapes otherwise.
	auto Tripo = [](const TCHAR* Name)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Tripo/%s/TD_Tripo_%s/StaticMeshes/TD_Tripo_%s.TD_Tripo_%s"), Name, Name, Name, Name));
	};
	UStaticMesh* Case = Tripo(TEXT("PayloadCase"));
	UStaticMesh* Mound = Tripo(TEXT("ShelterMound"));
	// Tripo round 5 kit meshes (AI-generated, sources.csv). Each arrives about 100 cm on its longest side with the
	// pivot at the bottom centre; Scale sets the kit size [DESIGNER VALUE]; Yaw turns the modelled front to +X,
	// where the player works. Fits were measured from the GLB vertices (research note placeholder_inventory.md).
	auto Kit = [this](UStaticMesh* Mesh, const FVector& Loc, float Scale, float Yaw)
	{
		UStaticMeshComponent* C = AddPart(Mesh, Loc, FVector(Scale), FRotator(0.f, Yaw, 0.f), nullptr);
		C->EmptyOverrideMaterials();
		return C;
	};

	switch (Verb)
	{
	case ETDWorkVerb::Hammer:
		if (UStaticMesh* StakeMesh = Tripo(TEXT("KitStake")))
		{
			// Survey stake with its own pennant, 90 cm long, foot 20 cm below ground at the start.
			Moving = Kit(StakeMesh, FVector(0.f, 0.f, -20.f), 0.9f, 0.f);
			StakeLampZ = 94.f;
			WorkPointOffset = FVector(0.f, 0.f, 45.f);
			LampAt = FVector(0.f, 0.f, 74.f);
			break;
		}
		// Stake 90 cm long, starts with 70 cm out of the ground; the flag unfurls when it is home.
		Moving = AddPart(Cyl, FVector(0.f, 0.f, 25.f), FVector(0.06f, 0.06f, 0.9f), FRotator::ZeroRotator, Rib);
		Extra = AddPart(Cube, FVector(0.f, 14.f, 60.f), FVector(0.01f, 0.28f, 0.18f), FRotator::ZeroRotator, Accent);
		ExtraScale = Extra->GetRelativeScale3D();
		Extra->SetRelativeScale3D(FVector(0.01f, 0.01f, 0.01f));
		LampAt = FVector(0.f, 0.f, 74.f);
		break;
	case ETDWorkVerb::Plug:
		if (UStaticMesh* BoxMesh = Tripo(TEXT("PowerJunctionBox")))
		{
			// Junction box on its stand, 120 cm tall; its +X face is 33 cm out, the plug seats there.
			Kit(BoxMesh, FVector::ZeroVector, 1.2f, 0.f);
			Moving = AddPart(Cyl, FVector(68.f, 0.f, 90.f), FVector(0.11f, 0.11f, 0.22f), FRotator(90.f, 0.f, 0.f), Accent);
			Extra = AddPart(Cyl, FVector(120.f, 0.f, 50.f), FVector(0.035f, 0.035f, 1.3f), FRotator(-45.f, 0.f, 0.f), Dark);
			LampAt = FVector(0.f, 0.f, 128.f);
			break;
		}
		AddPart(Cube, FVector(0.f, 0.f, 45.f), FVector(0.1f, 0.1f, 0.9f), FRotator::ZeroRotator, Dark);
		AddPart(Cube, FVector(0.f, 0.f, 95.f), FVector(0.25f, 0.3f, 0.3f), FRotator::ZeroRotator, Hull);
		Moving = AddPart(Cyl, FVector(48.f, 0.f, 95.f), FVector(0.11f, 0.11f, 0.22f), FRotator(90.f, 0.f, 0.f), Accent);
		// Cable from the plug down to the ground, back toward the site.
		Extra = AddPart(Cyl, FVector(100.f, 0.f, 50.f), FVector(0.035f, 0.035f, 1.3f), FRotator(-45.f, 0.f, 0.f), Dark);
		LampAt = FVector(0.f, 0.f, 116.f);
		break;
	case ETDWorkVerb::Wrench:
		if (UStaticMesh* PlateMesh = Tripo(TEXT("KitAnchorPlate")))
		{
			// 50 cm anchor plate with its bolt column; the turning nut sits on top of the column.
			Kit(PlateMesh, FVector::ZeroVector, 0.5f, 0.f);
			Moving = AddPart(Cyl, FVector(0.f, 0.f, 30.f), FVector(0.12f, 0.12f, 0.06f), FRotator::ZeroRotator, Dark);
			LampAt = FVector(-18.f, 18.f, 12.f);
			break;
		}
		AddPart(Cube, FVector(0.f, 0.f, 4.f), FVector(0.5f, 0.5f, 0.08f), FRotator::ZeroRotator, Rib);
		Moving = AddPart(Cyl, FVector(0.f, 0.f, 16.f), FVector(0.14f, 0.14f, 0.16f), FRotator::ZeroRotator, Dark);
		LampAt = FVector(-18.f, 18.f, 12.f);
		break;
	case ETDWorkVerb::Latch:
		if (Case)
		{
			// Hard-shell payload case, long side across (90 x 63 x 52 cm [DESIGNER VALUE]).
			UStaticMeshComponent* Body = AddPart(Case, FVector::ZeroVector, FVector(0.9f), FRotator(0.f, 90.f, 0.f), nullptr);
			Body->EmptyOverrideMaterials();
		}
		else
		{
			AddPart(Cube, FVector(0.f, 0.f, 28.f), FVector(0.6f, 0.9f, 0.56f), FRotator::ZeroRotator, Hull);
		}
		Extra = AddPart(Cube, FVector(0.f, 0.f, 59.f), FVector(0.62f, 0.92f, 0.06f), FRotator::ZeroRotator, Dark);
		ExtraScale = Extra->GetRelativeScale3D();
		Extra->SetVisibility(!Case);   // the payload case has its own lid
		Moving = AddPart(Cube, FVector(31.f, 0.f, 44.f), FVector(0.04f, 0.16f, 0.22f), FRotator::ZeroRotator, Accent);
		LampAt = FVector(31.f, 30.f, 50.f);
		break;
	case ETDWorkVerb::Brush:
		if (UStaticMesh* PanelMesh = Tripo(TEXT("KitSolarPanel")))
		{
			// Test panel on a post, 110 cm tall; the panel tilts 39 degrees toward +X (measured). The dust layer
			// lies 3 cm over the panel face and is swept off sideways.
			Kit(PanelMesh, FVector::ZeroVector, 1.1f, 90.f);
			BrushDust = FVector(0.84f, 0.8f, 0.012f);
			Moving = AddPart(Cube, FVector(3.6f, 0.f, 84.8f), BrushDust, FRotator(-39.f, 0.f, 0.f), Regolith);
			LampAt = FVector(-25.f, 45.f, 60.f);
			break;
		}
		AddPart(Cube, FVector(0.f, 0.f, 30.f), FVector(0.08f, 0.08f, 0.6f), FRotator::ZeroRotator, Rib);
		AddPart(Cube, FVector(0.f, 0.f, 62.f), FVector(0.8f, 1.2f, 0.04f), FRotator(-25.f, 0.f, 0.f), Solar);
		Moving = AddPart(Cube, FVector(0.f, 0.f, 64.5f), FVector(0.81f, 1.21f, 0.012f), FRotator(-25.f, 0.f, 0.f), Regolith);
		LampAt = FVector(-30.f, 62.f, 80.f);
		break;
	case ETDWorkVerb::Drill:
		if (UStaticMesh* RigMesh = Tripo(TEXT("KitDrillRig")))
		{
			// Tripod core-drill rig, 160 cm tall; the spinning bit runs down its centre.
			Kit(RigMesh, FVector::ZeroVector, 1.6f, 0.f);
		}
		else
		{
			AddPart(Cube, FVector(-22.f, 0.f, 75.f), FVector(0.08f, 0.08f, 1.5f), FRotator::ZeroRotator, Rib);
			AddPart(Cube, FVector(-10.f, 0.f, 140.f), FVector(0.35f, 0.12f, 0.12f), FRotator::ZeroRotator, Dark);
		}
		Moving = AddPart(Cyl, FVector(0.f, 0.f, 95.f), FVector(0.07f, 0.07f, 0.7f), FRotator::ZeroRotator, Dark);
		Extra = AddPart(Cyl, FVector(25.f, 0.f, 5.f), FVector(0.06f, 0.06f, 0.35f), FRotator(0.f, 0.f, 90.f), Regolith);
		ExtraScale = Extra->GetRelativeScale3D();
		Extra->SetVisibility(false);
		LampAt = Tripo(TEXT("KitDrillRig")) ? FVector(0.f, 24.f, 140.f) : FVector(-10.f, 0.f, 152.f);
		break;
	case ETDWorkVerb::Scan:
		if (Case)
		{
			UStaticMeshComponent* Body = AddPart(Case, FVector::ZeroVector, FVector(0.75f), FRotator(0.f, 90.f, 0.f), nullptr);
			Body->EmptyOverrideMaterials();
		}
		else
		{
			AddPart(Cube, FVector(0.f, 0.f, 23.f), FVector(0.5f, 0.7f, 0.46f), FRotator::ZeroRotator, Hull);
		}
		Moving = AddPart(Cube, FVector(26.f, 0.f, 28.f), FVector(0.01f, 0.32f, 0.16f), FRotator::ZeroRotator, White);
		ScanBar = AddPart(Cube, FVector(27.f, 0.f, 21.f), FVector(0.004f, 0.34f, 0.012f), FRotator::ZeroRotator, Accent);
		ScanBar->SetVisibility(false);
		LampAt = FVector(26.f, 24.f, 40.f);
		break;
	case ETDWorkVerb::Scoop:
		if (Mound)
		{
			// A small regolith pile (100 x 100 x 50 cm [DESIGNER VALUE]); it shrinks as it is shovelled away.
			Moving = AddPart(Mound, FVector(30.f, 0.f, 0.f), FVector(1.f, 1.f, 1.43f), FRotator::ZeroRotator, nullptr);
			Moving->EmptyOverrideMaterials();
			ScoopPileZ = 1.43f;
		}
		else
		{
			Moving = AddPart(Sphere, FVector(30.f, 0.f, 0.f), FVector(1.f, 1.f, 0.5f), FRotator::ZeroRotator, Regolith);
		}
		if (UStaticMesh* HopperMesh = Tripo(TEXT("KitHopper")))
		{
			// Hopper on legs, 80 cm tall; its bin runs from 40 to 74 cm, 32 x 38 cm inside (measured).
			Kit(HopperMesh, FVector(-55.f, 0.f, 0.f), 0.8f, 0.f);
			HopperFillZ = 40.f;
			HopperFillMaxH = 0.34f;
			Extra = AddPart(Cube, FVector(-55.f, 0.f, HopperFillZ + 2.f), FVector(0.3f, 0.36f, 0.04f), FRotator::ZeroRotator, Regolith);
			ExtraScale = Extra->GetRelativeScale3D();
			LampAt = FVector(-55.f, 34.f, 84.f);
			break;
		}
		AddPart(Cube, FVector(-55.f, 0.f, 30.f), FVector(0.5f, 0.6f, 0.6f), FRotator::ZeroRotator, Dark);
		Extra = AddPart(Cube, FVector(-55.f, 0.f, 4.f), FVector(0.44f, 0.54f, 0.04f), FRotator::ZeroRotator, Regolith);
		ExtraScale = Extra->GetRelativeScale3D();
		LampAt = FVector(-55.f, 32.f, 64.f);
		break;
	case ETDWorkVerb::Switch:
	default:
		if (UStaticMesh* PanelMesh = Tripo(TEXT("KitServicePanel")))
		{
			// Service panel on a post, 140 cm tall; the box face is 25 cm out at +X, the lever sits on it.
			Kit(PanelMesh, FVector::ZeroVector, 1.4f, 90.f);
			Moving = AddPart(Cube, FVector(29.f, 0.f, 104.f), FVector(0.05f, 0.05f, 0.14f), FRotator(-40.f, 0.f, 0.f), Accent);
			LampAt = FVector(29.f, 15.f, 124.f);
			break;
		}
		AddPart(Cube, FVector(-6.f, 0.f, 50.f), FVector(0.1f, 0.1f, 1.f), FRotator::ZeroRotator, Dark);
		AddPart(Cube, FVector(0.f, 0.f, 105.f), FVector(0.12f, 0.5f, 0.4f), FRotator::ZeroRotator, Hull);
		Moving = AddPart(Cube, FVector(8.f, 0.f, 105.f), FVector(0.05f, 0.05f, 0.14f), FRotator(-40.f, 0.f, 0.f), Accent);
		LampAt = FVector(7.f, 16.f, 118.f);
		break;
	}
	MovingStart = Moving ? Moving->GetRelativeLocation() : FVector::ZeroVector;
	// Dust falls at Mars gravity: 980 cm/s2 x MarsGravityRatio (constants.csv, NSSDC); ballistic, no drag.
	if (const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr)
	{
		const double Ratio = Mission->GetConstant(TEXT("MarsGravityRatio"));
		DustGravity = Ratio > 0.0 ? static_cast<float>(980.0 * Ratio) : 300.f;
	}

	// Status lamp: red until the spot is done (the object is the readout).
	LampMesh = AddPart(Sphere, LampAt, FVector(0.07f), FRotator::ZeroRotator, nullptr);
	if (UMaterialInterface* Base = ShapeMat())
	{
		LampMat = UMaterialInstanceDynamic::Create(Base, this);
		LampMesh->SetMaterial(0, LampMat);
	}
	Marker = AddPart(Cube, FVector(0.f, 0.f, 170.f), FVector(0.12f), FRotator(45.f, 0.f, 45.f), Accent);
	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Light->SetAttenuationRadius(320.f);
	Light->SetCastShadows(false);
	Light->SetLightColor(MarkerOrange);
	Light->SetMobility(EComponentMobility::Movable);
	Light->RegisterComponent();

	for (int32 Index = 0; Index < 6; ++Index)
	{
		UStaticMeshComponent* P = AddPart(Sphere, FVector::ZeroVector, FVector(0.05f), FRotator::ZeroRotator, Regolith);
		P->SetVisibility(false);
		P->SetCastShadow(false);
		Puffs.Add(P);
		PuffVelocity.Add(FVector::ZeroVector);
		PuffAge.Add(10.f);
	}
	SetActive(false);
	ApplyVisual();
}

FVector ATDWorkSpot::GetWorkPoint() const
{
	if (Moving)
	{
		return Moving->GetComponentTransform().TransformPosition(FVector::ZeroVector) + GetActorRotation().RotateVector(WorkPointOffset);
	}
	return GetActorLocation() + FVector(0.f, 0.f, 60.f);
}

void ATDWorkSpot::SetLamp(const FLinearColor& Color)
{
	if (LampMat)
	{
		LampMat->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void ATDWorkSpot::SetActive(bool bInActive)
{
	bActive = bInActive && !IsDone();
	if (Marker) { Marker->SetVisibility(bActive); }
	if (Light) { Light->SetVisibility(bActive || BeatFlash > 0.f); }
}

void ATDWorkSpot::ApplyVisual()
{
	const float P = GetProgress();
	if (!Moving)
	{
		return;
	}
	// A5: compute the new pose, keep it as the target and put the parts back where they were; Tick eases them over.
	const FTransform MovingWas = Moving->GetRelativeTransform();
	const FTransform ExtraWas = Extra ? Extra->GetRelativeTransform() : FTransform::Identity;
	ON_SCOPE_EXIT
	{
		MovingTarget = Moving->GetRelativeTransform();
		ExtraTarget = Extra ? Extra->GetRelativeTransform() : FTransform::Identity;
		const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
		if (bHasTargets && !(Mission && Mission->IsReduceMotion()))
		{
			Moving->SetRelativeTransform(MovingWas);
			if (Extra) { Extra->SetRelativeTransform(ExtraWas); }
		}
		bHasTargets = true;
	};
	switch (Verb)
	{
	case ETDWorkVerb::Hammer:
		Moving->SetRelativeLocation(MovingStart - FVector(0.f, 0.f, 55.f * P));
		if (LampMesh) { LampMesh->SetRelativeLocation(MovingStart + FVector(0.f, 0.f, StakeLampZ - 55.f * P)); }
		if (Extra)
		{
			Extra->SetRelativeLocation(MovingStart + FVector(0.f, 14.f, 35.f - 55.f * P));
			Extra->SetRelativeScale3D(IsDone() ? ExtraScale : FVector(0.01f));
		}
		break;
	case ETDWorkVerb::Plug:
	{
		// Seat, push, twist-lock.
		const float Push = FMath::Min(P * 1.5f, 1.f);
		Moving->SetRelativeLocation(MovingStart - FVector(PlugTravel * Push, 0.f, 0.f));
		Moving->SetRelativeRotation(FRotator(90.f, 0.f, P >= 1.f ? 90.f : 0.f));
		if (Extra) { Extra->SetRelativeLocation(FVector(MovingStart.X + 52.f - PlugTravel * Push, 0.f, 50.f)); }
		break;
	}
	case ETDWorkVerb::Wrench:
		Moving->SetRelativeLocation(MovingStart - FVector(0.f, 0.f, 6.f * P));
		Moving->SetRelativeRotation(FRotator(0.f, 120.f * BeatsDone, 0.f));
		break;
	case ETDWorkVerb::Latch:
		Moving->SetRelativeRotation(FRotator(0.f, 0.f, 0.f) + FRotator(-80.f * P, 0.f, 0.f));
		if (Extra && IsDone())
		{
			// Lid lifts and tips back once the last latch is open.
			Extra->SetRelativeLocation(FVector(-26.f, 0.f, 78.f));
			Extra->SetRelativeRotation(FRotator(55.f, 0.f, 0.f));
		}
		break;
	case ETDWorkVerb::Brush:
	{
		// Dust is swept off from one side: the dust layer shrinks and slides.
		const float Left = FMath::Max(0.001f, 1.f - P);
		Moving->SetRelativeScale3D(FVector(BrushDust.X, BrushDust.Y * Left, BrushDust.Z));
		Moving->SetRelativeLocation(MovingStart + FVector(0.f, -BrushDust.Y * 50.f * (1.f - Left), 0.f));
		// Dust eases down to a sliver (see Tick) instead of vanishing on the last beat.
		break;
	}
	case ETDWorkVerb::Drill:
		Moving->SetRelativeLocation(MovingStart - FVector(0.f, 0.f, 80.f * P));
		if (Extra) { Extra->SetVisibility(IsDone()); }
		break;
	case ETDWorkVerb::Scan:
		break;
	case ETDWorkVerb::Scoop:
		Moving->SetRelativeScale3D(FVector(FMath::Lerp(1.f, 0.35f, P), FMath::Lerp(1.f, 0.35f, P), FMath::Lerp(ScoopPileZ, ScoopPileZ * 0.24f, P)));
		if (Extra)
		{
			const float H = FMath::Lerp(0.04f, HopperFillMaxH, P);
			Extra->SetRelativeScale3D(FVector(ExtraScale.X, ExtraScale.Y, H));
			Extra->SetRelativeLocation(FVector(-55.f, 0.f, H * 50.f + HopperFillZ));
		}
		break;
	case ETDWorkVerb::Switch:
	default:
		Moving->SetRelativeRotation(FRotator(FMath::Lerp(-40.f, 40.f, P), 0.f, 0.f));
		break;
	}
	SetLamp(IsDone() ? LampGreen : BeatsDone > 0 ? LampAmber : LampRed);
}

bool ATDWorkSpot::Beat()
{
	if (IsDone())
	{
		return true;
	}
	++BeatsDone;
	BeatFlash = 0.25f;
	ScanSweep = 0.25f;
	ApplyVisual();
	const bool bDusty = Verb == ETDWorkVerb::Hammer || Verb == ETDWorkVerb::Drill || Verb == ETDWorkVerb::Scoop || Verb == ETDWorkVerb::Brush;
	if (bDusty)
	{
		Puff(GetWorkPoint() - FVector(0.f, 0.f, Verb == ETDWorkVerb::Hammer ? 40.f : 0.f), 6);
	}
	if (IsDone())
	{
		SetActive(false);
		if (Light)
		{
			Light->SetLightColor(LampGreen);
			Light->SetVisibility(true);
			BeatFlash = 0.8f;
		}
	}
	return IsDone();
}

void ATDWorkSpot::Puff(const FVector& At, int32 Count)
{
	for (int32 Index = 0; Index < FMath::Min(Count, Puffs.Num()); ++Index)
	{
		Puffs[Index]->SetWorldLocation(At + FVector(FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f), 0.f));
		Puffs[Index]->SetWorldScale3D(FVector(0.06f));
		Puffs[Index]->SetVisibility(true);
		PuffVelocity[Index] = FVector(FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(60.f, 160.f));
		PuffAge[Index] = 0.f;
	}
}

void ATDWorkSpot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	// A5: ease the moving parts toward their targets (about 0.15 s, [DESIGNER VALUE]).
	if (bHasTargets && Moving)
	{
		auto Ease = [DeltaSeconds](UStaticMeshComponent* C, const FTransform& Target)
		{
			const FTransform Now = C->GetRelativeTransform();
			const FVector L = FMath::VInterpTo(Now.GetLocation(), Target.GetLocation(), DeltaSeconds, 16.f);
			const FQuat Q = FQuat::Slerp(Now.GetRotation(), Target.GetRotation(), FMath::Clamp(DeltaSeconds * 16.f, 0.f, 1.f));
			const FVector S = FMath::VInterpTo(Now.GetScale3D(), Target.GetScale3D(), DeltaSeconds, 16.f);
			C->SetRelativeTransform(FTransform(Q, L, S));
		};
		Ease(Moving, MovingTarget);
		if (Extra) { Ease(Extra, ExtraTarget); }
		// The stake's lamp rides on top of the stake as it sinks.
		if (Verb == ETDWorkVerb::Hammer && LampMesh)
		{
			LampMesh->SetRelativeLocation(Moving->GetRelativeLocation() + FVector(0.f, 0.f, StakeLampZ));
		}
	}
	// Scan: a light bar sweeps up the label once per beat.
	if (ScanBar)
	{
		ScanSweep = FMath::Max(0.f, ScanSweep - DeltaSeconds);
		const float Sweep = ScanSweep > 0.f ? 1.f - ScanSweep / 0.25f : -1.f;
		ScanBar->SetVisibility(Sweep >= 0.f && Sweep <= 1.f);
		if (Sweep >= 0.f)
		{
			ScanBar->SetRelativeLocation(FVector(27.f, 0.f, FMath::Lerp(21.f, 35.f, Sweep)));
		}
	}
	if (Marker && bActive)
	{
		Marker->SetRelativeLocation(FVector(0.f, 0.f, 165.f + 8.f * FMath::Sin(Time * 3.f)));
		Marker->SetRelativeRotation(FRotator(45.f, Time * 90.f, 45.f));
	}
	if (Light)
	{
		if (BeatFlash > 0.f)
		{
			BeatFlash -= DeltaSeconds;
			Light->SetIntensity(IsDone() ? 6000.f * FMath::Max(0.f, BeatFlash) : 4500.f);
			if (BeatFlash <= 0.f)
			{
				Light->SetVisibility(bActive);
				Light->SetLightColor(MarkerOrange);
			}
		}
		else if (bActive)
		{
			Light->SetIntensity(1400.f + 900.f * (0.5f + 0.5f * FMath::Sin(Time * 4.f)));
		}
	}
	// Dust puff: cosmetic, the fall speed is a [DESIGNER VALUE].
	for (int32 Index = 0; Index < Puffs.Num(); ++Index)
	{
		if (PuffAge[Index] > 0.9f)
		{
			continue;
		}
		PuffAge[Index] += DeltaSeconds;
		PuffVelocity[Index].Z -= DustGravity * DeltaSeconds;
		Puffs[Index]->AddWorldOffset(PuffVelocity[Index] * DeltaSeconds);
		Puffs[Index]->SetWorldScale3D(FVector(FMath::Lerp(0.06f, 0.01f, FMath::Min(PuffAge[Index] / 0.9f, 1.f))));
		if (PuffAge[Index] > 0.9f)
		{
			Puffs[Index]->SetVisibility(false);
		}
	}
	if (Verb == ETDWorkVerb::Drill && Moving && BeatFlash > 0.f && !IsDone())
	{
		Moving->AddRelativeRotation(FRotator(0.f, 1440.f * DeltaSeconds, 0.f));
	}
}

// ATDMissionControlStation

ATDMissionControlStation::ATDMissionControlStation()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);
	Tags.Add(TEXT("Inspect_MissionControl"));
}

ATDMissionControlStation* ATDMissionControlStation::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ATDMissionControlStation> It(const_cast<UWorld*>(World)); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

ATDMissionControlStation* ATDMissionControlStation::Ensure(UWorld* World)
{
	if (ATDMissionControlStation* Existing = Find(World))
	{
		return Existing;
	}
	if (!World)
	{
		return nullptr;
	}
	// [DESIGNER VALUE] Beside the landing pad, 5 m from the player start (clear of the crew's idle spots), screen
	// turned toward the start.
	const FVector2D At(-1180.f, -560.f);
	const FVector2D Face(-950.f, -150.f);
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("TDStationGround"), false);
	float GroundZ = 0.f;
	if (World->LineTraceSingleByChannel(Hit, FVector(At.X, At.Y, 50000.f), FVector(At.X, At.Y, -50000.f), ECC_WorldStatic, Params))
	{
		GroundZ = Hit.ImpactPoint.Z;
	}
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Face.Y - At.Y, Face.X - At.X));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDMissionControlStation* Station = World->SpawnActor<ATDMissionControlStation>(FVector(At.X, At.Y, GroundZ), FRotator(0.f, Yaw, 0.f), Spawn);
	if (!Station)
	{
		return nullptr;
	}
	UStaticMesh* Cube = Shape(TEXT("Cube"));
	UStaticMesh* Cyl = Shape(TEXT("Cylinder"));
	UStaticMesh* Sphere = Shape(TEXT("Sphere"));
	UMaterialInterface* Dark = ProjectMat(TEXT("MI_TD_Base_Dark"));
	UMaterialInterface* Hull = ProjectMat(TEXT("MI_TD_Base_Hull"));
	UMaterialInterface* Rib = ProjectMat(TEXT("MI_TD_Base_Rib"));
	UMaterialInterface* Accent = ProjectMat(TEXT("MI_TD_Base_Accent"));
	auto Part = [Station](UStaticMesh* Mesh, const FVector& Loc, const FVector& Scale, const FRotator& Rot, UMaterialInterface* Mat, bool bBlock)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Station);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetupAttachment(Station->Root);
		C->SetRelativeLocation(Loc);
		C->SetRelativeRotation(Rot);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(bBlock ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		C->SetCollisionResponseToAllChannels(ECR_Block);
		if (Mat) { C->SetMaterial(0, Mat); }
		C->RegisterComponent();
		return C;
	};
	TArray<UStaticMeshComponent*> Body;
	Body.Add(Part(Cyl, FVector(0.f, 0.f, 2.f), FVector(2.6f, 2.6f, 0.04f), FRotator::ZeroRotator, Dark, false));
	Body.Add(Part(Cube, FVector(0.f, 0.f, 48.f), FVector(0.7f, 1.7f, 0.96f), FRotator::ZeroRotator, Hull, true));
	Body.Add(Part(Cube, FVector(22.f, 0.f, 97.f), FVector(0.5f, 1.72f, 0.04f), FRotator(-12.f, 0.f, 0.f), Dark, false));
	for (const float Side : { -95.f, 95.f })
	{
		Body.Add(Part(Cube, FVector(-20.f, Side, 115.f), FVector(0.14f, 0.14f, 2.3f), FRotator::ZeroRotator, Rib, true));
	}
	Body.Add(Part(Cube, FVector(-20.f, 0.f, 228.f), FVector(0.22f, 2.06f, 0.18f), FRotator::ZeroRotator, Accent, false));
	// Tripo console body (round 2, AI-generated, sources.csv) where it is imported: 206 cm wide [DESIGNER VALUE],
	// its front (mesh +Y) toward the player. The procedural parts stay as invisible colliders, and the live screen,
	// bars and cell lamps stay in front of the console's own bezel. Without the mesh, the procedural body shows.
	float BeaconZ = 252.f;
	bool bShell = false;
	if (UStaticMesh* ConsoleMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/TouchdownMars/Tripo/OpsConsole/TD_Tripo_OpsConsole/StaticMeshes/TD_Tripo_OpsConsole.TD_Tripo_OpsConsole")))
	{
		for (UStaticMeshComponent* C : Body)
		{
			C->SetVisibility(false);
		}
		UStaticMeshComponent* Shell = Part(ConsoleMesh, FVector(-20.f, 0.f, 0.f), FVector(2.06f), FRotator(0.f, -90.f, 0.f), nullptr, false);
		BeaconZ = Shell->Bounds.Origin.Z + Shell->Bounds.BoxExtent.Z - Station->GetActorLocation().Z + 12.f;
		bShell = true;
	}
	Station->Screen = Part(Cube, FVector(-14.f, 0.f, 150.f), FVector(0.05f, 1.6f, 0.85f), FRotator(-15.f, 0.f, 0.f), nullptr, false);
	// Screen readout, in the screen's own tilted frame.
	USceneComponent* ScreenFace = NewObject<USceneComponent>(Station);
	ScreenFace->SetMobility(EComponentMobility::Movable);
	ScreenFace->SetupAttachment(Station->Root);
	ScreenFace->SetRelativeLocation(FVector(-14.f, 0.f, 150.f));
	ScreenFace->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	ScreenFace->RegisterComponent();
	UMaterialInterface* BaseMat = ShapeMat();
	auto ScreenFacePart = [Station, ScreenFace, Cube, BaseMat](const FVector& Loc, const FVector& Scale, TArray<TObjectPtr<UMaterialInstanceDynamic>>& Mats)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Station);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Cube);
		C->SetupAttachment(ScreenFace);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		if (BaseMat)
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(BaseMat, Station);
			C->SetMaterial(0, M);
			Mats.Add(M);
		}
		C->RegisterComponent();
		return C;
	};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Station->Bars.Add(ScreenFacePart(FVector(3.f, -14.f - Index * 24.f, -10.f), FVector(0.02f, 0.16f, 0.5f), Station->BarMats));
	}
	for (int32 Index = 0; Index < 12; ++Index)
	{
		Station->CellLamps.Add(ScreenFacePart(FVector(3.f, 65.f - (Index % 6) * 11.f, 18.f - (Index / 6) * 14.f), FVector(0.02f, 0.08f, 0.08f), Station->CellMats));
	}
	UStaticMeshComponent* Beacon = Part(Sphere, FVector(-20.f, 0.f, BeaconZ), FVector(0.26f), FRotator::ZeroRotator, nullptr, false);
	if (UMaterialInterface* Base = ShapeMat())
	{
		Station->ScreenMat = UMaterialInstanceDynamic::Create(Base, Station);
		Station->ScreenMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.13f, 0.17f));
		Station->Screen->SetMaterial(0, Station->ScreenMat);
		Station->BeaconMat = UMaterialInstanceDynamic::Create(Base, Station);
		Beacon->SetMaterial(0, Station->BeaconMat);
	}
	if (bShell)
	{
		// The Tripo console has its own screens and lamps: the procedural screen slab, bars, cell lamps and the
		// beacon ball would float in front of it (the HUD already shows the power cells). Only their glow stays.
		for (UStaticMeshComponent* C : Station->Bars) { C->DestroyComponent(); }
		for (UStaticMeshComponent* C : Station->CellLamps) { C->DestroyComponent(); }
		Station->Bars.Empty();
		Station->CellLamps.Empty();
		Station->BarMats.Empty();
		Station->CellMats.Empty();
		Station->Screen->SetVisibility(false);
		Beacon->SetVisibility(false);
	}
	// Lit console face: 150 x 60 cm [DESIGNER VALUE]. On the Tripo console it sits on the screen bank, which leans
	// back about 16 degrees (measured from the GLB: screen face 16 cm out at 134 cm high, 2 cm in at 196 cm);
	// on the procedural body it covers the screen slab.
	Station->ScreenWidget = NewObject<UWidgetComponent>(Station);
	Station->ScreenWidget->SetupAttachment(Station->Root);
	Station->ScreenWidget->SetWidgetSpace(EWidgetSpace::World);
	Station->ScreenWidget->SetWidgetClass(UTDScreenWidget::StaticClass());
	Station->ScreenWidget->SetDrawSize(FVector2D(1200.f, 480.f));
	Station->ScreenWidget->SetBlendMode(EWidgetBlendMode::Opaque);
	Station->ScreenWidget->SetTwoSided(false);
	Station->ScreenWidget->SetTintColorAndOpacity(FLinearColor(2.2f, 2.2f, 2.2f, 1.f));   // reads as a lit display in daylight
	Station->ScreenWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Station->ScreenWidget->SetCastShadow(false);
	if (bShell)
	{
		Station->ScreenWidget->SetRelativeLocation(FVector(9.f, 0.f, 162.f));
		Station->ScreenWidget->SetRelativeRotation(FRotator(16.f, 0.f, 0.f));
		Station->ScreenWidget->SetRelativeScale3D(FVector(0.125f));
	}
	else
	{
		Station->ScreenWidget->SetRelativeLocation(FVector(-11.f, 0.f, 150.f));
		Station->ScreenWidget->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
		Station->ScreenWidget->SetRelativeScale3D(FVector(0.13f));
		Station->Screen->SetVisibility(false);
		for (UStaticMeshComponent* C : Station->Bars) { C->SetVisibility(false); }
		for (UStaticMeshComponent* C : Station->CellLamps) { C->SetVisibility(false); }
	}
	Station->ScreenWidget->RegisterComponent();
	Station->ScreenWidget->InitWidget();
	if (UTDScreenWidget* ConsoleFace = Cast<UTDScreenWidget>(Station->ScreenWidget->GetUserWidgetObject()))
	{
		ConsoleFace->Mode = ETDScreenMode::Console;
	}
	Station->ScreenLight = NewObject<UPointLightComponent>(Station);
	Station->ScreenLight->SetupAttachment(Station->Root);
	Station->ScreenLight->SetRelativeLocation(FVector(70.f, 0.f, 150.f));
	Station->ScreenLight->SetLightColor(FLinearColor(0.35f, 0.8f, 1.f));
	Station->ScreenLight->SetIntensity(2200.f);
	Station->ScreenLight->SetAttenuationRadius(260.f);
	Station->ScreenLight->SetCastShadows(false);
	Station->ScreenLight->SetMobility(EComponentMobility::Movable);
	Station->ScreenLight->RegisterComponent();
	Station->BeaconLight = NewObject<UPointLightComponent>(Station);
	Station->BeaconLight->SetupAttachment(Station->Root);
	Station->BeaconLight->SetRelativeLocation(FVector(-20.f, 0.f, BeaconZ + 10.f));
	Station->BeaconLight->SetAttenuationRadius(700.f);
	Station->BeaconLight->SetCastShadows(false);
	Station->BeaconLight->SetMobility(EComponentMobility::Movable);
	Station->BeaconLight->RegisterComponent();
	return Station;
}

void ATDMissionControlStation::SetNeedsYou(bool bNeeds)
{
	bNeedsYou = bNeeds;
	if (UTDScreenWidget* Face = ScreenWidget ? Cast<UTDScreenWidget>(ScreenWidget->GetUserWidgetObject()) : nullptr)
	{
		Face->SetNeedsYou(bNeeds);
	}
}

FVector ATDMissionControlStation::GetUsePoint() const
{
	return GetActorLocation() + GetActorForwardVector() * 110.f;
}

void ATDMissionControlStation::UpdateReadout()
{
	const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	if (!Mission || !Mission->HasMission())
	{
		return;
	}
	const FTDMissionState& S = Mission->GetState();
	for (int32 Index = 0; Index < Bars.Num(); ++Index)
	{
		const ETDSystem System = static_cast<ETDSystem>(Index);
		const int32 Level = S.Levels.IsValidIndex(Index) ? S.Levels[Index] : 0;
		const float H = FMath::Max(0.02f, 0.5f * Level / 4.f);
		Bars[Index]->SetRelativeScale3D(FVector(0.02f, 0.16f, H));
		Bars[Index]->SetRelativeLocation(FVector(3.f, -14.f - Index * 24.f, -35.f + H * 50.f));
		if (BarMats.IsValidIndex(Index))
		{
			BarMats[Index]->SetVectorParameterValue(TEXT("Color"), TDTokens::System(System));
		}
	}
	const int32 Placed = UTDMissionRules::GetCellsPlaced(S);
	const float Blink = 0.5f + 0.5f * FMath::Sin(Time * 6.f);
	for (int32 Index = 0; Index < CellLamps.Num(); ++Index)
	{
		CellLamps[Index]->SetVisibility(Index < S.CellsTotal);
		if (CellMats.IsValidIndex(Index))
		{
			const FLinearColor C = Index < Placed ? FLinearColor(0.02f, 0.03f, 0.04f) : FLinearColor(1.f, 0.45f, 0.08f) * (0.5f + 0.5f * Blink);
			CellMats[Index]->SetVectorParameterValue(TEXT("Color"), C);
		}
	}
}

void ATDMissionControlStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	ReadoutTimer -= DeltaSeconds;
	if (ReadoutTimer <= 0.f)
	{
		ReadoutTimer = 0.1f;
		UpdateReadout();
	}
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * (bNeedsYou ? 5.f : 1.5f));
	const FLinearColor BeaconColor = bNeedsYou ? FLinearColor(1.f, 0.45f, 0.08f) : FLinearColor(0.12f, 0.8f, 0.35f);
	if (BeaconMat)
	{
		BeaconMat->SetVectorParameterValue(TEXT("Color"), BeaconColor * (0.4f + 0.6f * Pulse));
	}
	if (BeaconLight)
	{
		BeaconLight->SetLightColor(BeaconColor);
		BeaconLight->SetIntensity((bNeedsYou ? 9000.f : 2500.f) * (0.35f + 0.65f * Pulse));
	}
	if (ScreenMat)
	{
		// Screen flicker: a slow scanline breathing.
		ScreenMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.13f, 0.17f) * (0.85f + 0.15f * FMath::Sin(Time * 7.f)));
	}
}
