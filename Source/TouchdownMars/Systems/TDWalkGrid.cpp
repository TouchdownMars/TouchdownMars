// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Systems/TDWalkGrid.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "TouchdownMars.h"

void FTDWalkGrid::Build(UWorld* World, const FBox2D& Bounds, float CellSize)
{
	Cells.Reset();
	if (!World)
	{
		return;
	}
	Cell = FMath::Max(25.f, CellSize);
	Origin = Bounds.Min;
	Width = FMath::Max(1, FMath::CeilToInt((Bounds.Max.X - Bounds.Min.X) / Cell));
	Height = FMath::Max(1, FMath::CeilToInt((Bounds.Max.Y - Bounds.Min.Y) / Cell));
	Cells.Init(0, Width * Height);

	TArray<ALandscapeProxy*> Landscapes;
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		Landscapes.Add(*It);
	}
	auto LandscapeZ = [&Landscapes](const FVector& At, float& OutZ)
	{
		for (ALandscapeProxy* Landscape : Landscapes)
		{
			const TOptional<float> Z = Landscape->GetHeightAtLocation(At);
			if (Z.IsSet())
			{
				OutZ = Z.GetValue();
				return true;
			}
		}
		return false;
	};

	FCollisionQueryParams Params(TEXT("TDWalkGrid"), false);
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}
	const FCollisionObjectQueryParams Statics(ECC_WorldStatic);
	const FCollisionShape Probe = FCollisionShape::MakeSphere(Cell * 0.45f);
	int32 Blocked = 0;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FVector2D P = ToWorld(FIntPoint(X, Y));
			float Ground = 0.f;
			if (!LandscapeZ(FVector(P.X, P.Y, 0.f), Ground))
			{
				FHitResult Hit;
				if (World->LineTraceSingleByObjectType(Hit, FVector(P.X, P.Y, 20000.f), FVector(P.X, P.Y, -20000.f), Statics))
				{
					Ground = Hit.ImpactPoint.Z;
				}
			}
			// Blocked when anything other than the landscape stands at chest height (modules, tanks, props, rocks).
			// Flat pads and roads stay walkable: the probe sits above them.
			// A second probe at knee height catches crates, cases and junction boxes (pebbles stay below it).
			const FVector Chest(P.X, P.Y, Ground + 110.f);
			const FVector Knee(P.X, P.Y, Ground + 58.f);
			if (World->OverlapAnyTestByObjectType(Chest, FQuat::Identity, Statics, Probe, Params)
				|| World->OverlapAnyTestByObjectType(Knee, FQuat::Identity, Statics, FCollisionShape::MakeSphere(Cell * 0.4f), Params))
			{
				Cells[Y * Width + X] = 1;
				++Blocked;
			}
		}
	}
	UE_LOG(LogTouchdownMars, Display, TEXT("WalkGrid: %d x %d cells of %.0f cm, %d blocked"), Width, Height, Cell, Blocked);
}

int32 FTDWalkGrid::NumBlocked() const
{
	int32 N = 0;
	for (uint8 C : Cells) { N += C; }
	return N;
}

FIntPoint FTDWalkGrid::ToCell(const FVector2D& P) const
{
	return FIntPoint(FMath::Clamp(FMath::FloorToInt((P.X - Origin.X) / Cell), 0, Width - 1), FMath::Clamp(FMath::FloorToInt((P.Y - Origin.Y) / Cell), 0, Height - 1));
}

FVector2D FTDWalkGrid::ToWorld(const FIntPoint& C) const
{
	return FVector2D(Origin.X + (C.X + 0.5f) * Cell, Origin.Y + (C.Y + 0.5f) * Cell);
}

FIntPoint FTDWalkGrid::NearestFree(const FIntPoint& C) const
{
	if (Free(C.X, C.Y))
	{
		return C;
	}
	for (int32 R = 1; R < 40; ++R)
	{
		for (int32 DY = -R; DY <= R; ++DY)
		{
			for (int32 DX = -R; DX <= R; ++DX)
			{
				if ((FMath::Abs(DX) == R || FMath::Abs(DY) == R) && Free(C.X + DX, C.Y + DY))
				{
					return FIntPoint(C.X + DX, C.Y + DY);
				}
			}
		}
	}
	return C;
}

bool FTDWalkGrid::LineFree(const FIntPoint& A, const FIntPoint& B) const
{
	// Sample the segment at quarter-cell steps; any blocked cell breaks the straight leg.
	const FVector2D PA(A.X, A.Y);
	const FVector2D PB(B.X, B.Y);
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(PA, PB) * 4.f));
	for (int32 I = 0; I <= Steps; ++I)
	{
		const FVector2D Q = FMath::Lerp(PA, PB, static_cast<float>(I) / Steps);
		if (!Free(FMath::RoundToInt(Q.X), FMath::RoundToInt(Q.Y)))
		{
			return false;
		}
	}
	return true;
}

TArray<FVector2D> FTDWalkGrid::FindPath(const FVector2D& StartWorld, const FVector2D& GoalWorld) const
{
	TArray<FVector2D> Out;
	if (!IsBuilt())
	{
		return Out;
	}
	const FIntPoint Start = NearestFree(ToCell(StartWorld));
	const FIntPoint Goal = NearestFree(ToCell(GoalWorld));
	const int32 N = Width * Height;
	TArray<float> G;
	G.Init(TNumericLimits<float>::Max(), N);
	TArray<int32> Came;
	Came.Init(-1, N);
	TArray<uint8> Closed;
	Closed.Init(0, N);
	auto Index = [this](const FIntPoint& C) { return C.Y * Width + C.X; };
	auto H = [&Goal](const FIntPoint& C) { return static_cast<float>(FVector2D::Distance(FVector2D(C.X, C.Y), FVector2D(Goal.X, Goal.Y))); };

	// Binary heap keyed on f = g + h.
	struct FNode { int32 Idx; float F; };
	TArray<FNode> Open;
	auto Less = [](const FNode& A, const FNode& B) { return A.F < B.F; };
	G[Index(Start)] = 0.f;
	Open.HeapPush({ Index(Start), H(Start) }, Less);
	static const int32 DX[] = { 1, -1, 0, 0, 1, 1, -1, -1 };
	static const int32 DY[] = { 0, 0, 1, -1, 1, -1, 1, -1 };
	bool bFound = false;
	while (Open.Num() > 0)
	{
		FNode Node;
		Open.HeapPop(Node, Less);
		if (Closed[Node.Idx])
		{
			continue;
		}
		Closed[Node.Idx] = 1;
		const FIntPoint C(Node.Idx % Width, Node.Idx / Width);
		if (C == Goal)
		{
			bFound = true;
			break;
		}
		for (int32 D = 0; D < 8; ++D)
		{
			const FIntPoint Nb(C.X + DX[D], C.Y + DY[D]);
			if (!Free(Nb.X, Nb.Y) || (D >= 4 && (!Free(C.X + DX[D], C.Y) || !Free(C.X, C.Y + DY[D]))))
			{
				continue; // no corner cutting
			}
			const int32 NI = Index(Nb);
			const float Cost = G[Node.Idx] + (D >= 4 ? 1.4142f : 1.f);
			if (Cost < G[NI])
			{
				G[NI] = Cost;
				Came[NI] = Node.Idx;
				Open.HeapPush({ NI, Cost + H(Nb) }, Less);
			}
		}
	}
	if (!bFound)
	{
		return Out;
	}
	TArray<FIntPoint> Raw;
	for (int32 At = Index(Goal); At != -1; At = Came[At])
	{
		Raw.Insert(FIntPoint(At % Width, At / Width), 0);
	}
	// Shorten: keep only the corners needed for straight, free legs.
	int32 Anchor = 0;
	for (int32 I = 2; I < Raw.Num(); ++I)
	{
		if (!LineFree(Raw[Anchor], Raw[I]))
		{
			Out.Add(ToWorld(Raw[I - 1]));
			Anchor = I - 1;
		}
	}
	Out.Add(GoalWorld);
	return Out;
}
