// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDSceneActors.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Actors/TDWorkSpot.h"
#include "Actors/TDToolMeshes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "LandscapeProxy.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"

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
	// Movement runs every frame (a 30 Hz timer made walking judder against the render rate).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
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
	// Bones refresh even before the first render, so no frame of the bind (T) pose shows in the opening shots.
	Suit->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
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
	if (SuitMesh && IdleAnim)
	{
		// Construction-time single-node setup: the editor viewport and the first game frame show the idle pose.
		Suit->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Suit->OverrideAnimationData(IdleAnim, true, true, 0.f, 1.f);
	}
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
	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		Landscapes.Add(*It);
	}
	HomeLocation = GetActorLocation();
	HomeRotation = GetActorRotation();
	FeetOffset = 0.f;
	const float Ground = GroundZ(HomeLocation);
	FeetOffset = HomeLocation.Z - Ground;
	if (FMath::Abs(FeetOffset) < 40.f)
	{
		HomeLocation.Z = Ground; // feet on the regolith (suit pivot is at the feet)
		FeetOffset = 0.f;
	}
	IdleSeed = static_cast<float>(static_cast<int32>(System)) * 2.1f; // fixed phase per crew member, no random stream
	IdleRand.Initialize(static_cast<int32>(FCrc::StrCrc32(*GetName())));
	IdleAct = EIdleAct::Relax;
	IdleActUntil = IdleRand.FRandRange(1.5f, 6.f); // staggered first decision: the crew never move in lockstep
	SetBasePose(HomeLocation, HomeRotation);
	if (SuitMesh && IdleAnim)
	{
		PlayClip(IdleAnim);
		Suit->SetPosition(IdleRand.FRand() * IdleAnim->GetPlayLength(), false);
		Suit->TickAnimation(0.f, false);
		Suit->RefreshBoneTransforms();
	}
	UE_LOG(LogTemp, Display, TEXT("Crew %s: home %s, feet offset %.1f cm"), *GetName(), *HomeLocation.ToCompactString(), FeetOffset);
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

void ATDCrewMember::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StepIdle(FMath::Min(DeltaSeconds, 0.1f)); // a hitch never throws a crew member across the map
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
				if (IsWalking() && !bOnTask)
				{
					StopWalking();
				}
				if (bWorkRange && !bOnTask)
				{
					bWorkRange = false;
					HideHandTool();
				}
				if (!bOnTask)
				{
					IdleAct = EIdleAct::Relax;
					IdleActUntil = IdleTime + 3.f;
					ChatPartner.Reset();
					IdleClaim = FVector::ZeroVector;
				}
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
	if (bOnTask || bStaged || IsEscorting())
	{
		return;
	}
	// A new phase brings everyone back to their station. Seen crew walk there; unseen crew are simply placed.
	if (FVector::Dist2D(BaseLocation, HomeLocation) > 60.f)
	{
		StartIdleAct(EIdleAct::ReturnHome);
	}
}

void ATDCrewMember::SetBasePose(const FVector& Location, const FRotator& Rotation)
{
	UKismetMathLibrary::ResetFloatSpringState(FacingSpring);
	BaseLocation = Location;
	BaseRotation = Rotation;
	SetActorLocationAndRotation(Location, Rotation);
}

void ATDCrewMember::StepIdle(float DeltaSeconds)
{
	IdleTime += DeltaSeconds;
	StepWorkRange();
	if (ClipSwapAt >= 0.f && IdleTime >= ClipSwapAt && !IsWalking())
	{
		ClipSwapAt = -1.f;
		PlayClip(Current == ETDCrewBehaviour::Worried ? WorriedAnim.Get() : IdleAnim.Get());
	}
	if (IsWalking())
	{
		StepWalk(DeltaSeconds);
		return;
	}
	if (IsEscorting() && !bOnTask)
	{
		StepEscort(DeltaSeconds);
		return;
	}
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const bool bReduce = Mission && Mission->IsReduceMotion();
	if (!bReduce)
	{
		StepIdleLife(DeltaSeconds);
		if (IsWalking())
		{
			return; // idle life just set off somewhere
		}
	}
	// A8: an idle crew member at rest turns the whole suit toward the player when they come close (suits limit
	// head turns, so suited people turn the torso). [DESIGNER VALUE] 600 cm, 20 degrees.
	if (SuitMesh && !bOnTask && !bInsideModule && !bStaged && !bReduce && IdleAct == EIdleAct::Relax)
	{
		if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && FVector::Dist2D(Player->GetActorLocation(), BaseLocation) < 600.f)
		{
			FaceToward(Player->GetActorLocation(), DeltaSeconds, 20.f);
		}
		else if (!IdleFocus.IsZero())
		{
			FaceToward(IdleFocus, DeltaSeconds, 6.f); // settle back to the station's facing
		}
	}
	if (bReduce || SuitMesh)
	{
		SetActorLocationAndRotation(BaseLocation, BaseRotation);
		if (SuitMesh && Mission)
		{
			Suit->bPauseAnims = bReduce;
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
	const bool bIdleLifeBehaviour = Behaviour == ETDCrewBehaviour::Working || Behaviour == ETDCrewBehaviour::Worried;
	if (bIdleLifeBehaviour && SuitMesh && !bStaged && (IsWalking() || IsEscorting() || (IdleAct != EIdleAct::Relax && IdleAct != EIdleAct::LookAround)))
	{
		OnBehaviour(Behaviour); // idle life keeps its activity and picks the matching clip at the next one
		return;
	}
	if (!bIdleLifeBehaviour && !bStaged && !IsEscorting())
	{
		if (IsWalking())
		{
			StopWalking();
		}
		if (bWorkRange)
		{
			bWorkRange = false;
			HideHandTool();
		}
		IdleAct = EIdleAct::Relax;
	}
	switch (Behaviour)
	{
	case ETDCrewBehaviour::Worried: PlayClip(WorriedAnim); break;
	case ETDCrewBehaviour::Alarm: PlayClip(WorriedAnim, 1.6f); break;
	case ETDCrewBehaviour::Celebrate: PlayClip(CelebrateAnim); break;
	case ETDCrewBehaviour::AtLander: PlayClip(FarewellAnim); break;
	default: PlayClip(IdleAnim); break;
	}
	if (!bStaged && !IsEscorting())
	{
		const bool bHomeBehaviour = bIdleLifeBehaviour || Behaviour == ETDCrewBehaviour::Alarm;
		if (bHomeBehaviour && SuitMesh && FVector::Dist2D(BaseLocation, HomeLocation) > 60.f)
		{
			if (Behaviour == ETDCrewBehaviour::Alarm)
			{
				StartIdleAct(EIdleAct::ReturnHome); // back to the station, on foot (no pop)
			}
		}
		else if (!bHomeBehaviour || FVector::Dist2D(BaseLocation, Location) < 60.f)
		{
			SetBasePose(FVector(Location.X, Location.Y, bHomeBehaviour ? HomeLocation.Z : GroundZ(Location) + FeetOffset), BaseRotation);
		}
	}
	OnBehaviour(Behaviour);
}

// Crew walking and tasks (surface operations)

float ATDCrewMember::GroundZ(const FVector& At) const
{
	// Feet follow the terrain only: a trace on WorldStatic also hit crates, rocks and module roofs, which lifted the
	// crew into the air. The highest landscape under the point wins (the near DEM sits on the far terrain).
	bool bFound = false;
	float Best = 0.f;
	for (const TWeakObjectPtr<ALandscapeProxy>& Landscape : Landscapes)
	{
		if (const ALandscapeProxy* Proxy = Landscape.Get())
		{
			const TOptional<float> Z = Proxy->GetHeightAtLocation(At);
			if (Z.IsSet() && (!bFound || Z.GetValue() > Best))
			{
				Best = Z.GetValue();
				bFound = true;
			}
		}
	}
	if (bFound)
	{
		return Best;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(TEXT("TDCrewGround"), false, this);
	const FVector Start(At.X, At.Y, At.Z + 300.f);
	const FVector End(At.X, At.Y, At.Z - 3000.f);
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
	// Turn in place first when the first leg points well away from the facing (no moonwalk, no sideways slide).
	bTurningToPath = false;
	for (int32 Index = 1; SuitMesh && Index < PathPoints.Num(); ++Index)
	{
		const FVector To = PathPoints[Index] - BaseLocation;
		if (To.Size2D() > 40.f)
		{
			bTurningToPath = FMath::Abs(FRotator::NormalizeAxis(To.Rotation().Yaw - SuitForwardYaw - BaseRotation.Yaw)) > 35.f;
			break;
		}
	}
	PlayClip(WalkAnim, bTurningToPath ? 0.5f : WalkRate(WalkSpeed));
}

void ATDCrewMember::StopWalking()
{
	PathPoints.Reset();
	PathIndex = 0;
	ArriveCallback = nullptr;
	bTurningToPath = false;
	PlayClip(IdleAnim);
}

void ATDCrewMember::StepWalk(float DeltaSeconds)
{
	// Corners: a middle point counts as reached within 40 cm, so the crew round corners instead of stopping dead
	// on each grid node (Reynolds path following; no exact-equality arrival).
	while (PathPoints.IsValidIndex(PathIndex) && PathIndex < PathPoints.Num() - 1
		&& FVector::Dist2D(PathPoints[PathIndex], BaseLocation) < 40.f)
	{
		++PathIndex;
	}
	if (!PathPoints.IsValidIndex(PathIndex))
	{
		TFunction<void()> Done = MoveTemp(ArriveCallback);
		PathPoints.Reset();
		PathIndex = 0;
		bTurningToPath = false;
		PlayClip(IdleAnim);
		if (Done)
		{
			Done();
		}
		return;
	}
	const bool bLast = PathIndex == PathPoints.Num() - 1;
	const FVector Goal = PathPoints[PathIndex];
	FVector Flat(Goal.X - BaseLocation.X, Goal.Y - BaseLocation.Y, 0.f);
	const float Distance = Flat.Size();
	if (Distance < 0.5f)
	{
		++PathIndex;
		return;
	}
	const FVector Dir = Flat / Distance;
	const float TargetYaw = Dir.Rotation().Yaw - SuitForwardYaw;
	UKismetMathLibrary::ResetFloatSpringState(FacingSpring);
	if (bTurningToPath)
	{
		// Suited turn: about 90 degrees a second with small steps, faster in the time-lapse. [DESIGNER VALUE]
		const float Rate = 90.f * FMath::Max(1.f, WalkSpeed / 100.f);
		BaseRotation.Yaw = FMath::FixedTurn(BaseRotation.Yaw, TargetYaw, Rate * DeltaSeconds);
		SetActorLocationAndRotation(BaseLocation, BaseRotation);
		if (FMath::Abs(FRotator::NormalizeAxis(TargetYaw - BaseRotation.Yaw)) < 20.f)
		{
			bTurningToPath = false;
			if (SuitMesh) { Suit->SetPlayRate(WalkRate(WalkSpeed)); }
		}
		return;
	}
	// A8: slow down over the last 80 cm (Reynolds arrival) instead of popping from mid-stride into the idle; the walk
	// play rate follows the eased speed so the feet stay planted. [DESIGNER VALUE]
	float Speed = WalkSpeed;
	if (bLast && Distance < 80.f)
	{
		Speed = WalkSpeed * FMath::Clamp(Distance / 80.f, 0.45f, 1.f);
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
		// Separation: step around colleagues and the player, capped at 30 % of the walking speed.
		Next += Dir * Step + AvoidPush(Dir) * (0.3f * Step);
		BaseRotation = FMath::RInterpTo(BaseRotation, FRotator(BaseRotation.Pitch, TargetYaw, BaseRotation.Roll), DeltaSeconds, 7.f);
	}
	const float TargetZ = GroundZ(Next) + FeetOffset;
	Next.Z = FMath::Abs(TargetZ - BaseLocation.Z) > 60.f ? TargetZ : FMath::FInterpTo(BaseLocation.Z, TargetZ, DeltaSeconds, 12.f);
	StepTravel += FVector::Dist2D(Next, BaseLocation);
	if (StepTravel > 75.f)
	{
		StepTravel = 0.f;
		StepSound();
	}
	BaseLocation = Next;
	SetActorLocationAndRotation(BaseLocation, BaseRotation);
}

void ATDCrewMember::StepSound()
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	if (!SuitMesh || bInsideModule || WalkSpeed > 200.f || !Mission || !Mission->IsSoundOn())
	{
		return; // no steps in the time-lapse (they would rattle) or while hidden
	}
	static TWeakObjectPtr<USoundAttenuation> Falloff;
	if (!Falloff.IsValid())
	{
		USoundAttenuation* Settings = NewObject<USoundAttenuation>(GetTransientPackage());
		Settings->AddToRoot();
		Settings->Attenuation.bAttenuate = true;
		Settings->Attenuation.bSpatialize = true;
		Settings->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		Settings->Attenuation.AttenuationShapeExtents = FVector(200.f);
		Settings->Attenuation.FalloffDistance = 1300.f;
		Falloff = Settings;
	}
	const FString Name = FString::Printf(TEXT("SFX_TD_Step_Regolith_%02d"), FMath::RandRange(1, 4));
	if (USoundWave* Wave = LoadObject<USoundWave>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Audio/%s.%s"), *Name, *Name)))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Wave, BaseLocation, 0.5f, FMath::FRandRange(0.9f, 1.06f), 0.f, Falloff.Get());
	}
}

FVector ATDCrewMember::AvoidPush(const FVector& Dir) const
{
	FVector Push = FVector::ZeroVector;
	auto Add = [this, &Push, &Dir](const FVector& Other)
	{
		FVector Away(BaseLocation.X - Other.X, BaseLocation.Y - Other.Y, 0.f);
		const float D = Away.Size();
		if (D < 1.f || D > 120.f)
		{
			return;
		}
		Away /= D;
		if (FVector::DotProduct(-Away, Dir) < -0.2f && D > 70.f)
		{
			return; // behind: only a very close neighbour counts
		}
		Push += Away * ((120.f - D) / 120.f);
	};
	for (TActorIterator<ATDCrewMember> It(GetWorld()); It; ++It)
	{
		if (*It != this && !It->IsInside() && !It->IsHidden())
		{
			Add(It->BaseLocation);
		}
	}
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Add(Player->GetActorLocation());
	}
	if (Push.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	// Sideways part only (never walk backwards); head-on, keep to the right.
	FVector Lateral = Push - Dir * FVector::DotProduct(Push, Dir);
	if (Lateral.SizeSquared() < 0.0025f)
	{
		Lateral = FVector::CrossProduct(FVector::UpVector, Dir) * Push.Size();
	}
	return Lateral.GetClampedToMaxSize(1.f);
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
	IdleAct = EIdleAct::Relax;
	ChatPartner.Reset();
	IdleClaim = FVector::ZeroVector;
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
		PlayClip(IdleAnim);
		// Never stand frozen after a task and never pop home: walk back (idle life takes over from there).
		StartIdleAct(EIdleAct::ReturnHome);
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
	if (bWorkRange)
	{
		bWorkRange = false;
		HideHandTool();
	}
	IdleAct = EIdleAct::Relax;
	EscortTarget = Target;
	bEscortMoving = false;
	bEscortDetour = false;
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
	bEscortDetour = false;
	if (!bOnTask)
	{
		StartIdleAct(EIdleAct::ReturnHome);
	}
}

bool ATDCrewMember::EscortLineBlocked(const FVector& To) const
{
	// A crew-sized sphere at waist height along the straight line, against everything but the terrain and people.
	FCollisionQueryParams Params(TEXT("TDCrewEscort"), false, this);
	for (const TWeakObjectPtr<ALandscapeProxy>& Landscape : Landscapes)
	{
		Params.AddIgnoredActor(Landscape.Get());
	}
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(Player);
	}
	const FVector A(BaseLocation.X, BaseLocation.Y, BaseLocation.Z + 80.f);
	const FVector B(To.X, To.Y, BaseLocation.Z + 80.f);
	FHitResult Hit;
	return GetWorld()->SweepSingleByObjectType(Hit, A, B, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeSphere(35.f), Params);
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
	// Twice a second: is a module or crate in the way? Then follow the walk grid around it instead of through it.
	if (IdleTime >= EscortCheckAt)
	{
		EscortCheckAt = IdleTime + 0.5f;
		bEscortDetour = false;
		if (FVector::Dist2D(Slot, BaseLocation) > 120.f && EscortLineBlocked(Slot))
		{
			if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
			{
				const TArray<FVector> Path = Mission->FindCrewPath(BaseLocation, Slot);
				if (Path.Num() > 1)
				{
					EscortWaypoint = Path[0];
					bEscortDetour = true;
				}
			}
		}
	}
	const FVector Aim = bEscortDetour ? EscortWaypoint : Slot;
	FVector Flat(Aim.X - BaseLocation.X, Aim.Y - BaseLocation.Y, 0.f);
	const float Distance = Flat.Size();
	const bool bMove = bEscortDetour || (bEscortMoving ? Distance > 60.f : Distance > 260.f); // hysteresis: no jitter at the edge
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
		BaseRotation = FMath::RInterpTo(BaseRotation, FRotator(BaseRotation.Pitch, Flat.Rotation().Yaw - SuitForwardYaw, BaseRotation.Roll), DeltaSeconds, 6.f);
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
	const float TargetZ = GroundZ(BaseLocation) + FeetOffset;
	BaseLocation.Z = FMath::Abs(TargetZ - BaseLocation.Z) > 60.f ? TargetZ : FMath::FInterpTo(BaseLocation.Z, TargetZ, DeltaSeconds, 12.f);
	SetActorLocationAndRotation(BaseLocation, BaseRotation);
}

// Crew idle life: nobody stands frozen when nothing is assigned.
// Sources (research_notes/Crew AI canlılığı): smart objects with claim and release (Epic Smart Objects; Forbus and
// Wright, The Sims, p.3), weighted random choice instead of argmax (Mark and Dill, GDC 2010), a novelty cooldown
// (RimWorld tolerance; Horizon Forbidden West GDC 2023), face-to-face talk spots (Final Fantasy XV), Reynolds arrival
// and separation (GDC 1999), and the analog-mission activities a reviewer expects: instrument and panel checks, a look
// at the horizon for dust, rock sampling near the station, buddy check-ins (CHAPEA; MDRS Handbook p.22, p.38).

bool ATDCrewMember::IsIdleFree() const
{
	return SuitMesh && !bOnTask && !IsEscorting() && !bInsideModule && !bStaged && !IsWalking()
		&& Current == ETDCrewBehaviour::Working && (IdleAct == EIdleAct::Relax || IdleAct == EIdleAct::LookAround)
		&& IdleTime >= NextChatAt;
}

void ATDCrewMember::JoinChat(const ATDCrewMember* Other, float Seconds)
{
	if (!Other || !IsIdleFree())
	{
		return;
	}
	IdleAct = EIdleAct::Listen;
	ChatPartner = const_cast<ATDCrewMember*>(Other);
	IdleFocus = Other->BaseLocation;
	IdleActUntil = IdleTime + Seconds;
	NextChatAt = IdleTime + Seconds + 60.f;
	ClipSwapAt = -1.f;
	PlayClip(IdleAnim);
}

ATDCrewMember* ATDCrewMember::FindChatPartner() const
{
	if (IdleTime < NextChatAt)
	{
		return nullptr;
	}
	ATDCrewMember* Best = nullptr;
	float BestDistance = 2000.f; // [DESIGNER VALUE] talk to someone within 20 m
	for (TActorIterator<ATDCrewMember> It(GetWorld()); It; ++It)
	{
		if (*It == this || !It->IsIdleFree())
		{
			continue;
		}
		const float D = FVector::Dist2D(It->BaseLocation, BaseLocation);
		if (D < BestDistance)
		{
			BestDistance = D;
			Best = *It;
		}
	}
	return Best;
}

void ATDCrewMember::GatherIdlePoints()
{
	bIdlePointsReady = true;
	IdlePoints.Reset();
	IdleLooks.Reset();
	auto AddSpots = [this](const FBox& Box, float Stand)
	{
		if (!Box.IsValid || Box.GetSize().X > 4000.f || Box.GetSize().Y > 4000.f || Box.GetSize().Z < 40.f)
		{
			return;
		}
		// Up to three stand spots per object: the face toward home and the faces to either side of it.
		const FVector Center = Box.GetCenter();
		FVector ToHome(HomeLocation.X - Center.X, HomeLocation.Y - Center.Y, 0.f);
		if (ToHome.Size() > 1800.f || ToHome.Size() < 1.f)
		{
			return;
		}
		ToHome.Normalize();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, ToHome);
		const FVector Probes[] = { HomeLocation, Center + (ToHome + Side) * 1500.f, Center + (ToHome - Side) * 1500.f };
		for (const FVector& Probe : Probes)
		{
			const FVector Closest(FMath::Clamp(Probe.X, Box.Min.X, Box.Max.X), FMath::Clamp(Probe.Y, Box.Min.Y, Box.Max.Y), HomeLocation.Z);
			FVector Out(Probe.X - Closest.X, Probe.Y - Closest.Y, 0.f);
			if (Out.Size() < 1.f)
			{
				continue;
			}
			Out.Normalize();
			const FVector StandAt = Closest + Out * Stand;
			bool bNear = false;
			for (const FVector& Existing : IdlePoints)
			{
				bNear |= FVector::Dist2D(Existing, StandAt) < 150.f;
			}
			if (!bNear)
			{
				IdlePoints.Add(FVector(StandAt.X, StandAt.Y, HomeLocation.Z));
				IdleLooks.Add(FVector(Closest.X, Closest.Y, HomeLocation.Z));
			}
		}
	};
	auto MeshBox = [](const AActor* Actor)
	{
		FBox Box(ForceInit);
		TArray<UStaticMeshComponent*> Meshes;
		Actor->GetComponents<UStaticMeshComponent>(Meshes);
		for (const UStaticMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh->IsVisible() && Mesh->GetStaticMesh())
			{
				Box += Mesh->Bounds.GetBox();
			}
		}
		return Box;
	};
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const AActor* Actor = *It;
		if (Actor->IsHidden() || Actor == this || Actor->IsA<ATDCrewMember>())
		{
			continue;
		}
		if (Actor->IsA<ATDSystemModule>() || Actor->IsA<ATDWorkSpot>())
		{
			AddSpots(MeshBox(Actor), Actor->IsA<ATDWorkSpot>() ? 110.f : 90.f);
			continue;
		}
		// Placed NASA and Tripo models (panels, tanks, crates, consoles) are points of interest too.
		if (const UStaticMeshComponent* Root = Cast<UStaticMeshComponent>(Actor->GetRootComponent()); Root && Root->GetStaticMesh())
		{
			const FString Path = Root->GetStaticMesh()->GetPathName();
			if (Path.Contains(TEXT("/Tripo/")) || Path.Contains(TEXT("/NASA/")))
			{
				AddSpots(MeshBox(Actor), 90.f);
			}
		}
	}
	UE_LOG(LogTemp, Verbose, TEXT("Crew %s: %d idle spots"), *GetName(), IdlePoints.Num());
}

int32 ATDCrewMember::PickIdleSpot()
{
	struct FCandidate { int32 Index; float Score; };
	TArray<FCandidate> Candidates;
	for (int32 Index = 0; Index < IdlePoints.Num(); ++Index)
	{
		const FVector& Spot = IdlePoints[Index];
		const float D = FVector::Dist2D(Spot, BaseLocation);
		if (D < 60.f || D > 1600.f)
		{
			continue;
		}
		bool bClaimed = false;
		for (TActorIterator<ATDCrewMember> It(GetWorld()); It && !bClaimed; ++It)
		{
			bClaimed = *It != this && !It->IdleClaim.IsZero() && FVector::Dist2D(It->IdleClaim, Spot) < 200.f;
		}
		if (bClaimed)
		{
			continue;
		}
		float Score = 1.f / (1.f + D / 600.f); // nearer spots advertise louder (Sims attenuation)
		if (const float* Used = IdleSpotUsed.Find(FIntPoint(FMath::RoundToInt(Spot.X / 50.f), FMath::RoundToInt(Spot.Y / 50.f))))
		{
			if (IdleTime - *Used < 90.f)
			{
				Score *= 0.4f; // novelty cooldown
			}
		}
		Candidates.Add({ Index, Score * static_cast<float>(IdleRand.FRandRange(0.8f, 1.2f)) });
	}
	if (Candidates.Num() == 0)
	{
		return INDEX_NONE;
	}
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Score > B.Score; });
	const int32 Top = FMath::Min(3, Candidates.Num());
	float Total = 0.f;
	for (int32 I = 0; I < Top; ++I)
	{
		Total += Candidates[I].Score;
	}
	float Roll = IdleRand.FRandRange(0.f, Total);
	for (int32 I = 0; I < Top; ++I)
	{
		Roll -= Candidates[I].Score;
		if (Roll <= 0.f)
		{
			return Candidates[I].Index;
		}
	}
	return Candidates[0].Index;
}

bool ATDCrewMember::WalkIdleTo(const FVector& Target, TFunction<void()> OnArrived)
{
	TArray<FVector> Points;
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Points = Mission->FindCrewPath(BaseLocation, Target);
	}
	if (Points.Num() == 0)
	{
		if (FVector::Dist2D(BaseLocation, Target) > 200.f)
		{
			return false; // no way around: skip this activity rather than walk through a module
		}
		Points.Add(Target);
	}
	// A suited stroll: a little under the walk clip's own pace. [DESIGNER VALUE]
	WalkPath(Points, 0.92f * WalkClipSpeedCmPerSec, MoveTemp(OnArrived));
	return true;
}

void ATDCrewMember::BeginWorkPose(bool bGround, int32 VerbIndex)
{
	if (!WorkAnim || !SuitMesh)
	{
		PlayClip(IdleAnim);
		return;
	}
	PlayClip(WorkAnim);
	WorkRangeStart = bGround ? 0.2f : 6.0f;
	WorkRangeEnd = bGround ? 3.7f : 13.4f;
	bWorkRange = true;
	Suit->SetPosition(WorkRangeStart + IdleRand.FRand() * (WorkRangeEnd - WorkRangeStart) * 0.5f, false);
	ShowHandTool(VerbIndex);
}

void ATDCrewMember::BeginInspect(const FVector& Look)
{
	// Instrument or panel check at a module, tablet in hand (MDRS daily operations checks).
	IdleAct = EIdleAct::Inspect;
	IdleFocus = Look;
	BeginWorkPose(false, static_cast<int32>(ETDWorkVerb::Scan));
	IdleActUntil = IdleTime + IdleRand.FRandRange(7.f, 13.f);
}

void ATDCrewMember::BeginChat(float Seconds)
{
	ATDCrewMember* Partner = ChatPartner.Get();
	if (!Partner || Partner->ChatPartner.Get() != this)
	{
		StartIdleAct(EIdleAct::Relax);
		return;
	}
	// Buddy check-in: both face each other, a greeting, then a relaxed stand (FFXV face-to-face spots).
	IdleAct = EIdleAct::Chat;
	IdleFocus = Partner->BaseLocation;
	IdleActUntil = IdleTime + Seconds;
	Partner->IdleActUntil = FMath::Max(Partner->IdleActUntil, Partner->IdleTime + Seconds);
	if (CelebrateAnim)
	{
		PlayClip(CelebrateAnim);
		ClipSwapAt = IdleTime + FMath::Min(3.5f, CelebrateAnim->GetPlayLength());
		Partner->PlayClip(CelebrateAnim);
		Partner->ClipSwapAt = Partner->IdleTime + 0.6f + FMath::Min(3.f, CelebrateAnim->GetPlayLength());
	}
}

void ATDCrewMember::StepIdleLife(float DeltaSeconds)
{
	const bool bCan = SuitMesh && !bOnTask && !IsEscorting() && !bInsideModule && !bStaged
		&& (Current == ETDCrewBehaviour::Working || Current == ETDCrewBehaviour::Worried);
	if (!bCan)
	{
		return;
	}
	switch (IdleAct)
	{
	case EIdleAct::Inspect:
		FaceToward(IdleFocus, DeltaSeconds, 5.f);
		break;
	case EIdleAct::Chat:
	case EIdleAct::Listen:
		if (const ATDCrewMember* Partner = ChatPartner.Get())
		{
			IdleFocus = Partner->BaseLocation;
		}
		FaceToward(IdleFocus, DeltaSeconds, 8.f);
		break;
	default:
		break;
	}
	if (IdleTime >= IdleActUntil)
	{
		PickNextIdleAct();
	}
}

void ATDCrewMember::PickNextIdleAct()
{
	if (bWorkRange)
	{
		if (UAnimSingleNodeInstance* Single = Suit->GetSingleNodeInstance()) { Single->SetReverse(false); }
		bWorkRange = false;
		HideHandTool();
	}
	const EIdleAct Last = IdleAct;
	ChatPartner.Reset();
	IdleClaim = FVector::ZeroVector;
	const bool bWorried = Current == ETDCrewBehaviour::Worried;
	const bool bAway = FVector::Dist2D(BaseLocation, HomeLocation) > 100.f;
	// Back to the station after one or two activities out (a crew member has a post to return to).
	if (bAway && (IdleActsAway >= 2 || IdleRand.FRand() < 0.45f))
	{
		StartIdleAct(EIdleAct::ReturnHome);
		return;
	}
	if (!bAway || !bIdlePointsReady)
	{
		GatherIdlePoints(); // deployed parts appear during the mission
	}
	// Weighted random [DESIGNER VALUE]; a worried crew member checks instruments and the sky more and does not chat.
	const bool bColleague = !bWorried && FindChatPartner() != nullptr;
	float Weights[5] = {
		bWorried ? 2.f : 3.f,                                // Relax
		bWorried ? 3.5f : 2.f,                               // LookAround
		IdlePoints.Num() > 0 ? (bWorried ? 4.5f : 3.f) : 0.f, // Inspect
		bColleague ? 1.6f : 0.f,                             // Chat
		(!bAway && !bWorried) ? 1.6f : 0.f                   // Stroll
	};
	const EIdleAct Acts[5] = { EIdleAct::Relax, EIdleAct::LookAround, EIdleAct::Inspect, EIdleAct::Chat, EIdleAct::Stroll };
	float Total = 0.f;
	for (int32 I = 0; I < 5; ++I)
	{
		if (Acts[I] == Last && Last != EIdleAct::Relax)
		{
			Weights[I] *= 0.3f; // rarely the same thing twice in a row
		}
		Total += Weights[I];
	}
	float Roll = IdleRand.FRandRange(0.f, Total);
	EIdleAct Pick = EIdleAct::Relax;
	for (int32 I = 0; I < 5; ++I)
	{
		Roll -= Weights[I];
		if (Roll <= 0.f && Weights[I] > 0.f)
		{
			Pick = Acts[I];
			break;
		}
	}
	StartIdleAct(Pick);
}

void ATDCrewMember::StartIdleAct(EIdleAct Act)
{
	const bool bWorried = Current == ETDCrewBehaviour::Worried;
	UAnimSequence* Rest = bWorried && WorriedAnim ? WorriedAnim.Get() : IdleAnim.Get();
	IdleAct = Act;
	IdleActUntil = 1.0e9f; // walking activities set their length on arrival
	UE_LOG(LogTemp, Verbose, TEXT("Crew %s: idle act %d at %s (%d spots)"), *GetName(), static_cast<int32>(Act), *BaseLocation.ToCompactString(), IdlePoints.Num());
	ClipSwapAt = -1.f;
	switch (Act)
	{
	case EIdleAct::Relax:
	case EIdleAct::Listen:
		IdleAct = EIdleAct::Relax;
		PlayClip(Rest);
		IdleActUntil = IdleTime + IdleRand.FRandRange(5.f, 11.f);
		break;
	case EIdleAct::LookAround:
		// Scan the horizon and the sky for dust (weather watch).
		PlayClip(WorriedAnim ? WorriedAnim.Get() : IdleAnim.Get(), 0.8f);
		IdleActUntil = IdleTime + IdleRand.FRandRange(4.5f, 8.f);
		break;
	case EIdleAct::Inspect:
	{
		const int32 Spot = PickIdleSpot();
		if (Spot == INDEX_NONE)
		{
			StartIdleAct(EIdleAct::LookAround);
			return;
		}
		const FVector Stand = IdlePoints[Spot];
		const FVector Look = IdleLooks[Spot];
		IdleClaim = Stand;
		IdleSpotUsed.Add(FIntPoint(FMath::RoundToInt(Stand.X / 50.f), FMath::RoundToInt(Stand.Y / 50.f)), IdleTime);
		++IdleActsAway;
		if (!WalkIdleTo(Stand, [this, Look]() { BeginInspect(Look); }))
		{
			IdleClaim = FVector::ZeroVector;
			StartIdleAct(EIdleAct::Relax);
		}
		break;
	}
	case EIdleAct::Chat:
	{
		ATDCrewMember* Other = FindChatPartner();
		if (!Other)
		{
			StartIdleAct(EIdleAct::Relax);
			return;
		}
		const float ChatSeconds = IdleRand.FRandRange(6.f, 10.f);
		FVector ToMe(BaseLocation.X - Other->BaseLocation.X, BaseLocation.Y - Other->BaseLocation.Y, 0.f);
		const float D = ToMe.Size();
		const FVector Meet = D > 1.f ? Other->BaseLocation + ToMe / D * 150.f : BaseLocation;
		const float WalkSeconds = FMath::Max(0.f, D - 150.f) / FMath::Max(10.f, 0.92f * WalkClipSpeedCmPerSec) + 3.f;
		ChatPartner = Other;
		NextChatAt = IdleTime + 60.f;
		Other->JoinChat(this, WalkSeconds + ChatSeconds);
		if (D < 230.f)
		{
			BeginChat(ChatSeconds);
		}
		else
		{
			++IdleActsAway;
			if (!WalkIdleTo(Meet, [this, ChatSeconds]() { BeginChat(ChatSeconds); }))
			{
				StartIdleAct(EIdleAct::Relax);
			}
		}
		break;
	}
	case EIdleAct::Stroll:
	{
		// A short walk out from the station: kneel at the regolith to sample it, or stop and scan the horizon.
		const float Angle = IdleRand.FRandRange(0.f, 2.f * PI);
		const float Distance = IdleRand.FRandRange(250.f, 500.f);
		const FVector Target = HomeLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Distance;
		const bool bSample = WorkAnim && IdleRand.FRand() < 0.5f;
		IdleClaim = Target;
		++IdleActsAway;
		if (!WalkIdleTo(Target, [this, bSample]()
			{
				if (bSample)
				{
					IdleAct = EIdleAct::Inspect;
					IdleFocus = BaseLocation + FRotator(0.f, BaseRotation.Yaw + SuitForwardYaw, 0.f).Vector() * 100.f;
					BeginWorkPose(true, static_cast<int32>(ETDWorkVerb::Scoop));
					IdleActUntil = IdleTime + IdleRand.FRandRange(6.f, 10.f);
				}
				else
				{
					IdleAct = EIdleAct::LookAround;
					PlayClip(WorriedAnim ? WorriedAnim.Get() : IdleAnim.Get(), 0.8f);
					IdleActUntil = IdleTime + IdleRand.FRandRange(4.f, 7.f);
				}
			}))
		{
			IdleClaim = FVector::ZeroVector;
			StartIdleAct(EIdleAct::LookAround);
		}
		break;
	}
	case EIdleAct::ReturnHome:
	{
		const FVector HomeFace = HomeLocation + FRotator(0.f, HomeRotation.Yaw + SuitForwardYaw, 0.f).Vector() * 1000.f;
		auto Arrive = [this, HomeFace]()
		{
			IdleActsAway = 0;
			IdleAct = EIdleAct::Relax;
			IdleFocus = HomeFace;
			PlayClip(Current == ETDCrewBehaviour::Worried && WorriedAnim ? WorriedAnim.Get() : IdleAnim.Get());
			IdleActUntil = IdleTime + IdleRand.FRandRange(4.f, 9.f);
		};
		if (FVector::Dist2D(BaseLocation, HomeLocation) < 40.f)
		{
			Arrive();
			break;
		}
		if (!(SuitMesh && Suit->WasRecentlyRendered(1.f)))
		{
			// Nobody is looking: just be at the station.
			StopWalking();
			SetBasePose(HomeLocation, HomeRotation);
			Arrive();
			break;
		}
		if (!WalkIdleTo(HomeLocation, Arrive))
		{
			WalkPath({ HomeLocation }, 0.92f * WalkClipSpeedCmPerSec, Arrive);
		}
		break;
	}
	}
}
