// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Actors/TDWorkSpot.h"

/**
 * Hand tools from NASA 3D Resources (github.com/nasa/NASA-3D-Resources: Hammer, Wrench, Pistol Grip Tool, the
 * real EVA power tool), imported by Scripts/import_nasa_props.py with their textures removed. Each entry places
 * the mesh in the same tool space the procedural tools use: the grip at the origin, Hammer handle up +Z with the
 * head on top, Wrench and drill pointing +X. Offsets were measured from the mesh vertices; sizes are the real
 * tool sizes or [DESIGNER VALUE] where NASA gives none. A missing mesh returns false and the caller keeps its
 * procedural shapes, so the game never breaks on a missing file.
 */
namespace TDTools
{
	struct FMeshPlacement
	{
		UStaticMesh* Mesh = nullptr;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		float Scale = 1.f;
	};

	inline bool NasaTool(ETDWorkVerb Verb, bool bCrew, FMeshPlacement& Out)
	{
		const TCHAR* Path = nullptr;
		switch (Verb)
		{
		case ETDWorkVerb::Hammer:
			// 36 cm hammer as modelled; mesh -Y is the head, so -Y turns to +Z.
			Path = TEXT("/Game/TouchdownMars/NASA/Hammer/Hammer_static/StaticMeshes/Hammer_static.Hammer_static");
			Out.Rotation = FRotationMatrix::MakeFromXY(FVector(1.f, 0.f, 0.f), FVector(0.f, 0.f, -1.f)).Rotator();
			Out.Location = bCrew ? FVector::ZeroVector : FVector(0.f, 0.f, 16.f);
			Out.Scale = 1.f;
			break;
		case ETDWorkVerb::Wrench:
			// Modelled 157 m long; a 28 cm spanner [DESIGNER VALUE]. Mesh +Y turns to +X.
			Path = TEXT("/Game/TouchdownMars/NASA/Wrench/Wrench_static/StaticMeshes/Wrench_static.Wrench_static");
			Out.Rotation = FRotator(0.f, -90.f, 0.f);
			Out.Scale = 28.f / 15717.f;
			Out.Location = bCrew ? FVector(1.3f, 0.f, 0.f) : FVector(4.3f, 0.f, 5.f);
			break;
		case ETDWorkVerb::Drill:
			// Modelled 15 m long; about 36 cm [DESIGNER VALUE]. The grip (mesh y -530, z -275) sits in the hand.
			Path = TEXT("/Game/TouchdownMars/NASA/PistolGripTool/PistolGripTool/StaticMeshes/PistolGripTool.PistolGripTool");
			Out.Rotation = FRotator(0.f, -90.f, 0.f);
			Out.Scale = 36.f / 1538.f;
			Out.Location = FVector(12.4f, 0.f, 6.4f);
			break;
		case ETDWorkVerb::Scoop:
		{
			// Tripo round 2 (AI-generated). Modelled on a slant: grip top (30, -39, 80), pan (-25, 31, 5) in mesh units.
			// The grip sits in the hand and the pan points forward and down (crew: straight down). 55 cm [DESIGNER VALUE].
			Path = TEXT("/Game/TouchdownMars/Tripo/ToolScoop/TD_Tripo_ToolScoop/StaticMeshes/TD_Tripo_ToolScoop.TD_Tripo_ToolScoop");
			const FVector Grip(30.f, -39.f, 80.f);
			const FVector Pan(-25.f, 31.f, 5.f);
			Out.Scale = 55.f / (Pan - Grip).Size();
			const FVector Target = bCrew ? FVector(0.f, 0.f, -1.f) : FVector(0.85f, 0.f, -0.53f).GetSafeNormal();
			const FQuat Turn = FQuat::FindBetweenNormals((Pan - Grip).GetSafeNormal(), Target);
			Out.Rotation = Turn.Rotator();
			Out.Location = -Turn.RotateVector(Grip * Out.Scale);
			break;
		}
		case ETDWorkVerb::Latch:
		case ETDWorkVerb::Scan:
		case ETDWorkVerb::Switch:
			// Tripo round 2 field tablet (AI-generated): face is mesh -Y; held upright toward the camera (crew: at the
			// side). 30 cm wide in first person, 14 cm on the crew scale [DESIGNER VALUE].
			Path = TEXT("/Game/TouchdownMars/Tripo/ToolTablet/TD_Tripo_ToolTablet/StaticMeshes/TD_Tripo_ToolTablet.TD_Tripo_ToolTablet");
			Out.Scale = bCrew ? 0.14f : 0.3f;
			Out.Rotation = bCrew ? FRotator::ZeroRotator : FRotator(0.f, -90.f, 0.f);
			Out.Location = FVector(bCrew ? 0.f : 6.f, 0.f, bCrew ? -40.f * Out.Scale : -4.f);
			break;
		default:
			return false;
		}
		Out.Mesh = LoadObject<UStaticMesh>(nullptr, Path);
		return Out.Mesh != nullptr;
	}

	/** Where the drill bit tip of the Pistol Grip Tool lands in tool space (for the spinning bit). */
	inline FVector DrillBitTip() { return FVector(30.4f, 0.f, 14.6f); }
}
