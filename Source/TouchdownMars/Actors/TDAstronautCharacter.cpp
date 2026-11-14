// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDAstronautCharacter.h"
#include "Model/TDOperations.h"

#include "Actors/TDPlayerController.h"
#include "Actors/TDSceneActors.h"
#include "UI/TDWidgets.h"
#include "Actors/TDSystemModule.h"
#include "Actors/TDWorkSpot.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DataTable.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Model/TDTypes.h"
#include "Systems/TDMissionSubsystem.h"
#include "TouchdownMars.h"

ATDAstronautCharacter::ATDAstronautCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->bUsePawnControlRotation = true;
	bUseControllerRotationYaw = true;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = WalkSpeed;
	Movement->JumpZVelocity = 260.f; // small hops; with Mars gravity they last about 1.4 s
	Movement->AirControl = 0.25f;
	Movement->BrakingDecelerationWalking = 600.f; // suited walking: heavy, slow to stop
}

void ATDAstronautCharacter::BeginPlay()
{
	Super::BeginPlay();
	Camera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeight));
	Camera->SetFieldOfView(FieldOfView);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	ApplyMarsGravity();
	SetConsoleOpen(false);
	ATDMissionControlStation::Ensure(GetWorld());
	// First-person tool: a grip point low right of the view; the parts are set per verb (BuildTool).
	ToolRoot = NewObject<USceneComponent>(this, TEXT("ToolRoot"));
	ToolRoot->SetupAttachment(Camera);
	ToolRoot->SetRelativeLocation(FVector(48.f, 26.f, -30.f));
	ToolRoot->RegisterComponent();
	ToolA = NewObject<UStaticMeshComponent>(this, TEXT("ToolA"));
	ToolB = NewObject<UStaticMeshComponent>(this, TEXT("ToolB"));
	for (UStaticMeshComponent* Part : { ToolA.Get(), ToolB.Get() })
	{
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetupAttachment(ToolRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetVisibility(false);
		Part->RegisterComponent();
	}
}

bool ATDAstronautCharacter::ApplyMarsGravity()
{
	// Mars surface gravity as a ratio of Earth's, from constants.csv (NASA NSSDC Mars fact sheet).
	// The tables load when the mission starts, which can be after this pawn's BeginPlay: retry on Tick.
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const UDataTable* Constants = Mission ? Mission->GetConstantsTable() : nullptr;
	const FTDConstantRow* Row = Constants ? Constants->FindRow<FTDConstantRow>(TEXT("MarsGravityRatio"), TEXT("Astronaut"), false) : nullptr;
	if (!Row)
	{
		return false;
	}
	GravityRatio = static_cast<float>(Row->Value);
	GetCharacterMovement()->GravityScale = GravityRatio;
	bGravityApplied = true;
	UE_LOG(LogTouchdownMars, Display, TEXT("Astronaut: Mars gravity ratio %.3f (constants.csv MarsGravityRatio)"), GravityRatio);
	return true;
}

void ATDAstronautCharacter::SyncHud()
{
	if (const ATDPlayerController* PC = Cast<ATDPlayerController>(GetController()))
	{
		if (UTDHudWidget* Hud = PC->GetMissionHud())
		{
			Hud->SetConsoleMode(bConsoleOpen);
			bHudSynced = true;
		}
	}
}

void ATDAstronautCharacter::CreateInput()
{
	if (Context)
	{
		return;
	}
	Context = NewObject<UInputMappingContext>(this, TEXT("IMC_TD_Astronaut"));
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_TD_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	LookAction = NewObject<UInputAction>(this, TEXT("IA_TD_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;
	JumpAction = NewObject<UInputAction>(this, TEXT("IA_TD_Jump"));
	ConsoleAction = NewObject<UInputAction>(this, TEXT("IA_TD_Console"));
	OpsAction = NewObject<UInputAction>(this, TEXT("IA_TD_Ops"));
	InteractAction = NewObject<UInputAction>(this, TEXT("IA_TD_Interact"));

	auto MapMove = [this](FKey Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		}
	};
	MapMove(EKeys::W, true, false);   // forward = +Y of the 2D value
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);  // right = +X
	MapMove(EKeys::A, false, true);
	MapMove(EKeys::Up, true, false);
	MapMove(EKeys::Down, true, true);
	MapMove(EKeys::Right, false, false);
	MapMove(EKeys::Left, false, true);
	Context->MapKey(LookAction, EKeys::Mouse2D);
	Context->MapKey(JumpAction, EKeys::SpaceBar);
	Context->MapKey(ConsoleAction, EKeys::Tab);
	Context->MapKey(ConsoleAction, EKeys::RightMouseButton);
	Context->MapKey(OpsAction, EKeys::B);
	Context->MapKey(InteractAction, EKeys::E);

	// Gamepad (Xbox layout): left stick walks, right stick looks, A jumps, X does your task, Y opens the mission
	// console, View opens the base computer. UI screens use the pad cursor (UI/TDGamepadCursor).
	{
		FEnhancedActionKeyMapping& PadMove = Context->MapKey(MoveAction, EKeys::Gamepad_Left2D);
		UInputModifierDeadZone* MoveDead = NewObject<UInputModifierDeadZone>(this);
		MoveDead->LowerThreshold = 0.2f;
		PadMove.Modifiers.Add(MoveDead);

		FEnhancedActionKeyMapping& PadLook = Context->MapKey(LookAction, EKeys::Gamepad_Right2D);
		UInputModifierDeadZone* LookDead = NewObject<UInputModifierDeadZone>(this);
		LookDead->LowerThreshold = 0.15f;
		PadLook.Modifiers.Add(LookDead);
		// About 140 degrees per second at full stick, frame-rate independent (Look multiplies by LookSensitivity 0.5).
		UInputModifierScalar* LookScale = NewObject<UInputModifierScalar>(this);
		// Y is negated: stick up looks up (the mouse axis has the opposite sign, Look() is written for the mouse).
		LookScale->Scalar = FVector(280.0, -200.0, 1.0);
		PadLook.Modifiers.Add(LookScale);
		PadLook.Modifiers.Add(NewObject<UInputModifierScaleByDeltaTime>(this));

		Context->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
		Context->MapKey(ConsoleAction, EKeys::Gamepad_FaceButton_Top);
		Context->MapKey(OpsAction, EKeys::Gamepad_Special_Left);
		Context->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
	}
}

void ATDAstronautCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateInput();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Input->AddMappingContext(Context, 10); // above the menu context (Space/Enter/Esc)
		}
	}
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATDAstronautCharacter::Move);
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATDAstronautCharacter::Look);
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		Input->BindAction(ConsoleAction, ETriggerEvent::Started, this, &ATDAstronautCharacter::ToggleConsole);
		Input->BindAction(OpsAction, ETriggerEvent::Started, this, &ATDAstronautCharacter::ToggleOps);
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ATDAstronautCharacter::Interact);
		Input->BindAction(InteractAction, ETriggerEvent::Completed, this, &ATDAstronautCharacter::InteractReleased);
		Input->BindAction(InteractAction, ETriggerEvent::Canceled, this, &ATDAstronautCharacter::InteractReleased);
	}
}

void ATDAstronautCharacter::Move(const FInputActionValue& Value)
{
	if (bConsoleOpen || !Controller || bWorking)
	{
		return; // feet planted while the tool works
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ATDAstronautCharacter::Look(const FInputActionValue& Value)
{
	if (bConsoleOpen)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X * LookSensitivity);
	AddControllerPitchInput(-Axis.Y * LookSensitivity);
}

void ATDAstronautCharacter::ToggleConsole()
{
	if (const ATDPlayerController* DesignPC = Cast<ATDPlayerController>(GetController()); DesignPC && DesignPC->IsDesignScreenOpen())
	{
		return; // the Mission Design Room owns the mouse
	}
	// The decisions live at the Mission Control console (research note §4.5: decisions between work at a station
	// you walk to). TAB / Y opens it when you stand there, else the helmet shows the way.
	UseStation();
}

void ATDAstronautCharacter::UseStation()
{
	ATDPlayerController* PC = Cast<ATDPlayerController>(GetController());
	if (!PC || PC->IsDesignScreenOpen())
	{
		return;
	}
	if (PC->IsBaseComputerOpen())
	{
		PC->ToggleBaseComputer();
		return;
	}
	if (IsNearStation())
	{
		PC->ToggleBaseComputer();
		return;
	}
	StationPingTime = FPlatformTime::Seconds();
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>())
	{
		Mission->PlaySfx(TEXT("UI_Locked"));
	}
}

bool ATDAstronautCharacter::IsNearStation() const
{
	const ATDMissionControlStation* Station = ATDMissionControlStation::Find(GetWorld());
	if (!Station)
	{
		return false;
	}
	return FVector::Dist2D(GetActorLocation(), Station->GetActorLocation()) < 300.f;
}

void ATDAstronautCharacter::SetConsoleOpen(bool bOpen)
{
	bConsoleOpen = bOpen;
	SyncHud();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	if (const ATDPlayerController* DesignPC = Cast<ATDPlayerController>(PC); DesignPC && DesignPC->IsDesignScreenOpen())
	{
		return; // keep the design room's UI-only input
	}
	if (bOpen)
	{
		// Mission console: cursor on, the decision HUD takes the clicks.
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		// Walking: mouse looks around, cursor hidden.
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void ATDAstronautCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bGravityApplied)
	{
		ApplyMarsGravity();
	}
	if (!bHudSynced)
	{
		SyncHud();
	}
	UpdateFocus();
	TickWork(DeltaSeconds);
}

void ATDAstronautCharacter::UpdateFocus()
{
	FocusText = FText::GetEmpty();
	if (bConsoleOpen)
	{
		return;
	}
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * InspectDistance;
	FCollisionQueryParams Params(TEXT("TDInspect"), false, this);
	TArray<FHitResult> Hits;
	if (!GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(30.f), Params))
	{
		return;
	}
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	for (const FHitResult& Hit : Hits)
	{
		const AActor* Actor = Hit.GetActor();
		if (!Actor || !Mission)
		{
			continue;
		}
		if (const ATDCrewMember* Crew = Cast<ATDCrewMember>(Actor))
		{
			FocusText = Mission->GetText(FName(*FString::Printf(TEXT("Crew_%s"), *StaticEnum<ETDSystem>()->GetNameStringByValue(static_cast<int64>(Crew->System)))));
			return;
		}
		for (const FName& Tag : Actor->Tags)
		{
			const FString TagString = Tag.ToString();
			if (TagString.StartsWith(TEXT("Inspect_")))
			{
				FocusText = Mission->GetText(FName(*TagString));
				return;
			}
		}
	}
}

// Base operations: the base computer and your own tasks

void ATDAstronautCharacter::ToggleOps()
{
	UseStation();
}

FName ATDAstronautCharacter::NearbyOwnTask() const
{
	const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	if (!Mission || !Mission->HasOps() || Mission->IsRunningPhase() || Mission->GetOps().Crew.Num() == 0)
	{
		return NAME_None;
	}
	const FVector Here = GetActorLocation();
	for (const FName& Id : Mission->GetOps().Crew[0].Tasks)
	{
		const FTDTaskRow* Row = Mission->GetTasksTable()->FindRow<FTDTaskRow>(Id, TEXT("TDAstronaut"), false);
		if (!Row)
		{
			continue;
		}
		const FVector Site = Mission->GetSitePosition(Row->Site);
		if (FVector2D::Distance(FVector2D(Here.X, Here.Y), FVector2D(Site.X, Site.Y)) < 450.f)
		{
			return Id;
		}
	}
	return NAME_None;
}

FText ATDAstronautCharacter::GetInteractPrompt() const
{
	const FName Id = NearbyOwnTask();
	const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	if (Id.IsNone() || !Mission)
	{
		return FText::GetEmpty();
	}
	const FTDTaskRow* Row = Mission->GetTasksTable()->FindRow<FTDTaskRow>(Id, TEXT("TDAstronaut"), false);
	return FText::Format(Mission->GetText(TEXT("Ops_Prompt")), Row ? Mission->GetText(Row->NameKey) : FText::FromName(Id));
}

void ATDAstronautCharacter::Interact()
{
	bInteractHeld = true;
	if (!SpotInReach() && IsNearStation())
	{
		bInteractHeld = false;
		UseStation();
	}
}

void ATDAstronautCharacter::InteractReleased()
{
	bInteractHeld = false;
}

// Hands-on work ------------------------------------------------------------------------------------------------

void ATDAstronautCharacter::SyncWorkSets()
{
	const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	TArray<FName> Own;
	if (Mission && Mission->HasOps() && Mission->GetOps().Crew.Num() > 0 && Mission->IsMissionRunning())
	{
		Own = Mission->GetOps().Crew[0].Tasks;
	}
	const double Now = FPlatformTime::Seconds();
	for (int32 Index = WorkSets.Num() - 1; Index >= 0; --Index)
	{
		FTDWorkSet& Set = WorkSets[Index];
		if (Own.Contains(Set.Task))
		{
			continue;
		}
		// Done (or unplanned): finished spots stay a few seconds so the result is seen, unfinished go at once.
		bool bAllDone = Set.Spots.Num() > 0;
		for (const TWeakObjectPtr<ATDWorkSpot>& Spot : Set.Spots)
		{
			bAllDone &= Spot.IsValid() && Spot->IsDone();
		}
		if (bAllDone && Set.FinishedAt < 0.0)
		{
			Set.FinishedAt = Now;
		}
		if (!bAllDone || Now - Set.FinishedAt > 8.0)
		{
			for (const TWeakObjectPtr<ATDWorkSpot>& Spot : Set.Spots)
			{
				if (Spot.IsValid())
				{
					SpotRefs.Remove(Spot.Get());
					Spot->Destroy();
				}
			}
			WorkSets.RemoveAt(Index);
		}
	}
	for (const FName& Task : Own)
	{
		if (!WorkSets.ContainsByPredicate([&Task](const FTDWorkSet& Set) { return Set.Task == Task; }))
		{
			SpawnWorkSet(Task);
		}
	}
}

void ATDAstronautCharacter::SpawnWorkSet(FName Task)
{
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const FTDTaskRow* Row = Mission && Mission->GetTasksTable() ? Mission->GetTasksTable()->FindRow<FTDTaskRow>(Task, TEXT("TDAstronaut"), false) : nullptr;
	const FTDSiteRow* Site = Row && Mission->GetSitesTable() ? Mission->GetSitesTable()->FindRow<FTDSiteRow>(Row->Site, TEXT("TDAstronaut"), false) : nullptr;
	FTDWorkSet& Set = WorkSets.AddDefaulted_GetRef();
	Set.Task = Task;
	if (!Row || !Site)
	{
		return;
	}
	const FTDHandsOnSpec Spec = TDHandsOn::SpecFor(Task);
	const FVector2D S(Site->X, Site->Y);
	FVector2D D = FVector2D(Site->LookX, Site->LookY) - S;
	D = D.IsNearlyZero() ? FVector2D(1.f, 0.f) : D.GetSafeNormal();
	const FVector2D Side(-D.Y, D.X);
	const bool bField = Spec.Verb == ETDWorkVerb::Hammer || Spec.Verb == ETDWorkVerb::Drill || Spec.Verb == ETDWorkVerb::Scoop || Spec.Verb == ETDWorkVerb::Brush;
	TArray<FVector2D> Points;
	if (Spec.Verb == ETDWorkVerb::Hammer && Spec.Spots == 4)
	{
		// Survey square: four corner stakes.
		const FVector2D C = S + D * 220.f;
		for (const FVector2D& Corner : { FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1) })
		{
			Points.Add(C + D * (Corner.X * 170.f) + Side * (Corner.Y * 170.f));
		}
	}
	else
	{
		const FVector2D C = S + D * (Site->bInside ? 20.f : 120.f);
		const float Spacing = bField ? 210.f : 130.f;
		for (int32 Index = 0; Index < Spec.Spots; ++Index)
		{
			Points.Add(C + Side * ((Index - (Spec.Spots - 1) * 0.5f) * Spacing));
		}
	}
	FCollisionQueryParams Params(TEXT("TDWorkGround"), false, this);
	for (TActorIterator<ATDCrewMember> It(GetWorld()); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}
	// Spots face back toward the site, where the player comes from.
	const float FaceYaw = FMath::RadiansToDegrees(FMath::Atan2(-D.Y, -D.X));
	for (const FVector2D& Raw : Points)
	{
		// Pull the spot toward the site until it is clear of buildings, then put it on the ground.
		FVector2D P = Raw;
		float GroundZ = GetActorLocation().Z - 92.f;
		for (int32 Step = 0; Step < 6; ++Step)
		{
			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(P.X, P.Y, 50000.f), FVector(P.X, P.Y, -50000.f), ECC_Visibility, Params))
			{
				GroundZ = Hit.ImpactPoint.Z;
			}
			const bool bBlocked = GetWorld()->OverlapAnyTestByChannel(FVector(P.X, P.Y, GroundZ + 90.f), FQuat::Identity, ECC_WorldStatic,
				FCollisionShape::MakeSphere(45.f), Params);
			if (!bBlocked)
			{
				break;
			}
			P += (S - P).GetSafeNormal() * 45.f - D * 30.f;
		}
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ATDWorkSpot* Spot = GetWorld()->SpawnActor<ATDWorkSpot>(FVector(P.X, P.Y, GroundZ), FRotator::ZeroRotator, SpawnParams))
		{
			Spot->Setup(Spec.Verb, Spec.Beats, FaceYaw);
			Spot->SetActive(true);
			Set.Spots.Add(Spot);
			SpotRefs.Add(Spot);
		}
	}
	UE_LOG(LogTouchdownMars, Display, TEXT("Work: %d spots for %s (%s)"), Set.Spots.Num(), *Task.ToString(), TDHandsOn::VerbKey(Spec.Verb));
}

ATDWorkSpot* ATDAstronautCharacter::SpotInReach() const
{
	const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	if (!Mission || Mission->IsRunningPhase() || bConsoleOpen)
	{
		return nullptr;
	}
	const FVector Here = GetActorLocation();
	const FVector View = Camera->GetForwardVector().GetSafeNormal2D();
	ATDWorkSpot* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (const FTDWorkSet& Set : WorkSets)
	{
		if (Set.FinishedAt >= 0.0)
		{
			continue;
		}
		for (const TWeakObjectPtr<ATDWorkSpot>& Spot : Set.Spots)
		{
			if (!Spot.IsValid() || Spot->IsDone())
			{
				continue;
			}
			const FVector To = Spot->GetActorLocation() - Here;
			const float Dist = To.Size2D();
			const float Facing = FVector::DotProduct(To.GetSafeNormal2D(), View);
			if (Dist > 190.f || (Dist > 70.f && Facing < 0.35f))
			{
				continue; // in arm's reach and roughly in front
			}
			const float Score = Dist - Facing * 60.f;
			if (Score < BestScore)
			{
				BestScore = Score;
				Best = Spot.Get();
			}
		}
	}
	return Best;
}

bool ATDAstronautCharacter::GetTaskWork(FName Task, int32& OutDoneSpots, int32& OutSpots, FVector& OutNext) const
{
	const FTDWorkSet* Set = WorkSets.FindByPredicate([&Task](const FTDWorkSet& S) { return S.Task == Task; });
	if (!Set || Set->Spots.Num() == 0)
	{
		return false;
	}
	OutDoneSpots = 0;
	OutSpots = Set->Spots.Num();
	float Best = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const TWeakObjectPtr<ATDWorkSpot>& Spot : Set->Spots)
	{
		if (!Spot.IsValid())
		{
			continue;
		}
		if (Spot->IsDone())
		{
			++OutDoneSpots;
			continue;
		}
		const float Dist = FVector::Dist2D(Spot->GetActorLocation(), GetActorLocation());
		if (Dist < Best)
		{
			Best = Dist;
			OutNext = Spot->GetActorLocation();
			bFound = true;
		}
	}
	return bFound;
}

void ATDAstronautCharacter::BuildTool(int32 VerbIndex)
{
	if (ToolVerb == VerbIndex || !ToolA || !ToolB)
	{
		return;
	}
	ToolVerb = VerbIndex;
	auto ShapeMesh = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name)); };
	auto Mat = [](const TCHAR* Name) { return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/TouchdownMars/Materials/%s.%s"), Name, Name)); };
	UStaticMesh* Cube = ShapeMesh(TEXT("Cube"));
	UStaticMesh* Cyl = ShapeMesh(TEXT("Cylinder"));
	UMaterialInterface* Dark = Mat(TEXT("MI_TD_Base_Dark"));
	UMaterialInterface* Rib = Mat(TEXT("MI_TD_Base_Rib"));
	UMaterialInterface* Accent = Mat(TEXT("MI_TD_Base_Accent"));
	UMaterialInterface* Hull = Mat(TEXT("MI_TD_Base_Hull"));
	auto Set = [](UStaticMeshComponent* C, UStaticMesh* M, const FVector& Loc, const FVector& Scale, const FRotator& Rot, UMaterialInterface* Mt)
	{
		C->SetStaticMesh(M);
		C->SetRelativeLocation(Loc);
		C->SetRelativeScale3D(Scale);
		C->SetRelativeRotation(Rot);
		C->SetMaterial(0, Mt);
	};
	switch (static_cast<ETDWorkVerb>(VerbIndex))
	{
	case ETDWorkVerb::Hammer: // sledge: handle up from the grip, head on top
		Set(ToolA, Cyl, FVector(0.f, 0.f, 16.f), FVector(0.035f, 0.035f, 0.42f), FRotator::ZeroRotator, Dark);
		Set(ToolB, Cube, FVector(0.f, 0.f, 36.f), FVector(0.16f, 0.07f, 0.07f), FRotator::ZeroRotator, Rib);
		break;
	case ETDWorkVerb::Plug: // connector with its cable
		Set(ToolA, Cyl, FVector(8.f, 0.f, 0.f), FVector(0.09f, 0.09f, 0.2f), FRotator(90.f, 0.f, 0.f), Accent);
		Set(ToolB, Cyl, FVector(-14.f, 0.f, -4.f), FVector(0.03f, 0.03f, 0.3f), FRotator(70.f, 0.f, 0.f), Dark);
		break;
	case ETDWorkVerb::Wrench:
		Set(ToolA, Cube, FVector(8.f, 0.f, 0.f), FVector(0.28f, 0.035f, 0.025f), FRotator::ZeroRotator, Rib);
		Set(ToolB, Cube, FVector(23.f, 0.f, 0.f), FVector(0.05f, 0.09f, 0.035f), FRotator::ZeroRotator, Rib);
		break;
	case ETDWorkVerb::Brush:
		Set(ToolA, Cyl, FVector(10.f, 0.f, 0.f), FVector(0.03f, 0.03f, 0.35f), FRotator(80.f, 0.f, 0.f), Dark);
		Set(ToolB, Cube, FVector(27.f, 0.f, -3.f), FVector(0.05f, 0.26f, 0.05f), FRotator::ZeroRotator, Hull);
		break;
	case ETDWorkVerb::Drill:
		Set(ToolA, Cube, FVector(0.f, 0.f, 0.f), FVector(0.16f, 0.08f, 0.13f), FRotator::ZeroRotator, Accent);
		Set(ToolB, Cyl, FVector(16.f, 0.f, 2.f), FVector(0.025f, 0.025f, 0.2f), FRotator(90.f, 0.f, 0.f), Rib);
		break;
	case ETDWorkVerb::Scoop:
		Set(ToolA, Cyl, FVector(6.f, 0.f, 8.f), FVector(0.03f, 0.03f, 0.4f), FRotator(60.f, 0.f, 0.f), Dark);
		Set(ToolB, Cube, FVector(24.f, 0.f, -4.f), FVector(0.18f, 0.16f, 0.015f), FRotator(-15.f, 0.f, 0.f), Rib);
		break;
	case ETDWorkVerb::Latch:
	case ETDWorkVerb::Scan:
	case ETDWorkVerb::Switch:
	default: // suit tablet / scanner
		Set(ToolA, Cube, FVector(4.f, 0.f, 0.f), FVector(0.14f, 0.1f, 0.03f), FRotator(-10.f, 0.f, 0.f), Dark);
		Set(ToolB, Cube, FVector(11.f, 0.f, 1.8f), FVector(0.005f, 0.07f, 0.012f), FRotator(-10.f, 0.f, 0.f), Accent);
		break;
	}
}

void ATDAstronautCharacter::LandBeat(ATDWorkSpot* Spot)
{
	UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
	const bool bSpotDone = Spot->Beat();
	Kick = 1.f;
	BeatPulse = 1.f;
	if (Mission)
	{
		// One distinct sound per verb; the hammer rings a little higher each hit (research note §6.1).
		const int32 Hit = Spot->GetBeatsDone();
		switch (Spot->GetVerb())
		{
		case ETDWorkVerb::Hammer: Mission->PlaySfxPitched(TEXT("Stamp"), 0.85f + 0.12f * Hit); break;
		case ETDWorkVerb::Plug: Mission->PlaySfxPitched(TEXT("CardFlip"), 0.7f + 0.2f * Hit); break;
		case ETDWorkVerb::Wrench: Mission->PlaySfxPitched(TEXT("UI_Hover"), 0.6f + 0.1f * Hit); break;
		case ETDWorkVerb::Latch: Mission->PlaySfxPitched(TEXT("CardFlip"), 1.2f); break;
		case ETDWorkVerb::Brush: Mission->PlaySfxPitched(TEXT("UI_Hover"), 1.5f, 0.6f); break;
		case ETDWorkVerb::Drill: Mission->PlaySfxPitched(TEXT("Stamp"), 0.55f + 0.05f * Hit); break;
		case ETDWorkVerb::Scan: Mission->PlaySfxPitched(TEXT("UI_Hover"), 1.8f); break;
		case ETDWorkVerb::Scoop: Mission->PlaySfxPitched(TEXT("Stamp"), 0.7f); break;
		default: Mission->PlaySfxPitched(TEXT("UI_Confirm"), 1.3f); break;
		}
		if (bSpotDone)
		{
			Mission->PlaySfxPitched(TEXT("UI_Confirm"), 1.f);
		}
	}
	if (!bSpotDone || !Mission)
	{
		return;
	}
	// Last spot of the task: the task is done (effects, report line, scene parts).
	for (FTDWorkSet& Set : WorkSets)
	{
		if (!Set.Spots.Contains(Spot))
		{
			continue;
		}
		const bool bAll = !Set.Spots.ContainsByPredicate([](const TWeakObjectPtr<ATDWorkSpot>& S) { return S.IsValid() && !S->IsDone(); });
		if (bAll && Mission->CompleteOwnTask(Set.Task))
		{
			Set.FinishedAt = FPlatformTime::Seconds();
			Mission->LogOnboarding(TEXT("HandsOnDone"), Set.Task.ToString());
			// The nearest crew member acknowledges (bark), half a beat later in spirit.
			ATDCrewMember* Nearest = nullptr;
			float Best = 3000.f;
			for (TActorIterator<ATDCrewMember> It(GetWorld()); It; ++It)
			{
				const float Dist = FVector::Dist(It->GetActorLocation(), GetActorLocation());
				if (Dist < Best && !It->IsHidden())
				{
					Best = Dist;
					Nearest = *It;
				}
			}
			if (Nearest)
			{
				Nearest->Say(Mission->GetText(TEXT("Bark_WorkDone")), 3.5f);
			}
		}
		break;
	}
}

void ATDAstronautCharacter::TickWork(float DeltaSeconds)
{
	SyncTimer -= DeltaSeconds;
	if (SyncTimer <= 0.f)
	{
		SyncTimer = 0.4f;
		SyncWorkSets();
	}
	if (ATDMissionControlStation* Station = ATDMissionControlStation::Find(GetWorld()))
	{
		const UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>();
		const bool bNeeds = Mission && Mission->IsMissionRunning() && !Mission->IsRunningPhase()
			&& (!Mission->CanEndPhase() || (!Mission->GetState().CurrentEventId.IsNone() && Mission->GetState().EventChoice < 0));
		Station->SetNeedsYou(bNeeds);
	}

	ATDWorkSpot* Spot = SpotInReach();
	bWorking = Spot && bInteractHeld;
	if (bWorking)
	{
		const FTDWorkSet* Set = WorkSets.FindByPredicate([Spot](const FTDWorkSet& S) { return S.Spots.Contains(Spot); });
		const float BeatSeconds = Set ? TDHandsOn::SpecFor(Set->Task).BeatSeconds : 0.5f;
		WorkBeatSeconds = BeatSeconds;
		WorkAccum += DeltaSeconds;
		if (WorkAccum >= BeatSeconds)
		{
			WorkAccum -= BeatSeconds;
			LandBeat(Spot);
			if (Spot->IsDone())
			{
				WorkAccum = 0.f;
				bInteractHeld = false; // one spot per hold: walk to the next
				bWorking = false;
			}
		}
	}
	else
	{
		WorkAccum = FMath::Max(0.f, WorkAccum - DeltaSeconds * 0.5f);
	}
	if (Spot)
	{
		BuildTool(static_cast<int32>(Spot->GetVerb()));
	}
	PoseTool(DeltaSeconds, Spot != nullptr);
}

void ATDAstronautCharacter::PoseTool(float DeltaSeconds, bool bNear)
{
	ToolOut = FMath::FInterpTo(ToolOut, bNear ? 1.f : 0.f, DeltaSeconds, 8.f);
	const bool bVisible = ToolOut > 0.02f && ToolVerb >= 0;
	if (ToolA) { ToolA->SetVisibility(bVisible); }
	if (ToolB) { ToolB->SetVisibility(bVisible); }
	Kick = FMath::Max(0.f, Kick - DeltaSeconds * 7.f);
	BeatPulse = FMath::Max(0.f, BeatPulse - DeltaSeconds * 3.f);
	// Camera kick on every hit (Nijman: small, fast, decays).
	Camera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeight - 2.5f * Kick));
	Camera->SetFieldOfView(FieldOfView - 1.5f * Kick);
	if (!ToolRoot || !bVisible)
	{
		return;
	}
	const float T = bWorking ? FMath::Clamp(WorkAccum / FMath::Max(0.05f, WorkBeatSeconds), 0.f, 1.f) : 0.f;
	const float Idle = FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f) * 0.6f;
	FVector Loc(48.f, 26.f, -30.f - 40.f * (1.f - ToolOut) + Idle);
	FRotator Rot = FRotator::ZeroRotator;
	switch (static_cast<ETDWorkVerb>(ToolVerb))
	{
	case ETDWorkVerb::Hammer:
		// Wind up (top back), then strike forward; the hit lands at the end of the beat.
		Rot.Pitch = bWorking ? (T < 0.7f ? FMath::Lerp(0.f, 65.f, T / 0.7f) : FMath::Lerp(65.f, -40.f, (T - 0.7f) / 0.3f)) : 10.f;
		Rot.Pitch -= 30.f * Kick;
		break;
	case ETDWorkVerb::Plug:
	case ETDWorkVerb::Latch:
	case ETDWorkVerb::Switch:
	case ETDWorkVerb::Scan:
		Loc.X += bWorking ? 16.f * FMath::Sin(T * PI) : 0.f;
		Rot.Roll = static_cast<ETDWorkVerb>(ToolVerb) == ETDWorkVerb::Plug ? 90.f * T : 0.f;
		break;
	case ETDWorkVerb::Wrench:
		Rot.Roll = bWorking ? -55.f * FMath::Sin(T * PI) : 0.f;
		break;
	case ETDWorkVerb::Brush:
		Loc.Y += bWorking ? 16.f * FMath::Sin(T * 2.f * PI) : 0.f;
		Rot.Yaw = bWorking ? 12.f * FMath::Cos(T * 2.f * PI) : 0.f;
		break;
	case ETDWorkVerb::Drill:
		Loc.X += bWorking ? 8.f : 0.f;
		if (bWorking)
		{
			Loc += FVector(FMath::FRandRange(-0.6f, 0.6f), FMath::FRandRange(-0.6f, 0.6f), FMath::FRandRange(-0.6f, 0.6f));
			ToolB->AddRelativeRotation(FRotator(0.f, 0.f, 1800.f * DeltaSeconds));
		}
		break;
	case ETDWorkVerb::Scoop:
		Rot.Pitch = bWorking ? (T < 0.5f ? FMath::Lerp(0.f, -35.f, T * 2.f) : FMath::Lerp(-35.f, 25.f, (T - 0.5f) * 2.f)) : 0.f;
		break;
	default:
		break;
	}
	ToolRoot->SetRelativeLocation(Loc);
	ToolRoot->SetRelativeRotation(Rot);
}

FTDWorkUi ATDAstronautCharacter::GetWorkUi() const
{
	FTDWorkUi Ui;
	const UTDMissionSubsystem* Mission = GetWorld() ? GetWorld()->GetSubsystem<UTDMissionSubsystem>() : nullptr;
	if (!Mission || bConsoleOpen)
	{
		return Ui;
	}
	if (const ATDWorkSpot* Spot = SpotInReach())
	{
		int32 Done = 0;
		int32 Count = 0;
		float BeatSeconds = 0.5f;
		for (const FTDWorkSet& Set : WorkSets)
		{
			if (Set.Spots.Contains(Spot))
			{
				Count = Set.Spots.Num();
				BeatSeconds = TDHandsOn::SpecFor(Set.Task).BeatSeconds;
				for (const TWeakObjectPtr<ATDWorkSpot>& S : Set.Spots)
				{
					Done += S.IsValid() && S->IsDone() ? 1 : 0;
				}
			}
		}
		Ui.bShow = true;
		Ui.bHolding = bWorking;
		Ui.Prompt = FText::Format(Mission->GetText(TEXT("Work_Prompt")),
			FText::Format(Mission->GetText(TDHandsOn::VerbKey(Spot->GetVerb())), Done + 1, Count));
		// The ring moves in beat steps plus the part of the beat being worked.
		Ui.Ring = FMath::Clamp((Spot->GetBeatsDone() + (bWorking ? WorkAccum / BeatSeconds : 0.f)) / FMath::Max(1, Spot->GetBeats()), 0.f, 1.f);
		Ui.BeatPulse = BeatPulse;
		return Ui;
	}
	if (IsNearStation() && Mission->IsMissionRunning() && !Mission->IsRunningPhase())
	{
		Ui.bShow = true;
		Ui.bStation = true;
		Ui.Prompt = Mission->GetText(TEXT("MC_Use"));
	}
	return Ui;
}

void ATDAstronautCharacter::DevGoToNextSpot(FName Task)
{
	SyncWorkSets();
	int32 Done = 0;
	int32 Spots = 0;
	FVector Next;
	if (!GetTaskWork(Task, Done, Spots, Next))
	{
		UE_LOG(LogTouchdownMars, Warning, TEXT("WorkTest: no open spot for %s"), *Task.ToString());
		return;
	}
	const FTDWorkSet* Set = WorkSets.FindByPredicate([&Task](const FTDWorkSet& S) { return S.Task == Task; });
	FVector Facing = FVector::ForwardVector;
	for (const TWeakObjectPtr<ATDWorkSpot>& Spot : Set->Spots)
	{
		if (Spot.IsValid() && !Spot->IsDone())
		{
			Facing = Spot->GetActorForwardVector();
			Next = Spot->GetActorLocation();
			break;
		}
	}
	const FVector Stand = Next + Facing * 120.f + FVector(0.f, 0.f, 100.f);
	SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
	if (Controller)
	{
		const FVector Look = (Next + FVector(0.f, 0.f, 40.f)) - (Stand + FVector(0.f, 0.f, EyeHeight));
		Controller->SetControlRotation(Look.Rotation());
	}
	UE_LOG(LogTouchdownMars, Display, TEXT("WorkTest: at %s spot %d/%d"), *Task.ToString(), Done + 1, Spots);
}

void ATDAstronautCharacter::DevGoToStation()
{
	if (const ATDMissionControlStation* Station = ATDMissionControlStation::Find(GetWorld()))
	{
		const FVector Stand = Station->GetUsePoint() + FVector(0.f, 0.f, 100.f);
		SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
		if (Controller)
		{
			Controller->SetControlRotation((Station->GetActorLocation() + FVector(0.f, 0.f, 140.f) - Stand - FVector(0.f, 0.f, EyeHeight)).Rotation());
		}
	}
}
