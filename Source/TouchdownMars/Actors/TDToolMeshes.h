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
		default:
			return false;
		}
		Out.Mesh = LoadObject<UStaticMesh>(nullptr, Path);
		return Out.Mesh != nullptr;
	}

	/** Where the drill bit tip of the Pistol Grip Tool lands in tool space (for the spinning bit). */
	inline FVector DrillBitTip() { return FVector(30.4f, 0.f, 14.6f); }
}
