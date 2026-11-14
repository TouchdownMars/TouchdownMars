// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDSceneActors.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Actors/TDWorkSpot.h"
#include "Actors/TDToolMeshes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

#include "Actors/TDSystemModule.h"
#include "Animation/AnimSequence.h"
#include "CineCameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
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
	for (UStaticMeshComponent* Line : Lines)
	{
		Line->SetVisibility(bShowEnergyLines);
		Line->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
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
	// 180 cm crew in suits (scale: 1 unit = 1 cm).
	Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.8f));
	// Crew never carve the nav mesh and never block ground traces of other crew.
	Body->SetCanEverAffectNavigation(false);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Suit = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Suit"));
	Suit->SetupAttachment(Body);
	Suit->SetUsingAbsoluteScale(true);
	Suit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATDCrewMember::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Material = Body->CreateAndSetMaterialInstanceDynamic(0);
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), TDTokens::System(System));
	}
	Suit->SetSkeletalMesh(SuitMesh);
	Suit->SetVisibility(SuitMesh != nullptr);
	Body->SetVisibility(SuitMesh == nullptr, false);
	if (SuitMesh)
	{
		const FBoxSphereBounds Bounds = SuitMesh->GetBounds();
		const float MeshHeight = FMath::Max(1.f, Bounds.BoxExtent.Z * 2.f);
		Suit->SetWorldScale3D(FVector(SuitHeight / MeshHeight));
		Suit->SetRelativeLocation(FVector::ZeroVector); // both pivots are at the feet
		SuitMaterials.Reset();
		for (int32 Index = 0; Index < Suit->GetNumMaterials(); ++Index)
		{
			if (SuitMaterial)
			{
				Suit->SetMaterial(Index, SuitMaterial);
			}
			SuitMaterials.Add(Suit->CreateAndSetMaterialInstanceDynamic(Index));
		}
		ApplySuitTint(TDTokens::System(System));
	}
}

void ATDCrewMember::ApplySuitTint(const FLinearColor& Color)
{
	// A system-color tint on the white suit keeps the color coding (and hides any printed patch).
	const FLinearColor Tint = FMath::Lerp(FLinearColor::White, Color, 0.55f);
	for (UMaterialInstanceDynamic* SuitMID : SuitMaterials)
	{
		if (SuitMID)
		{
			SuitMID->SetVectorParameterValue(TEXT("Color"), Tint);           // MI_TD_Suit_* (BasicShapeMaterial)
			SuitMID->SetVectorParameterValue(TEXT("DiffuseColor"), Tint);    // imported FBX material
			SuitMID->SetVectorParameterValue(TEXT("BaseColorFactor"), Tint); // imported glTF material
		}
	}
}

void ATDCrewMember::PlayClip(UAnimSequence* Clip, float Rate)
{
	if (!SuitMesh || !Clip)
	{
		return;
	}
	// A1: PlayAnimation restarts at frame 0 on every call (engine: AnimSingleNodeInstanceProxy sets CurrentTime = 0),
	// so the same clip only gets its rate updated; idles no longer snap back when a system level changes.
	if (const UAnimSingleNodeInstance* Single = Suit->GetSingleNodeInstance(); Single && Single->GetAnimationAsset() == Clip)
	{
		Suit->SetPlayRate(Clip == WalkAnim ? Rate : Rate * (1.f + 0.06f * FMath::Sin(IdleSeed * 1.7f)));
		return;
	}
	Suit->PlayAnimation(Clip, true);
	if (Clip != WalkAnim)
	{
		// Each crew member breathes and shifts weight at their own moment: own start phase, own rate (+-6 %,
		// [DESIGNER VALUE]). Not on walk: a rate offset there makes the feet slide.
		Suit->SetPosition(FMath::Fmod(IdleSeed / (2.f * PI), 1.f) * Clip->GetPlayLength(), false);
		Rate *= 1.f + 0.06f * FMath::Sin(IdleSeed * 1.7f);
	}
	Suit->SetPlayRate(Rate);
}

float ATDCrewMember::WalkRate(float SpeedCmPerSec) const
{
	// A2: feet stay planted at any path speed; 0.35-5 [DESIGNER VALUE] covers a slow arrival and the time-lapse.
	return FMath::Clamp(SpeedCmPerSec / FMath::Max(10.f, WalkClipSpeedCmPerSec), 0.35f, 5.f);
}

void ATDCrewMember::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	FeetOffset = GetActorLocation().Z - GroundZ(GetActorLocation());
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
				PlayClip(It->ActorHasTag(TEXT("Anim_Walk")) ? WalkAnim.Get() : (It->ActorHasTag(TEXT("Anim_Work")) ? WorkAnim.Get() : IdleAnim.Get()));
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
		PlayBehaviour(Current);
	}
}

void ATDCrewMember::HandlePhaseStarted(ETDPhase Phase)
{
	if (bOnTask)
	{
		return;
	}
	SetBasePose(HomeLocation, BaseRotation);
}

void ATDCrewMember::SetBasePose(const FVector& Location, const FRotator& Rotation)
{
	UKismetMathLibrary::ResetFloatSpringState(FacingSpring);
	BaseLocation = Location;
	BaseRotation = Rotation;
	SetActorLocationAndRotation(Location, Rotation);
}

void ATDCrewMember::StepIdle()
{
	IdleTime += 1.f / 30.f;
	StepWorkRange();
	if (IsWalking())
	{
		StepWalk(1.f / 30.f);
		return;
	}
	if (IsEscorting() && !bOnTask)
	{
		StepEscort(1.f / 30.f);
		return;
	}
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	// A8: an idle crew member at home turns the whole suit toward the player when they come close (suits limit
	// head turns, so suited people turn the torso). [DESIGNER VALUE] 600 cm, 20 degrees.
	if (SuitMesh && !bOnTask && !bInsideModule && !bStaged && !(Mission && Mission->IsReduceMotion()))
	{
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && FVector::Dist2D(Player->GetActorLocation(), BaseLocation) < 600.f)
		{
			FaceToward(Player->GetActorLocation(), 1.f / 30.f, 20.f);
		}
	}
	if ((Mission && Mission->IsReduceMotion()) || SuitMesh)
	{
		SetActorLocationAndRotation(BaseLocation, BaseRotation);
		if (SuitMesh && Mission)
		{
			Suit->bPauseAnims = Mission->IsReduceMotion();
		}
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
	ApplySuitTint(Color);
	if (bOnTask)
	{
		OnBehaviour(Behaviour);
		return; // busy with a task: keep walking/working where they are
	}
	switch (Behaviour)
	{
	case ETDCrewBehaviour::Worried: PlayClip(WorriedAnim); break;
	case ETDCrewBehaviour::Alarm: PlayClip(WorriedAnim, 1.6f); break;
	case ETDCrewBehaviour::Celebrate: PlayClip(CelebrateAnim); break;
	case ETDCrewBehaviour::AtLander: PlayClip(FarewellAnim); break;
	default: PlayClip(IdleAnim); break;
	}
	if (!bStaged)
	{
		SetBasePose(Location, BaseRotation);
	}
	OnBehaviour(Behaviour);
}

// Crew walking and tasks (surface operations)

float ATDCrewMember::GroundZ(const FVector& At) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("TDCrewGround"), false, this);
	const FVector Start(At.X, At.Y, At.Z + 3000.f);
	const FVector End(At.X, At.Y, At.Z - 6000.f);
	if (GetWorld() && GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return Hit.ImpactPoint.Z;
	}
	return At.Z - FeetOffset;
}

void ATDCrewMember::WalkTo(const FVector& Target, float SpeedCmPerSec, TFunction<void()> OnArrived)
{
	SetInside(false);
	PathPoints.Reset();
	PathIndex = 0;
	WalkSpeed = FMath::Max(10.f, SpeedCmPerSec);
	ArriveCallback = MoveTemp(OnArrived);
	const FVector Start = BaseLocation;
	if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		const FVector From(Start.X, Start.Y, GroundZ(Start));
		const FVector To(Target.X, Target.Y, GroundZ(Target));
		if (UNavigationPath* Path = Nav->FindPathToLocationSynchronously(GetWorld(), From, To, this))
		{
			if (Path->IsValid() && Path->PathPoints.Num() > 1)
			{
				PathPoints = Path->PathPoints;
			}
		}
	}
	if (PathPoints.Num() == 0)
	{
		PathPoints = { Start, Target }; // no nav mesh: walk straight
	}
	PathIndex = 1;
	PlayClip(WalkAnim, WalkRate(WalkSpeed));
}

void ATDCrewMember::WalkPath(const TArray<FVector>& Points, float SpeedCmPerSec, TFunction<void()> OnArrived)
{
	SetInside(false);
	WalkSpeed = FMath::Max(10.f, SpeedCmPerSec);
	ArriveCallback = MoveTemp(OnArrived);
	PathPoints.Reset();
	PathPoints.Add(BaseLocation);
	PathPoints.Append(Points);
	PathIndex = 1;
	PlayClip(WalkAnim, WalkRate(WalkSpeed));
}

void ATDCrewMember::StopWalking()
{
	PathPoints.Reset();
	PathIndex = 0;
	ArriveCallback = nullptr;
	PlayClip(IdleAnim);
}

void ATDCrewMember::StepWalk(float DeltaSeconds)
{
	if (!PathPoints.IsValidIndex(PathIndex))
	{
		TFunction<void()> Done = MoveTemp(ArriveCallback);
		PathPoints.Reset();
		PathIndex = 0;
		PlayClip(IdleAnim);
		if (Done)
		{
			Done();
		}
		return;
	}
	const FVector Goal = PathPoints[PathIndex];
	FVector Flat = FVector(Goal.X - BaseLocation.X, Goal.Y - BaseLocation.Y, 0.f);
	const float Distance = Flat.Size();
	// A8: slow down over the last 40 cm of the path instead of popping from mid-stride into the idle; the walk
	// play rate follows the eased speed so the feet stay planted. [DESIGNER VALUE]
	float Speed = WalkSpeed;
	if (PathIndex == PathPoints.Num() - 1 && Distance < 40.f)
	{
		Speed = WalkSpeed * FMath::Clamp(Distance / 40.f, 0.4f, 1.f);
		if (SuitMesh) { Suit->SetPlayRate(WalkRate(Speed)); }
	}
	const float Step = Speed * DeltaSeconds;
	FVector Next = BaseLocation;
	if (Distance <= Step)
	{
		Next.X = Goal.X;
		Next.Y = Goal.Y;
		++PathIndex;
	}
	else
	{
		Flat /= Distance;
		Next += Flat * Step;
		// Turn smoothly toward the walking direction (the facing spring starts fresh after the walk).
		UKismetMathLibrary::ResetFloatSpringState(FacingSpring);
		const float TargetYaw = Flat.Rotation().Yaw - SuitForwardYaw;
		BaseRotation.Yaw = FMath::FixedTurn(BaseRotation.Yaw, TargetYaw, 360.f * DeltaSeconds);
	}
	Next.Z = GroundZ(Next) + FeetOffset;
	BaseLocation = Next;
	SetActorLocationAndRotation(BaseLocation, BaseRotation);
}

void ATDCrewMember::WorkAt(const FVector& LookAt, FName TaskId)
{
	FVector Dir = LookAt - BaseLocation;
	Dir.Z = 0.f;
	if (!Dir.IsNearlyZero())
	{
		BaseRotation.Yaw = Dir.Rotation().Yaw - SuitForwardYaw;
		SetActorLocationAndRotation(BaseLocation, BaseRotation);
	}
	PlayClip(WorkAnim ? WorkAnim.Get() : IdleAnim.Get());
	if (!WorkAnim || !SuitMesh || TaskId.IsNone())
	{
		return;
	}
	// A6: ground work (stakes, regolith, bolts) stays crouched with hands near the ground; waist work (cables,
	// latches, panels, scanners, drills) stands. Ranges measured from the lift_heavy clip, pulled in from the ends.
	// Suited crew really work kneeling and crouching (NASA Artemis conops, EVA-EXP-0075).
	const ETDWorkVerb Verb = TDHandsOn::SpecFor(TaskId).Verb;
	const bool bGround = Verb == ETDWorkVerb::Hammer || Verb == ETDWorkVerb::Scoop || Verb == ETDWorkVerb::Wrench;
	WorkRangeStart = bGround ? 0.2f : 6.0f;
	WorkRangeEnd = bGround ? 3.7f : 13.4f;
	bWorkRange = true;
	Suit->SetPosition(WorkRangeStart + FMath::Fmod(IdleSeed, 1.f) * (WorkRangeEnd - WorkRangeStart) * 0.5f, false);
	ShowHandTool(static_cast<int32>(Verb));
}

void ATDCrewMember::StepWorkRange()
{
	if (!bWorkRange || !SuitMesh)
	{
		return;
	}
	UAnimSingleNodeInstance* Single = Suit->GetSingleNodeInstance();
	if (!Single || Single->GetAnimationAsset() != WorkAnim)
	{
		bWorkRange = false;
		HideHandTool();
		return;
	}
	// Ping-pong instead of a hard wrap: no pop at the range ends.
	// A frame hitch can carry the position past an end (and a looping clip wraps): clamp back into the range.
	const float Now = Single->GetCurrentTime();
	if (Now >= WorkRangeEnd || (Single->IsReverse() && Now > WorkRangeEnd - 0.01f))
	{
		Single->SetPosition(WorkRangeEnd, false);
		Single->SetReverse(true);
	}
	else if (Now <= WorkRangeStart)
	{
		Single->SetPosition(WorkRangeStart, false);
		Single->SetReverse(false);
	}
}

void ATDCrewMember::ShowHandTool(int32 VerbIndex)
{
	auto Mesh = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name)); };
	auto Mat = [](const TCHAR* Name) { return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Materials/%s.%s"), Name, Name)); };
	for (TObjectPtr<UStaticMeshComponent>* Part : { &HandToolA, &HandToolB })
	{
		if (!*Part)
		{
			*Part = NewObject<UStaticMeshComponent>(this);
			(*Part)->SetMobility(EComponentMobility::Movable);
			(*Part)->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			(*Part)->SetCastShadow(false);
			(*Part)->SetupAttachment(Suit, TEXT("RightHand"));
			(*Part)->RegisterComponent();
		}
	}
	// The suit is scaled up to SuitHeight; the tool undoes that scale. Sizes and offsets [DESIGNER VALUE].
	const float Undo = 1.f / FMath::Max(0.01f, Suit->GetComponentScale().X);
	auto Set = [Undo](UStaticMeshComponent* C, UStaticMesh* M, const FVector& Loc, const FVector& Scale, const FRotator& Rot, UMaterialInterface* Mt)
	{
		C->SetStaticMesh(M);
		C->SetRelativeLocation(Loc * Undo);
		C->SetRelativeRotation(Rot);
		C->SetRelativeScale3D(Scale * Undo);
		C->EmptyOverrideMaterials();
		C->SetMaterial(0, Mt);
		C->SetVisibility(true);
	};
	// NASA 3D Resources tools where we have them (TDToolMeshes.h); the procedural shapes stay as the fallback.
	TDTools::FMeshPlacement Nasa;
	if (TDTools::NasaTool(static_cast<ETDWorkVerb>(VerbIndex), true, Nasa))
	{
		HandToolA->SetStaticMesh(Nasa.Mesh);
		HandToolA->EmptyOverrideMaterials();
		HandToolA->SetRelativeLocation(Nasa.Location * Undo);
		HandToolA->SetRelativeRotation(Nasa.Rotation);
		HandToolA->SetRelativeScale3D(FVector(Nasa.Scale * Undo));
		HandToolA->SetVisibility(true);
		HandToolB->SetVisibility(false);
		return;
	}
	UStaticMesh* Cube = Mesh(TEXT("Cube"));
	UStaticMesh* Cyl = Mesh(TEXT("Cylinder"));
	UMaterialInterface* Dark = Mat(TEXT("MI_TD_Base_Dark"));
	UMaterialInterface* Rib = Mat(TEXT("MI_TD_Base_Rib"));
	UMaterialInterface* Accent = Mat(TEXT("MI_TD_Base_Accent"));
	switch (static_cast<ETDWorkVerb>(VerbIndex))
	{
	case ETDWorkVerb::Hammer:
		Set(HandToolA, Cyl, FVector(0.f, 0.f, 0.f), FVector(0.03f, 0.03f, 0.4f), FRotator::ZeroRotator, Dark);
		Set(HandToolB, Cube, FVector(0.f, 0.f, 20.f), FVector(0.14f, 0.06f, 0.06f), FRotator::ZeroRotator, Rib);
		break;
	case ETDWorkVerb::Scoop:
		Set(HandToolA, Cyl, FVector(0.f, 0.f, 0.f), FVector(0.03f, 0.03f, 0.5f), FRotator::ZeroRotator, Dark);
		Set(HandToolB, Cube, FVector(0.f, 0.f, -26.f), FVector(0.16f, 0.02f, 0.2f), FRotator::ZeroRotator, Rib);
		break;
	case ETDWorkVerb::Wrench:
	case ETDWorkVerb::Drill:
		Set(HandToolA, Cube, FVector(0.f, 0.f, 0.f), FVector(0.12f, 0.07f, 0.1f), FRotator::ZeroRotator, Accent);
		Set(HandToolB, Cyl, FVector(10.f, 0.f, 0.f), FVector(0.02f, 0.02f, 0.18f), FRotator(90.f, 0.f, 0.f), Rib);
		break;
	default: // tablet / scanner for cables, latches, panels, brushes, switches
		Set(HandToolA, Cube, FVector(0.f, 0.f, 0.f), FVector(0.12f, 0.02f, 0.09f), FRotator::ZeroRotator, Dark);
		Set(HandToolB, Cube, FVector(0.f, -1.2f, 0.f), FVector(0.1f, 0.004f, 0.07f), FRotator::ZeroRotator, Accent);
		break;
	}
}

void ATDCrewMember::HideHandTool()
{
	if (HandToolA) { HandToolA->SetVisibility(false); }
	if (HandToolB) { HandToolB->SetVisibility(false); }
}

void ATDCrewMember::FaceToward(const FVector& Point, float DeltaSeconds, float ThresholdDegrees)
{
	const FVector To = Point - BaseLocation;
	if (To.IsNearlyZero())
	{
		return;
	}
	const float Target = To.Rotation().Yaw - SuitForwardYaw;
	const float Delta = FRotator::NormalizeAxis(Target - BaseRotation.Yaw);
	if (FMath::Abs(Delta) < ThresholdDegrees && FMath::Abs(FacingSpring.Velocity) < 1.f)
	{
		UKismetMathLibrary::ResetFloatSpringState(FacingSpring);
		return; // do not track every small step
	}
	// Spring on the angle difference: slow, deliberate, one small overshoot. [DESIGNER VALUE]
	// Mass 1, no target velocity: the spring follows the angle difference only.
	const float Step = UKismetMathLibrary::FloatSpringInterp(0.f, Delta, FacingSpring, 8.f, 0.75f, DeltaSeconds, 1.f, 0.f);
	BaseRotation.Yaw += Step;
}

void ATDCrewMember::SetInside(bool bInside)
{
	bInsideModule = bInside;
	SetActorHiddenInGame(bInside);
	SetActorEnableCollision(!bInside);
}

void ATDCrewMember::Say(const FText& Line, float Seconds)
{
	Bark = Line;
	BarkUntil = FPlatformTime::Seconds() + Seconds;
}

FText ATDCrewMember::GetBark() const
{
	return FPlatformTime::Seconds() < BarkUntil ? Bark : FText::GetEmpty();
}

void ATDCrewMember::RunSteps(TArray<FTDCrewStep> Steps, float WalkSpeedCmPerSec, TFunction<void()> OnAllDone)
{
	StepQueue = MoveTemp(Steps);
	StepsDone = MoveTemp(OnAllDone);
	StepSpeed = WalkSpeedCmPerSec;
	bOnTask = true;
	RunNextStep();
}

void ATDCrewMember::RunNextStep()
{
	GetWorldTimerManager().ClearTimer(WorkTimer);
	if (bWorkRange && SuitMesh)
	{
		if (UAnimSingleNodeInstance* Single = Suit->GetSingleNodeInstance()) { Single->SetReverse(false); }
	}
	bWorkRange = false;
	HideHandTool();
	if (StepQueue.Num() == 0)
	{
		bOnTask = false;
		StatusText = FText::GetEmpty();
		StatusProgress = -1.f;
		SetInside(false);
		SetBasePose(HomeLocation, BaseRotation);
		PlayClip(IdleAnim);
		TFunction<void()> Done = MoveTemp(StepsDone);
		if (Done)
		{
			Done();
		}
		return;
	}
	const FTDCrewStep Step = StepQueue[0];
	StepQueue.RemoveAt(0);
	UE_LOG(LogTemp, Display, TEXT("Crew %s: step '%s', %d points, from %s to %s, work %.1f s%s"), *GetName(), *Step.Status.ToString(), Step.Points.Num(),
		*BaseLocation.ToCompactString(), Step.Points.Num() > 0 ? *Step.Points.Last().ToCompactString() : TEXT("-"), Step.WorkSeconds, Step.bInside ? TEXT(" inside") : TEXT(""));
	StatusText = Step.Status;
	StatusProgress = Step.WorkSeconds > 0.f ? 0.f : -1.f;
	WalkPath(Step.Points, StepSpeed, [this, Step]()
	{
		if (Step.WorkSeconds <= 0.f)
		{
			RunNextStep();
			return;
		}
		if (Step.bInside)
		{
			SetInside(true);
		}
		else
		{
			WorkAt(Step.Look, Step.TaskId);
		}
		WorkStart = FPlatformTime::Seconds();
		WorkLength = Step.WorkSeconds;
		GetWorldTimerManager().SetTimer(WorkTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			StatusProgress = FMath::Clamp(static_cast<float>((FPlatformTime::Seconds() - WorkStart) / FMath::Max(0.1f, WorkLength)), 0.f, 1.f);
			if (StatusProgress >= 1.f)
			{
				SetInside(false);
				RunNextStep();
			}
		}), 0.1f, true);
	});
}

void ATDCrewMember::StartEscort(AActor* Target)
{
	if (!Target || bOnTask)
	{
		return;
	}
	StopWalking();
	EscortTarget = Target;
	bEscortMoving = false;
	SetInside(false);
}

void ATDCrewMember::StopEscort()
{
	if (!IsEscorting())
	{
		return;
	}
	EscortTarget.Reset();
	bEscortMoving = false;
	if (!bOnTask)
	{
		WalkTo(HomeLocation, WalkClipSpeedCmPerSec, [this]() { SetBasePose(HomeLocation, BaseRotation); PlayClip(IdleAnim); });
	}
}

void ATDCrewMember::StepEscort(float DeltaSeconds)
{
	const AActor* Target = EscortTarget.Get();
	if (!Target)
	{
		return;
	}
	// Ellie's rule (The Last of Us): stay close and keep up without teleporting. In first person the buddy walks
	// side by side, a little ahead on the right, so the player sees them (an EVA pair walks together).
	FVector Forward = Target->GetActorForwardVector().GetSafeNormal2D();
	if (const APawn* Pawn = Cast<APawn>(Target); Pawn && Pawn->GetController())
	{
		Forward = Pawn->GetController()->GetControlRotation().Vector().GetSafeNormal2D();
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const FVector Slot = Target->GetActorLocation() + Forward * 160.f + Right * 230.f;
	FVector Flat(Slot.X - BaseLocation.X, Slot.Y - BaseLocation.Y, 0.f);
	const float Distance = Flat.Size();
	const bool bMove = bEscortMoving ? Distance > 60.f : Distance > 260.f; // hysteresis: no jitter at the edge
	if (bMove)
	{
		// Escort pace [DESIGNER VALUE]: 130 near, 150 mid; the fast catch-up only while the buddy is off screen, so
		// nobody sees a 4x walk (above about 109 cm/s is game pace, faster than a suited walk derived from Apollo).
		const bool bSeen = Suit && Suit->WasRecentlyRendered(0.25f);
		const float Speed = Distance > 1500.f ? (bSeen ? 150.f : 380.f) : (Distance > 600.f ? 150.f : 130.f);
		PlayClip(WalkAnim, WalkRate(Speed));
		bEscortMoving = true;
		UKismetMathLibrary::ResetFloatSpringState(FacingSpring);
		Flat /= FMath::Max(1.f, Distance);
		const float Step = FMath::Min(Distance, Speed * DeltaSeconds);
		BaseLocation += Flat * Step;
		BaseRotation.Yaw = FMath::FixedTurn(BaseRotation.Yaw, Flat.Rotation().Yaw - SuitForwardYaw, 360.f * DeltaSeconds);
	}
	else
	{
		if (bEscortMoving)
		{
			PlayClip(IdleAnim);
		}
		bEscortMoving = false;
		FaceToward(Target->GetActorLocation(), DeltaSeconds, 20.f);
	}
	BaseLocation.Z = GroundZ(BaseLocation) + FeetOffset;
	SetActorLocationAndRotation(BaseLocation, BaseRotation);
}
