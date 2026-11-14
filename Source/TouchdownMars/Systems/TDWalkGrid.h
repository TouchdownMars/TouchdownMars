// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Walkable ground grid for the crew (no nav mesh needed): built once from the level by tracing the landscape and
 * testing for modules, tanks and props above it. A* on the grid, then the path is shortened with straight-line
 * checks so the crew walk in long straight legs around the buildings.
 */
class TOUCHDOWNMARS_API FTDWalkGrid
{
public:
	/** Bounds in level cm (X/Y), cell size in cm. */
	void Build(UWorld* World, const FBox2D& Bounds, float CellSize);
	bool IsBuilt() const { return Cells.Num() > 0; }

	/** Path from Start to Goal (level cm, Z ignored); empty when no path. The first point is after Start. */
	TArray<FVector2D> FindPath(const FVector2D& Start, const FVector2D& Goal) const;

	int32 NumBlocked() const;

private:
	bool InBounds(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < Width && Y < Height; }
	bool Free(int32 X, int32 Y) const { return InBounds(X, Y) && Cells[Y * Width + X] == 0; }
	FIntPoint ToCell(const FVector2D& P) const;
	FVector2D ToWorld(const FIntPoint& C) const;
	FIntPoint NearestFree(const FIntPoint& C) const;
	bool LineFree(const FIntPoint& A, const FIntPoint& B) const;

	TArray<uint8> Cells; // 0 free, 1 blocked
	int32 Width = 0;
	int32 Height = 0;
	FVector2D Origin = FVector2D::ZeroVector;
	float Cell = 100.f;
};
