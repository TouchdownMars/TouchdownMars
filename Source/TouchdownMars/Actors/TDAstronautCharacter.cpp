// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

#include "Actors/TDAstronautCharacter.h"
#include "Model/TDOperations.h"

#include "Actors/TDPlayerController.h"
#include "Actors/TDSceneActors.h"
#include "UI/TDWidgets.h"
#include "Actors/TDSystemModule.h"
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
	}
}

void ATDAstronautCharacter::Move(const FInputActionValue& Value)
{
	if (bConsoleOpen || !Controller)
	{
		return;
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
	SetConsoleOpen(!bConsoleOpen);
	UE_LOG(LogTouchdownMars, Display, TEXT("Astronaut: mission console %s"), bConsoleOpen ? TEXT("open") : TEXT("closed"));
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
	if (ATDPlayerController* PC = Cast<ATDPlayerController>(GetController()))
	{
		PC->ToggleBaseComputer();
	}
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
	const FName Id = NearbyOwnTask();
	if (UTDMissionSubsystem* Mission = GetWorld()->GetSubsystem<UTDMissionSubsystem>(); Mission && !Id.IsNone())
	{
		Mission->CompleteOwnTask(Id);
	}
}
