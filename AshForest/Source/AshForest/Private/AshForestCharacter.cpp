// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AshForestCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"
#include "AshForestCheckpoint.h"
#include "AshForestProjectile.h"
#include "FocusPointTrigger.h"

//////////////////////////////////////////////////////////////////////////
// AAshForestCharacter

AAshForestCharacter::AAshForestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.SetTickFunctionEnable(true);

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 65.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.5f;

	DefaultCameraSocketOffset.Set(0.f, 0.f, 90.f);
	CameraSocketVelocityOffset_MAX.Set(0.f, 0.f, 110.f);
	LockedOnCameraSocketOffset.Set(50.f, 250.f, 100.f);

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SocketOffset = DefaultCameraSocketOffset;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	DashCooldownTime_Normal = .02f;
	DashCooldownTime_AfterWallJump= 1.f;
	DashSpeed = 9000.f;
	DashDuration_MAX = .3f;
	DashCharges_MAX = 6;
	DashChargeReloadInterval = 1.f;
	AllowedDashesWhileFalling = 1;
	DashDistance_MAX = 750.f;
	DashDamage = 50.f;

	ClimbingSpeed_Start = 1200.f;
	ClimbingSpeed_DecayRate = 800.f;
	ClimbingDuration_MAX = 1.f;
	ClimbingJumpImpulseAxisSizes.Set(1000.f, 2000.f);

	WallRunSpeed_Start = 7000.f;
	WallRunSpeed_DecayRate = 500.f;
	WallRunDuration_MAX = 1.5f;
	WallRunJumpVelocityZ = 2000.f;
	WallRunPastLookDirAngleToSurface = 25.f;

	GrabLedgeCheckInterval = .05;

	LockOnFindTarget_Radius = 1500.f;
	LockOnFindTarget_WithinLookDirAngleDelta = 44.5f;
	LockOnInterpViewToTargetSpeed = 5.f;
	LockedOnInterpCameraSocketOffsetSpeed_IN = 2.5f;
	LockedOnInterpCameraSocketOffsetSpeed_OUT = 4.f;
	AllowSwitchLockOnTargetInterval = .5f;

	CameraArmLength_MAX = 500.f;
	CameraArmLengthInterpSpeeds.Set(5.f, 2.f);

	WarpFOV_MAX = 110.f;
	WarpFOV_InterpSpeed = 4.f;

	MeshInterpSpeed_Location = 8.f;
	MeshInterpSpeed_Rotation = 5.f;

	HealthRestoreRate = 2.f;
	MaxHealth = 100.f;

	LatestCheckpointIndex = -1;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AAshForestCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AAshForestCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//AS: Custom Actions
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AAshForestCharacter::OnDashPressed);
	PlayerInputComponent->BindAction("Dash", IE_Released, this, &AAshForestCharacter::OnDashReleased);
	PlayerInputComponent->BindAction("LockOn", IE_Pressed, this, &AAshForestCharacter::OnLockOnPressed);
	PlayerInputComponent->BindAction("LockOn", IE_Released, this, &AAshForestCharacter::OnLockOnReleased);
	PlayerInputComponent->BindAction("SwitchTarget_Left", IE_Pressed, this, &AAshForestCharacter::SwitchLockOnTarget_Left);
	PlayerInputComponent->BindAction("SwitchTarget_Right", IE_Pressed, this, &AAshForestCharacter::SwitchLockOnTarget_Right);
	PlayerInputComponent->BindAction("Focus", IE_Pressed, this, &AAshForestCharacter::StartFocusing);
	PlayerInputComponent->BindAction("Focus", IE_Released, this, &AAshForestCharacter::StopFocusing);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAshForestCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAshForestCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AAshForestCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AAshForestCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AAshForestCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AAshForestCharacter::LookUpAtRate);

	//AS: Custom Axes
	PlayerInputComponent->BindAxis("SwitchTarget", this, &AAshForestCharacter::OnMouseWheelScroll);
}

void AAshForestCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	if (!bIsFocusing)
	{
		if (!IsLockedOn())
			AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
		else if (FMath::Abs(Rate) >= .75f && IsLockedOn())
			TrySwitchLockOnTarget(Rate);
	}
}

void AAshForestCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	if(!IsLockedOn() && !bIsFocusing)
		AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AAshForestCharacter::AddControllerPitchInput(float Val)
{
	if (!IsLockedOn() && !bIsFocusing)
		Super::AddControllerPitchInput(Val);
}

void AAshForestCharacter::AddControllerYawInput(float Val)
{
	if (!IsLockedOn() && !bIsFocusing)
		Super::AddControllerYawInput(Val);
}

void AAshForestCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && AshMoveState_Current == EAshCustomMoveState::EAshMove_NONE)
	{
		// find out which way is forward
		FRotator Rotation = GetControlRotation();

		if (LockOnTarget_Current != NULL)
			Rotation = (LockOnTarget_Current->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();

		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AAshForestCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && AshMoveState_Current == EAshCustomMoveState::EAshMove_NONE)
	{
		// find out which way is right
		FRotator Rotation = GetControlRotation();

		if (LockOnTarget_Current != NULL)
			Rotation = (LockOnTarget_Current->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();

		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AAshForestCharacter::OnMouseWheelScroll(float Rate)
{
	if (Rate != 0.f && IsLockedOn())
	{
		if (Rate > 0.f)
			SwitchLockOnTarget_Right();
		else
			SwitchLockOnTarget_Left();
	}
}

void AAshForestCharacter::BeginPlay() 
{
	DashCharges_Current = DashCharges_MAX;

	MyInitialMovementVars.InitialGravityScale = GetCharacterMovement()->GravityScale;
	MyInitialMovementVars.InitialGroundFriction = GetCharacterMovement()->GroundFriction;
	MyInitialMovementVars.InitialMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	MyInitialMovementVars.InitialFallingLateralFriction = .05f;//GetCharacterMovement()->FallingLateralFriction;
	MyInitialMovementVars.InitialAirControl = GetCharacterMovement()->AirControl;

	CameraArmLength_Default = CameraBoom->TargetArmLength;

	if (GetController())
	{
		MyCameraManager = ((APlayerController*)GetController())->PlayerCameraManager;

		check(MyCameraManager);

		MyCameraManager->UnlockFOV();
		StartFOV = MyCameraManager->DefaultFOV;
	}

	ResetMeshTransform();

	Super::BeginPlay();
}

void AAshForestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Tick_UpdateHealth(DeltaTime);

	if (bWantsToDash)
	{
		bWantsToDash = false;
		TryDash();
	}

	if (bWantsToLockOn)
	{
		bWantsToLockOn = false;
		TryLockOn();
	}

	if (WantsToSwitchLockOnTargetDir != 0)
	{
		WantsToSwitchLockOnTargetDir = FMath::Clamp(WantsToSwitchLockOnTargetDir, -1, 1);
		TrySwitchLockOnTarget((float)WantsToSwitchLockOnTargetDir);
		WantsToSwitchLockOnTargetDir = 0;
	}

	switch (AshMoveState_Current)
	{
	case EAshCustomMoveState::EAshMove_DASHING:
		Tick_Dash(DeltaTime);
		break;
	case EAshCustomMoveState::EAshMove_CLIMBING:
		Tick_Climbing(DeltaTime);
		break;
	default:
		if (WantsToGrabLedge())
			TryGrabLedge();
		break;
	}

	Tick_UpdateCamera(DeltaTime);

	if (IsLockedOn()) 
		Tick_LockedOn(DeltaTime);

	if (bIsMeshTransformInterpolating)
		Tick_MeshInterp(DeltaTime);

	//AS: Reload Dash Charges Over Time
	if (DashCharges_Current < DashCharges_MAX && AshMoveState_Current == EAshCustomMoveState::EAshMove_NONE /*&& ((UCharacterMovementComponent*)GetMovementComponent())->IsWalking()*/)
	{
		DashChargeReloadDurationCurrent -= DeltaTime;

		if (DashChargeReloadDurationCurrent <= 0.f)
		{
			DashCharges_Current++;
			DashChargeReloadDurationCurrent = DashChargeReloadInterval;

			OnDashRecharge();
		}
	}

	//AS: Restore Air Control after wall jumping
	if (GetCharacterMovement()->FallingLateralFriction == 0.f && GetWorld()->TimeSince(LastWallJumpTime) > DashCooldownTime_AfterWallJump)
	{
		GetCharacterMovement()->AirControl = MyInitialMovementVars.InitialAirControl;
		GetCharacterMovement()->FallingLateralFriction = MyInitialMovementVars.InitialFallingLateralFriction;
	}
}

void AAshForestCharacter::Jump()
{
	if (AshMoveState_Current == EAshCustomMoveState::EAshMove_CLIMBING)
		DoWallJump();
	else
		Super::Jump();
}

void AAshForestCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode /*= 0*/)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (GetCharacterMovement()->IsWalking())
	{
		DashesWhileFalling_Current = 0;
		GetCharacterMovement()->FallingLateralFriction = MyInitialMovementVars.InitialFallingLateralFriction;
		GetCharacterMovement()->bUseSeparateBrakingFriction = true;
	}
	else
		GetCharacterMovement()->bUseSeparateBrakingFriction = false;
}

void AAshForestCharacter::SetAshCustomMoveState(TEnumAsByte<EAshCustomMoveState::Type> NewMoveState)
{
	if (NewMoveState != AshMoveState_Current)
	{
		AshMoveState_Previous = AshMoveState_Current;
		AshMoveState_Current = NewMoveState;

		OnAshCustomMoveStateChanged();
	}
}

void AAshForestCharacter::OnAshCustomMoveStateChanged()
{
	if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple, FString::Printf(TEXT("New Move State [%i]"), (uint8)AshMoveState_Current));

	switch (AshMoveState_Current)
	{
	case EAshCustomMoveState::EAshMove_NONE:
		break;
	case EAshCustomMoveState::EAshMove_DASHING:
		break;
	case EAshCustomMoveState::EAshMove_CLIMBING:
		break;
	default:
		break;
	}
}

void AAshForestCharacter::AAshForestCharacter::OnDashPressed()
{
	bWantsToDash = true;
}

void AAshForestCharacter::AAshForestCharacter::OnDashReleased()
{
	bWantsToDash = false;
}

void AAshForestCharacter::TryDash()
{
	if (CanDash(true))
	{
		auto dir = GetLastMovementInputVector();

		//AS: Set the dash dir to the look vector if the player wasn't pressing any movement inputs
		if (dir == FVector::ZeroVector)
		{
			const FRotator rotation = LockOnTarget_Current != nullptr ? (LockOnTarget_Current->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation() : GetControlRotation();
			dir = FRotator(0, rotation.Yaw, 0).Vector();
		}

		if (dir != FVector::ZeroVector)
			StartDash(dir);
	}
}

bool AAshForestCharacter::CanDash(const bool bIsForStart /*= false*/) const
{
	return bIsForStart ? (!bIsFocusing && (GetWorld()->TimeSince(LastDashEndTime) > DashCooldownTime_Current) && AshMoveState_Current == EAshCustomMoveState::EAshMove_NONE && DashCharges_Current > 0 && (GetCharacterMovement()->IsWalking() || DashesWhileFalling_Current < AllowedDashesWhileFalling)) : IsDashing();
}

void AAshForestCharacter::OnDash_Implementation()
{

}

void AAshForestCharacter::OnDashRecharge_Implementation()
{

}

void AAshForestCharacter::StartDash(FVector & DashDir)
{
	if (!DashDir.IsNormalized())
		DashDir = DashDir.GetSafeNormal();

	SetAshCustomMoveState(EAshCustomMoveState::EAshMove_DASHING);

	OriginalDashStartLocation = GetActorLocation();
	OriginalDashDir = CurrentDashDir = DashDir;
	PrevDashLoc = GetActorLocation();
	DashDistance_Current = DashDistance_MAX;
	LastDashStartTime = GetWorld()->GetTimeSeconds();
	DashCooldownTime_Current = DashCooldownTime_Normal;

	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->MaxWalkSpeed = DashSpeed;
	GetCharacterMovement()->GroundFriction = 0.f;
	GetCharacterMovement()->FallingLateralFriction = 0.f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = false;

	DashDamagedActors.Empty();

	SetActorRotation(FRotator(0.f, CurrentDashDir.Rotation().Yaw, 0.f));

	auto newVel = CurrentDashDir * DashSpeed;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

	DashCharges_Current--;

	if (GetCharacterMovement()->IsFalling())
		DashesWhileFalling_Current++;

	OnDash();
}

void AAshForestCharacter::Tick_Dash(float DeltaTime)
{
	if (!CanDash())
		return;

	//AS: Check to see if we have gone past our max allowed dashing time
	if (GetWorld()->TimeSince(LastDashStartTime) > DashDuration_MAX)
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (REACHED MAX TIME)")));

		EndDash();  
		return;
	}

	DashDistance_Current -= (GetActorLocation() - PrevDashLoc).Size2D();

	//AS: Check to see if we have gone past our max allowed dashing distance
	if (DashDistance_Current <= 0.f ||  (GetActorLocation() - OriginalDashStartLocation).Size2D() > DashDistance_MAX)
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (REACHED MAX DISTANCE)")));

		EndDash();
		return;
	}

	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	capRadius += 15.f;

	if (LockOnTarget_Current != NULL && (GetActorLocation() - LockOnTarget_Current->GetComponentLocation()).SizeSquared2D() > FMath::Square(DashDistance_MAX * .5f))
	{
		auto dirFromTarget = (GetActorLocation() - LockOnTarget_Current->GetComponentLocation()).GetSafeNormal2D();

		if (FVector::DotProduct(-dirFromTarget, OriginalDashDir) <= 0.f)
		{
			const FVector prevDirFromTarget = (PrevDashLoc - LockOnTarget_Current->GetComponentLocation()).GetSafeNormal2D();
			float deltaAngle = FMath::Acos(FVector::DotProduct(dirFromTarget, prevDirFromTarget)) * (180.f / PI);

			if (FVector::CrossProduct(dirFromTarget, prevDirFromTarget).Z < 0.f)
				deltaAngle *= -1.f;

			OriginalDashDir = FRotator(0.f, deltaAngle, 0.f).UnrotateVector(OriginalDashDir);

			if (bDebugAshMovement)
				DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (OriginalDashDir * 100.f), FColor::Blue, false, 5.f, 0, 5.f);
		}
	}

	CurrentDashDir = OriginalDashDir;

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	const FVector traceStart = GetActorLocation();
	const FVector traceEnd = GetActorLocation() + (OriginalDashDir * DashDistance_Current);
	TArray<FHitResult> dashHits;
	const bool bFoundHit = GetWorld()->SweepMultiByChannel(dashHits, traceStart, traceEnd, GetActorRotation().Quaternion(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeCapsule(capRadius, capHalfHeight), params);
	
	if (bDebugAshMovement)
	{
		DrawDebugCapsule(GetWorld(), traceStart, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
		DrawDebugLine(GetWorld(), traceStart, traceEnd, bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
		DrawDebugCapsule(GetWorld(), traceEnd, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
	}
	
	if (bFoundHit)
	{
		for (auto dashHit : dashHits)
		{
			if (dashHit.Actor == NULL)
				continue;

			if (dashHit.Actor->IsA(AAshForestProjectile::StaticClass()))
			{
				if (!DashDamagedActors.Contains(dashHit.Actor.Get()))
				{
					DashDamagedActors.Add(dashHit.Actor.Get());
					DeflectProjectile(dashHit.Actor.Get());
				}
				
				continue;
			}

			if (!DashDamagedActors.Contains(dashHit.Actor.Get()) && dashHit.Actor->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass()))
			{
				if (ITargetableInterface::Execute_CanBeDamaged(dashHit.Actor.Get(), this, dashHit))
				{
					if (bDebugAshMovement)
						DrawDebugSphere(GetWorld(), dashHit.ImpactPoint, 75.f, 32, FColor::Yellow, false, 5.f, 0, 5.f);

					ITargetableInterface::Execute_TakeDamage(dashHit.Actor.Get(), this, DashDamage, dashHit);

					if (dashHit.Actor != NULL)
					{
						if(ITargetableInterface::Execute_IgnoresCollisionWithDamager(dashHit.Actor.Get(), this, dashHit))
							GetCapsuleComponent()->IgnoreActorWhenMoving(dashHit.Actor.Get(), true);
						
						DashDamagedActors.Add(dashHit.Actor.Get());
						
						if (auto hitChar = Cast<ADamageableCharacter>(dashHit.Actor.Get()))
						{
							auto dashImpulse = ((CurrentDashDir + FVector(0.f, 0.f, .25f)) * .5f) * 5000.f;
							hitChar->GetCharacterMovement()->AddImpulse(dashImpulse, true);
						}
					}
				}
			}
			else if ((dashHit.Location - GetActorLocation()).SizeSquared2D() <= FMath::Square(5.f))
			{
				//AS: Try to dash around smaller blockers or up ramps
				auto hitForwardDot = FVector::DotProduct(dashHit.ImpactNormal, OriginalDashDir);

				if (hitForwardDot < 0.f)
				{
					auto hitUpDot = FVector::DotProduct(dashHit.ImpactNormal, FVector::UpVector);

					if (hitUpDot >= .5f)
					{
						CurrentDashDir = FVector::VectorPlaneProject(OriginalDashDir, dashHit.ImpactNormal);

						if (bDebugAshMovement)
							DrawDebugLine(GetWorld(), dashHit.Location, dashHit.Location + (CurrentDashDir * 50.f), FColor::Magenta, false, 5.f, 0, 5.f);
					}
					else
					{
						EndDashWithHit(dashHit);

						if (bDebugAshMovement)
						{
							DrawDebugLine(GetWorld(), dashHit.ImpactPoint, dashHit.ImpactPoint + (dashHit.ImpactNormal * 50.f), FColor::Red, false, 5.f, 0, 8.f);
							if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (HIT WALL)")));
						}

						return;
					}

					if (bDebugAshMovement)
						DrawDebugLine(GetWorld(), dashHit.ImpactPoint, dashHit.ImpactPoint + (dashHit.ImpactNormal * 50.f), FColor::Yellow, false, 5.f, 0, 5.f);
				}
			}
		}
	}

	auto newVel = CurrentDashDir * DashSpeed;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

	PrevDashLoc = GetActorLocation();
}

void AAshForestCharacter::EndDash()
{
	if (CurrentClimbingNormal != FVector::ZeroVector)
		SetAshCustomMoveState(EAshCustomMoveState::EAshMove_CLIMBING);
	else
		SetAshCustomMoveState(EAshCustomMoveState::EAshMove_NONE);

	LastDashEndTime = GetWorld()->GetTimeSeconds();

	GetCharacterMovement()->GravityScale = MyInitialMovementVars.InitialGravityScale;
	GetCharacterMovement()->GroundFriction = MyInitialMovementVars.InitialGroundFriction;
	GetCharacterMovement()->MaxWalkSpeed = MyInitialMovementVars.InitialMaxWalkSpeed;
	GetCharacterMovement()->FallingLateralFriction = MyInitialMovementVars.InitialFallingLateralFriction;
	GetCharacterMovement()->AirControl = MyInitialMovementVars.InitialAirControl;

	//AS: Un-ignore actors we dashed through
	for (auto currActor : DashDamagedActors)
	{
		if (!currActor)
			continue;

		GetCapsuleComponent()->IgnoreActorWhenMoving(currActor, false);
	}

	//AS: Make sure that we can't go extra distance due to large tick delta times
	if ((GetActorLocation() - OriginalDashStartLocation).Size() > DashDistance_MAX)
		SetActorLocation(OriginalDashStartLocation + ((GetActorLocation() - OriginalDashStartLocation).GetSafeNormal() * DashDistance_MAX));

	auto newVel = GetVelocity().GetSafeNormal2D() * (GetCharacterMovement()->MaxWalkSpeed * 2.f);
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);
}

void AAshForestCharacter::EndDashWithHit(const FHitResult & EndHit)
{
	if (!IsDashing())
		return;

	EndDash();

	if (EndHit.ImpactNormal != FVector::ZeroVector)
	{
		if (CanClimbHitSurface(true, EndHit))
			StartClimbing(EndHit);
	}
}

bool AAshForestCharacter::CanClimbHitSurface(const bool & bIsForStart, const FHitResult & SurfaceHit) const
{
	if (IsLockedOn() || SurfaceHit.Component == NULL || !SurfaceHit.Component->CanBeClimbed())
		return false;

	if (bIsForStart)
		return FVector::DotProduct((SurfaceHit.ImpactPoint - GetActorLocation()).GetSafeNormal2D(), GetControlRotation().Vector()) > .15f;

	return true;
}

PRAGMA_DISABLE_OPTIMIZATION
void AAshForestCharacter::StartClimbing(const FHitResult & ClimbingSurfaceHit)
{
	SetAshCustomMoveState(EAshCustomMoveState::EAshMove_CLIMBING);

	LastStartClimbingTime = GetWorld()->GetTimeSeconds();
	CurrentClimbingNormal = ClimbingSurfaceHit.ImpactNormal;
	CurrentDirToClimbingSurface = (ClimbingSurfaceHit.ImpactPoint - GetActorLocation()).GetSafeNormal();
	PrevClimbingLocation = FVector::ZeroVector;
	bIsWallRunning = false;
	bDidWallJump = false;

	const FVector currPlayerVelDir = FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector();
	const FVector currSurfaceNormal = CurrentClimbingNormal.GetSafeNormal2D();
	const float angleBetween = FMath::Abs(FMath::Acos((-currSurfaceNormal | currPlayerVelDir)) * (180.f / PI));

	if (angleBetween >= WallRunPastLookDirAngleToSurface)
	{
		CurrentClimbingDir = FVector::VectorPlaneProject(currPlayerVelDir, ClimbingSurfaceHit.ImpactNormal);
		CurrentClimbingDir.Z = 0.f;
		CurrentClimbingDir = CurrentClimbingDir.GetSafeNormal();

		bIsWallRunning = true;
	}
	else
		CurrentClimbingDir = FVector::VectorPlaneProject(FVector::UpVector, ClimbingSurfaceHit.ImpactNormal).GetSafeNormal();
	
	ClimbingSpeed_Current = bIsWallRunning ? WallRunSpeed_Start : ClimbingSpeed_Start;

	((UCharacterMovementComponent*)GetMovementComponent())->SetMovementMode(MOVE_Falling);

	auto newVel = CurrentClimbingDir * ClimbingSpeed_Current;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

	//DashesWhileFalling_Current = AllowedDashesWhileFalling;
}

void AAshForestCharacter::Tick_Climbing(float DeltaTime)
{
	if (GetWorld()->TimeSince(LastStartClimbingTime) > (bIsWallRunning ? WallRunDuration_MAX : ClimbingDuration_MAX))
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (REACHED MAX TIME)")));

		EndClimbing();
		return;
	}

	FVector ledgeLoc;
	if (CheckForLedge(ledgeLoc))
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (FOUND LEDGE)")));

		EndClimbing(true, ledgeLoc);
		return;
	}

	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	FHitResult climbingHit;
	auto bFoundSurface = GetWorld()->SweepSingleByChannel(climbingHit, GetActorLocation(), GetActorLocation() + (CurrentDirToClimbingSurface * (capRadius + 100.f)), FQuat::Identity, ECC_Camera, FCollisionShape::MakeCapsule(capRadius, capHalfHeight), params);
	
	if (bFoundSurface && !CanClimbHitSurface(false, climbingHit))
		bFoundSurface = false;
	
	FVector projectedClimbDir = FVector::VectorPlaneProject(CurrentClimbingDir, CurrentClimbingNormal);

	if (bIsWallRunning)
		projectedClimbDir.Z = 0.f;

	projectedClimbDir = projectedClimbDir.GetSafeNormal();

	if (projectedClimbDir == FVector::ZeroVector)
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (VELOCITY PROJECTION FAILED)")));

		EndClimbing();
		return;
	}

	if (bFoundSurface)
	{
		CurrentClimbingNormal = climbingHit.ImpactNormal;
		CurrentDirToClimbingSurface = (climbingHit.ImpactPoint - GetActorLocation()).GetSafeNormal();

		SoftSetActorLocation(FMath::VInterpTo(climbingHit.Location, GetActorLocation(), DeltaTime, 5.f), true);
		SetActorRotation(FRotator(0.f, (bIsWallRunning ? GetVelocity().GetSafeNormal() : CurrentDirToClimbingSurface).Rotation().Yaw, 0.f));
	}
	else //AS: If we have run out of wall to climb
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (NO VALID SURFACE FOUND)")));

		EndClimbing();
		return;
	}

	//AS: If the player basically hasn't moved since the last frame
	if (PrevClimbingLocation != FVector::ZeroVector && (GetActorLocation() - PrevClimbingLocation).SizeSquared() <= FMath::Square(2.f))
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (STUCK ON GEO)")));

		EndClimbing();
		return;
	}

	auto newVel = projectedClimbDir * ClimbingSpeed_Current;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

	ClimbingSpeed_Current -= (DeltaTime * (bIsWallRunning ? WallRunSpeed_DecayRate : ClimbingSpeed_DecayRate));

	if (ClimbingSpeed_Current <= 0.f)
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (RAN OUT OF SPEED)")));

		EndClimbing();
		return;
	}

	CurrentClimbingDir = projectedClimbDir;
	PrevClimbingLocation = GetActorLocation();
}

void AAshForestCharacter::EndClimbing(const bool bDoClimbOver /*= false*/, const FVector SurfaceTopLocation /*= FVector::ZeroVector*/)
{
	SetAshCustomMoveState(EAshCustomMoveState::EAshMove_NONE);
	
	ResetMeshTransform();

	if (!bDidWallJump)
	{
		if (!bIsWallRunning)
		{
			if (bDoClimbOver)
				ClimbOverLedge(SurfaceTopLocation);
		}
		else
		{
			FVector endWallRunVel = CurrentClimbingDir * ClimbingSpeed_Current;
			((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(endWallRunVel);
		}
	}

	JumpCurrentCount = 0;
	bWasJumping = false;

	LastEndClimbingTime = GetWorld()->GetTimeSeconds();
	CurrentClimbingNormal = FVector::ZeroVector;
	CurrentDirToClimbingSurface = FVector::ZeroVector;
	ClimbingSpeed_Current = 0.f;
	CurrentClimbingDir = FVector::ZeroVector;
	bIsWallRunning = false;
	bDidWallJump = false;
}

void AAshForestCharacter::DoWallJump()
{
	bDidWallJump = true;
	LastWallJumpTime = GetWorld()->GetTimeSeconds();
	DashCooldownTime_Current = bIsWallRunning ? .6f : DashCooldownTime_AfterWallJump;
	LastDashEndTime = GetWorld()->GetTimeSeconds();
	DashesWhileFalling_Current = 0;

	if (!bIsWallRunning)
	{
		FVector wallJumpImpulse = CurrentClimbingNormal * ClimbingJumpImpulseAxisSizes.X;
		wallJumpImpulse.Z = ClimbingJumpImpulseAxisSizes.Y;
		((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(wallJumpImpulse);

		GetCharacterMovement()->AirControl = 0.f;
	}
	else
	{
		FVector wallJumpVel = (((CurrentClimbingDir * 2.f) + CurrentClimbingNormal) / 2.f).GetSafeNormal() * ((UCharacterMovementComponent*)GetMovementComponent())->MaxWalkSpeed;
		wallJumpVel.Z = WallRunJumpVelocityZ;
		((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(wallJumpVel);

		bIsWallRunning = false;	
	}

	EndClimbing();

	GetCharacterMovement()->FallingLateralFriction = 0.f;
	GetCharacterMovement()->bUseSeparateBrakingFriction = false;

	OnWallJump();
}
PRAGMA_ENABLE_OPTIMIZATION
void AAshForestCharacter::OnWallJump_Implementation()
{
	//AS: On wall jump event
}

PRAGMA_DISABLE_OPTIMIZATION
bool AAshForestCharacter::WantsToGrabLedge() const
{
	if (!GetCharacterMovement()->IsFalling() || AshMoveState_Current != EAshCustomMoveState::EAshMove_NONE)
		return false;

	auto dot = FVector::DotProduct(GetLastMovementInputVector(), GetActorForwardVector()/*GetVelocity().GetSafeNormal2D()*/);
	return (dot >= .25f); //|| (dot == 0.f && FVector::DotProduct(GetLastMovementInputVector(), GetActorForwardVector()) > 0.f));
}

bool AAshForestCharacter::TryGrabLedge()
{
	auto retVal = false;

	FVector ledgeLoc;
	if (CheckForLedge(ledgeLoc))
		retVal = ClimbOverLedge(ledgeLoc);

	return retVal;
}

bool AAshForestCharacter::CheckForLedge(FVector & FoundLedgeLocation)
{
	FoundLedgeLocation = FVector::ZeroVector;

	if (bIsWallRunning || GetWorld()->TimeSince(LastGrabLedgeCheckTime) < GrabLedgeCheckInterval)
		return false;

	LastGrabLedgeCheckTime = GetWorld()->GetTimeSeconds();

	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	auto traceOrigin = GetActorLocation() + (FVector::UpVector * (capHalfHeight + 50.f));
	auto traceOrigin_Forward = traceOrigin + (GetActorForwardVector() * (capRadius + 20.f));

	auto traceStart_First_Origin = traceOrigin + ((bLastGrabLedgeSideLeft ? GetActorRightVector() : -GetActorRightVector()) * capRadius);
	auto traceStart_Second_Origin = traceOrigin + ((bLastGrabLedgeSideLeft ? -GetActorRightVector() : GetActorRightVector()) * capRadius);
	auto traceStart_First = traceOrigin_Forward + ((bLastGrabLedgeSideLeft ? GetActorRightVector() : -GetActorRightVector()) * capRadius);
	auto traceStart_Second = traceOrigin_Forward + ((bLastGrabLedgeSideLeft ? -GetActorRightVector() : GetActorRightVector()) * capRadius);
	auto traceEnd_First = traceStart_First + (-FVector::UpVector * (capHalfHeight + 50.f));
	auto traceEnd_Second = traceStart_Second + (-FVector::UpVector * (capHalfHeight + 50.f));

	bLastGrabLedgeSideLeft = !bLastGrabLedgeSideLeft;

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	//AS: Ledge Trace Lambda
	auto DoLedgeTrace = [&](FHitResult & FillHitResult, bool & bFoundLedge, const FVector TraceOrigin, FVector TraceStart, FVector TraceEnd)
	{
		bFoundLedge = GetWorld()->LineTraceSingleByChannel(FillHitResult, TraceOrigin, TraceStart, ECC_Camera, params);

		if (bDebugAshMovement)
			DrawDebugLine(GetWorld(), TraceOrigin, TraceStart, !bFoundLedge ? FColor::Green : FColor::Yellow, false, 5.f, 0, 5.f);

		if (bFoundLedge)
		{
			if (bDebugAshMovement)
				DrawDebugSphere(GetWorld(), FillHitResult.ImpactPoint, 20.f, 16, FColor::Orange, false, 5.f, 0, 3.f);

			TraceStart = FillHitResult.ImpactPoint + FillHitResult.ImpactNormal * .1f;
		}

		bFoundLedge = GetWorld()->LineTraceSingleByChannel(FillHitResult, TraceStart, TraceEnd, ECC_Camera, params);

		if (bDebugAshMovement)
			DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bFoundLedge ? FColor::Green : FColor::Red, false, 5.f, 0, 5.f);

		if (bFoundLedge && !IsValidLedgeHit(FillHitResult))
			bFoundLedge = false;

		if (bDebugAshMovement)
			DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bFoundLedge ? FColor::Green : FColor::Red, false, 5.f, 0, 5.f);

		if (bFoundLedge)
		{
			if (bDebugAshMovement)
				DrawDebugSphere(GetWorld(), FillHitResult.ImpactPoint, 20.f, 16, FColor::Cyan, false, 5.f, 0, 3.f);
		}
	};

	//AS: Use the above lambda to do both ledge traces (assuming the first one succeeds)
	bool bFoundLedge = false;
	FHitResult ledgeHit_First;

	DoLedgeTrace(ledgeHit_First, bFoundLedge, traceStart_First_Origin, traceStart_First, traceEnd_First);

	if (!bFoundLedge)
		return false;

	bFoundLedge = false;
	FHitResult ledgeHit_Second;

	DoLedgeTrace(ledgeHit_Second, bFoundLedge, traceStart_Second_Origin, traceStart_Second, traceEnd_Second);

	if (!bFoundLedge)
		return false;

	//AS: Do center ledge trace to the average found ledge points ======================================================================
	auto avgLedgeLoc = (ledgeHit_First.ImpactPoint + ledgeHit_Second.ImpactPoint) * .5f;
	auto traceStart_center = avgLedgeLoc + FVector(0.f, 0.f, 100.f);
	auto traceEnd_center = traceStart_center + (-FVector::UpVector * 200.f);

	FHitResult ledgeHit_Center;
	auto bFoundLedge_Center = GetWorld()->LineTraceSingleByChannel(ledgeHit_Center, traceStart_center, traceEnd_center, ECC_Camera, params);

	if (bFoundLedge_Center && !IsValidLedgeHit(ledgeHit_Center))
		bFoundLedge_Center = false;

	if (bDebugAshMovement)
		DrawDebugLine(GetWorld(), traceStart_Second, traceEnd_Second, bFoundLedge_Center ? FColor::Green : FColor::Red, false, 5.f, 0, 5.f);

	if (bFoundLedge_Center)
	{
		if (bDebugAshMovement)
			DrawDebugSphere(GetWorld(), ledgeHit_Center.ImpactPoint, 20.f, 16, FColor::Green, false, 5.f, 0, 3.f);
	}
	else
		return false;

	FoundLedgeLocation = (ledgeHit_Center.ImpactPoint + (ledgeHit_Center.ImpactNormal * (capRadius + .1f)));

	return true;
}

bool AAshForestCharacter::IsValidLedgeHit(const FHitResult & LedgeHit)
{
	return LedgeHit.ImpactNormal != FVector::ZeroVector && (FVector::DotProduct(LedgeHit.ImpactNormal, FVector::UpVector) >= .25f);
}
PRAGMA_ENABLE_OPTIMIZATION

bool AAshForestCharacter::ClimbOverLedge(const FVector & FoundLedgeLocation)
{
	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	auto wantsLocation = FoundLedgeLocation + (FVector::UpVector * (capHalfHeight - capRadius));

	//AS: Make sure there is enough space for the player capsule on top of the ledge
	auto bEnoughSpace = !GetWorld()->OverlapAnyTestByChannel(wantsLocation, GetActorRotation().Quaternion(), ECC_Camera, FCollisionShape::MakeCapsule(capRadius, capHalfHeight), params);

	if (bDebugAshMovement)
		DrawDebugCapsule(GetWorld(), wantsLocation, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bEnoughSpace ? FColor::Green : FColor::Red, false, 5.f, 0, 3.f);

	if (bEnoughSpace)
	{
		ResetMeshTransform();
		SoftSetActorLocation(wantsLocation);

		auto newVel = GetVelocity();
		newVel.Z = FMath::Min(newVel.Z, 0.f);
		((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

		return true;
	}

	return false;
}

void AAshForestCharacter::OnLockOnPressed()
{
	bWantsToLockOn = true;
}

void AAshForestCharacter::OnLockOnReleased()
{
	bWantsToLockOn = false;
}

void AAshForestCharacter::OnLockOnSwitchInput(float Dir)
{
	if (IsLockedOn())
		WantsToSwitchLockOnTargetDir = Dir;
}

void AAshForestCharacter::TryLockOn()
{
	if (LockOnTarget_Current == NULL)
		SetLockOnTarget(FindLockOnTarget());
	else
		SetLockOnTarget(NULL);
}

USceneComponent* AAshForestCharacter::FindLockOnTarget(const bool bIgnorePreviousTarget/* = false*/, const FRotator OverrideViewRot /*= FRotator::ZeroRotator*/)
{
	TArray<USceneComponent*> temp;
	return GetPotentialLockOnTargets(temp, bIgnorePreviousTarget, OverrideViewRot);
}

USceneComponent* AAshForestCharacter::GetPotentialLockOnTargets(TArray<USceneComponent*> & PotentialTargets, const bool bIgnorePreviousTarget /*= false*/, const FRotator OverrideViewRot /*= FRotator::ZeroRotator*/)
{
	PotentialTargets.Empty();

	FCollisionObjectQueryParams objectParams;
	objectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);
	objectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> potentialTargets;
	GetWorld()->OverlapMultiByObjectType(potentialTargets, GetActorLocation(), FQuat::Identity, objectParams, FCollisionShape::MakeSphere(LockOnFindTarget_Radius), queryParams);

	if (bDebugAshMovement)
		DrawDebugSphere(GetWorld(), GetActorLocation(), LockOnFindTarget_Radius, 32, FColor::Purple, false, 5.f, 0, 3.f);

	FRotator rotation = GetControlRotation();

	if (OverrideViewRot != FRotator::ZeroRotator)
		rotation = OverrideViewRot;
	else if (LockOnTarget_Current != NULL)
		rotation = (LockOnTarget_Current->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();

	auto YawRotationVec = FRotator(0, rotation.Yaw, 0).Vector();
	USceneComponent* retTarget = NULL;
	TArray<USceneComponent*> currPotentialTargetsArray;
	USceneComponent* currPotentialTarget = NULL;
	auto dirToTarget = FVector::ZeroVector;
	auto angleToTarget_curr = 0.f;
	auto angleToTarget_best = 400.f;
	auto bIsValidTarget = false;

	if (potentialTargets.Num() > 0)
	{
		FHitResult blockingHit;
		FCollisionQueryParams params;
		params.AddIgnoredActor(this);

		FVector viewLoc;
		FRotator viewRot;
		GetController()->GetPlayerViewPoint(viewLoc, viewRot);

		for (FOverlapResult currTarget : potentialTargets)
		{
			if (currTarget.Actor == NULL || currTarget.Actor == this || !currTarget.Actor->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass()) || !ITargetableInterface::Execute_CanBeTargeted(currTarget.Actor.Get(), this))
				continue;

			//AS: Don't switch to non-enemy targets if your current target is a damageable character
			if (LockOnTarget_Current != NULL && LockOnTarget_Current->GetOwner()->IsA(ADamageableCharacter::StaticClass()) && !currTarget.Actor->IsA(ADamageableCharacter::StaticClass()))
				continue;

			if (ITargetableInterface::Execute_GetTargetableComponents(currTarget.Actor.Get(), currPotentialTargetsArray))
			{
				if (currPotentialTargetsArray.Num() == 1)
					currPotentialTarget = currPotentialTargetsArray[0];
				else
					continue;
			}
			else
				continue;

			//AS: Code to ignore previous target
			if ((LockOnTarget_Current != NULL && currPotentialTarget == LockOnTarget_Current)
				|| (bIgnorePreviousTarget && currPotentialTarget == LockOnTarget_Previous)
				|| PotentialTargets.Contains(currPotentialTarget))
				continue;

			bIsValidTarget = false;

			angleToTarget_curr = FMath::Abs(FMath::Acos(FVector::DotProduct((currPotentialTarget->GetComponentLocation() - viewLoc).GetSafeNormal2D(), YawRotationVec)) * (180.f / PI));
			
			if (angleToTarget_curr <= LockOnFindTarget_WithinLookDirAngleDelta)
			{
				auto bPathClear = !GetWorld()->LineTraceSingleByChannel(blockingHit, viewLoc, currPotentialTarget->GetComponentLocation(), ECC_Camera, params);

				if (!bPathClear && blockingHit.Actor != NULL && blockingHit.Actor == currTarget.Actor)
					bPathClear = true;

				if (bPathClear)
				{
					PotentialTargets.AddUnique(currPotentialTarget);

					if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, FString::Printf(TEXT("Found target[%i]: %s [%3.2f] "), PotentialTargets.Num(), *currPotentialTarget->GetName(), angleToTarget_curr));

					if (angleToTarget_curr < angleToTarget_best)
					{
						angleToTarget_best = angleToTarget_curr;
						retTarget = currPotentialTarget;

						bIsValidTarget = true;
					}
				}

				if (bDebugAshMovement)
					DrawDebugLine(GetWorld(), GetActorLocation(), currPotentialTarget->GetComponentLocation(), bIsValidTarget ? FColor::Yellow : FColor::Red, false, 5.f, 0, 3.f);
			}
		}

		if (bDebugAshMovement && retTarget)
			DrawDebugLine(GetWorld(), GetActorLocation(), retTarget->GetComponentLocation(), FColor::Green, false, 5.f, 0, 6.f);
	}

	return retTarget;
}

void AAshForestCharacter::SetLockOnTarget(USceneComponent* NewLockOnTarget_Current)
{
	if (NewLockOnTarget_Current == NULL || LockOnTarget_Current != NewLockOnTarget_Current)
	{
		if(LockOnTarget_Current != NULL)
			LockOnTarget_Previous = LockOnTarget_Current;

		LockOnTarget_Current = NewLockOnTarget_Current;
		OnLockOnTargetUpdated();
	}
}

bool AAshForestCharacter::IsLockedOn(USceneComponent* ToSpecificTarget /*= NULL*/) const
{
	return ToSpecificTarget ? LockOnTarget_Current == ToSpecificTarget : (LockOnTarget_Current != NULL || bShouldBeLockedOn);
}

bool AAshForestCharacter::TrySwitchLockOnTarget(const float & RightInput)
{
	if (RightInput == 0.f || LockOnTarget_Current == NULL || GetWorld()->TimeSince(LastSwitchLockOnTargetTime) < AllowSwitchLockOnTargetInterval)
		return false;

	TArray<USceneComponent*> potentialTargets;
	GetPotentialLockOnTargets(potentialTargets);

	FVector viewLoc;
	FRotator viewRot;
	GetController()->GetPlayerViewPoint(viewLoc, viewRot);

	auto dirToCurrentTarget = (LockOnTarget_Current->GetComponentLocation() - viewLoc).GetSafeNormal2D();
	auto YawRotationVec = FRotator(0, Controller->GetControlRotation().Yaw, 0).Vector();
	
	USceneComponent* bestTarget = NULL;
	auto dirToTarget = FVector::ZeroVector;
	auto angleToTarget_curr = 0.f;
	auto angleToTarget_best = 400.f;
	auto bIsValidTarget = false;

	for (USceneComponent* currTarget : potentialTargets)
	{
		if (currTarget == LockOnTarget_Current)
			continue;

		dirToTarget = (currTarget->GetComponentLocation() - viewLoc).GetSafeNormal2D();
 		angleToTarget_curr = FMath::Acos(FVector::DotProduct(dirToCurrentTarget, dirToTarget)) * (180.f / PI);

		if (FVector::CrossProduct(dirToCurrentTarget, dirToTarget).Z < 0.f)
			angleToTarget_curr *= -1.f;

		if (FMath::Sign(RightInput) == FMath::Sign(angleToTarget_curr))
		{
			if (FMath::Abs(angleToTarget_curr) < FMath::Abs(angleToTarget_best))
			{
				bestTarget = currTarget;
				angleToTarget_best = angleToTarget_curr;
			}
		}
	}

	if (bestTarget)
	{
		LastSwitchLockOnTargetTime = GetWorld()->GetTimeSeconds();

		SetLockOnTarget(bestTarget);
		return true;
	}

	return false;
}

void AAshForestCharacter::SwitchLockOnTarget_Left()
{
	if (IsLockedOn())
		WantsToSwitchLockOnTargetDir = -1;
}

void AAshForestCharacter::SwitchLockOnTarget_Right()
{
	if (IsLockedOn())
		WantsToSwitchLockOnTargetDir = 1;
}

void AAshForestCharacter::OnLockOnTargetUpdated_Implementation()
{
	bShouldBeLockedOn = LockOnTarget_Current != NULL;
	bUseControllerRotationYaw = LockOnTarget_Current != NULL;
}

void AAshForestCharacter::Tick_LockedOn(float DeltaTime)
{
	//AS: If our current target ref has become invalid
	if (LockOnTarget_Current == NULL)
	{
		AutoSwitchLockOnTarget();
		return;
	}

	//AS: If our current target can no longer be targeted
	if (!LockOnTarget_Current.Get()->GetOwner() || !LockOnTarget_Current.Get()->GetOwner()->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass())
		|| !ITargetableInterface::Execute_CanBeTargeted(LockOnTarget_Current.Get()->GetOwner(), this))
	{
		AutoSwitchLockOnTarget(LockOnTarget_Current.Get());
		return;
	}

	if (bDebugAshMovement)
		DrawDebugLine(GetWorld(), GetActorLocation(), LockOnTarget_Current->GetComponentLocation(), FColor::Magenta, false, -1.f, 0, 3.f);

	auto newControlRot = GetControlRotation();
	auto RotToTarget = (LockOnTarget_Current->GetComponentLocation() - GetPawnViewLocation()).GetSafeNormal().Rotation();
	RotToTarget.Roll = 0.f;

	auto rotDelta = (RotToTarget - newControlRot);

	if (FMath::Abs(rotDelta.Pitch) > .1f || FMath::Abs(rotDelta.Yaw) > .1f)
		newControlRot = FMath::RInterpTo(GetControlRotation(), RotToTarget, DeltaTime, LockOnInterpViewToTargetSpeed);
	else
		newControlRot = RotToTarget;

	GetController()->SetControlRotation(newControlRot);
}

void AAshForestCharacter::Tick_UpdateCamera(float DeltaTime)
{
	if (IsLockedOn())
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, LockedOnCameraSocketOffset, DeltaTime, LockedOnInterpCameraSocketOffsetSpeed_IN);
	else
	{
		if (!bIsFocusing && bWantsToFocus && CurrentFocusPointTrigger != nullptr)
			SetIsFocusing(true);

		if (bIsFocusing && CurrentFocusPointTrigger != nullptr)
		{
			if (CurrentFocusPointTrigger->FocusTargetCameraArmLength_InterpSpeed > 0.f)
				CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, CurrentFocusPointTrigger->FocusTargetCameraArmLength, DeltaTime, CurrentFocusPointTrigger->FocusTargetCameraArmLength_InterpSpeed);
			else
				CameraBoom->TargetArmLength = (FMath::FInterpTo(CameraBoom->TargetArmLength, CameraArmLength_Default, DeltaTime, (CameraArmLength_Default >= CameraBoom->TargetArmLength) ? CameraArmLengthInterpSpeeds.X : CameraArmLengthInterpSpeeds.Y));

			if (CurrentFocusPointTrigger->FocusTargetCameraSocketOffset_InterpSpeed > 0.f)
				CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, CurrentFocusPointTrigger->FocusTargetCameraSocketOffset, DeltaTime, CurrentFocusPointTrigger->FocusTargetCameraSocketOffset_InterpSpeed);
			else
				CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, DefaultCameraSocketOffset, DeltaTime, LockedOnInterpCameraSocketOffsetSpeed_OUT);

			if (MyCameraManager && CurrentFocusPointTrigger->FocusTargetFOV_InterpSpeed > 0.f)
				MyCameraManager->SetFOV(FMath::FInterpTo(MyCameraManager->GetFOVAngle(), CurrentFocusPointTrigger->FocusTargetFOV, DeltaTime, CurrentFocusPointTrigger->FocusTargetFOV_InterpSpeed));
			else
				MyCameraManager->SetFOV(FMath::FInterpTo(MyCameraManager->GetFOVAngle(), MyCameraManager->DefaultFOV, DeltaTime, WarpFOV_InterpSpeed));

			if (CurrentFocusPointTrigger->FocusPointActor != nullptr && CurrentFocusPointTrigger->ControlRotation_InterpSpeed > 0.f)
			{
				FRotator newControlRot = GetControlRotation();
				FRotator RotToTarget = (CurrentFocusPointTrigger->FocusPointActor->GetActorLocation() - GetPawnViewLocation()).GetSafeNormal().Rotation();
				RotToTarget.Roll = 0.f;

				auto rotDelta = (RotToTarget - newControlRot);

				if (FMath::Abs(rotDelta.Pitch) > .1f || FMath::Abs(rotDelta.Yaw) > .1f)
					newControlRot = FMath::RInterpTo(GetControlRotation(), RotToTarget, DeltaTime, CurrentFocusPointTrigger->ControlRotation_InterpSpeed);
				else
					newControlRot = RotToTarget;

				GetController()->SetControlRotation(newControlRot);
			}
		}
		else
		{
			const float velRatio = FMath::Clamp(GetVelocity().Size() / GetCharacterMovement()->MaxWalkSpeed, 0.f, 1.f);
			const FVector newSocketOffset = FMath::Lerp(DefaultCameraSocketOffset, CameraSocketVelocityOffset_MAX, velRatio);
			CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, newSocketOffset, DeltaTime, LockedOnInterpCameraSocketOffsetSpeed_OUT);

			const float newLength = FMath::Lerp(CameraArmLength_Default, CameraArmLength_MAX, velRatio);
			CameraBoom->TargetArmLength = (FMath::FInterpTo(CameraBoom->TargetArmLength, newLength, DeltaTime, (newLength >= CameraBoom->TargetArmLength) ? CameraArmLengthInterpSpeeds.X : CameraArmLengthInterpSpeeds.Y));

			if (MyCameraManager)
			{
				//AS: Lerp the FOV wider as the player looks up to make the world seem larger/taller than it is
				if (GetControlRotation().Pitch >= 45.f && GetControlRotation().Pitch <= 90.f)
				{
					const float targetFOV = FMath::Lerp(MyCameraManager->DefaultFOV, WarpFOV_MAX, FMath::Clamp(((GetControlRotation().Pitch - 45.f) / 45.f), 0.f, 1.f));
					MyCameraManager->SetFOV(FMath::FInterpTo(MyCameraManager->GetFOVAngle(), targetFOV, DeltaTime, WarpFOV_InterpSpeed));
				}
				else
					MyCameraManager->SetFOV(FMath::FInterpTo(MyCameraManager->GetFOVAngle(), MyCameraManager->DefaultFOV, DeltaTime, WarpFOV_InterpSpeed));
			}
		}
	}
}

void AAshForestCharacter::SoftSetActorLocation(const FVector & NewLocation, const bool & bSweep /*= false*/)
{
	auto meshLoc = GetMesh()->GetComponentLocation();

	SetActorLocation(NewLocation, bSweep);

	GetMesh()->SetWorldLocation(meshLoc);

	bIsMeshTransformInterpolating = true;
}

void AAshForestCharacter::SoftSetActorRotation(const FRotator & NewRotation)
{
	auto meshRot = GetMesh()->GetComponentRotation();

	SetActorRotation(NewRotation);

	GetMesh()->SetWorldRotation(meshRot);

	bIsMeshTransformInterpolating = true;
}

void AAshForestCharacter::SoftSetActorLocationAndRotation(const FVector & NewLocation, const FRotator & NewRotation, const bool & bSweep /*= false*/)
{
	SoftSetActorLocation(NewLocation, bSweep);
	SoftSetActorRotation(NewRotation);
}

void AAshForestCharacter::Tick_MeshInterp(float DeltaTime)
{
	if (!bIsMeshTransformInterpolating)
		return;

	auto bStillInterping = false;

	auto currLoc = GetMesh()->GetRelativeTransform().GetLocation();
	if (currLoc != MeshTargetRelLocation)
	{
		bStillInterping = true;

		auto locDelta = MeshTargetRelLocation - currLoc;

		if (!locDelta.IsNearlyZero(.25f))
			GetMesh()->SetRelativeLocation(FMath::VInterpTo(currLoc, MeshTargetRelLocation, DeltaTime, MeshInterpSpeed_Location));
		else
			GetMesh()->SetRelativeLocation(MeshTargetRelLocation);
	}

	auto currRot = GetMesh()->GetRelativeTransform().GetRotation().Rotator();
	auto rotDelta = MeshTargetRelRotation - currRot;
	if (!rotDelta.IsNearlyZero(.03f))
	{
		bStillInterping = true;

		auto rotDelta = MeshTargetRelRotation - currRot;

		if (!rotDelta.IsNearlyZero(.05f))
			GetMesh()->SetRelativeRotation(FMath::RInterpTo(currRot, MeshTargetRelRotation, DeltaTime, MeshInterpSpeed_Rotation));
		else
			GetMesh()->SetRelativeRotation(MeshTargetRelRotation);
	}

	bIsMeshTransformInterpolating = bStillInterping;
}

void AAshForestCharacter::ResetMeshTransform()
{
	MeshTargetRelLocation = FVector(0.f, 0.f, -95.f);
	MeshTargetRelRotation = FRotator(0.f, 270.f, 0.f);

	bIsMeshTransformInterpolating = true;
}

void AAshForestCharacter::Tick_UpdateHealth(float DeltaTime)
{
	if (CurrentHealth < MaxHealth)
	{
		CurrentHealth = FMath::Clamp(CurrentHealth + (HealthRestoreRate * DeltaTime), 0.f, MaxHealth);
	}
}

void AAshForestCharacter::DeflectProjectile(AActor* HitProjectile)
{
	if (HitProjectile->IsA(AAshForestProjectile::StaticClass()))
	{
		GetCapsuleComponent()->IgnoreActorWhenMoving(HitProjectile, true);

		auto deflectDir = LockOnTarget_Current != NULL ? (LockOnTarget_Current->GetComponentLocation() - HitProjectile->GetActorLocation()).GetSafeNormal() : CurrentDashDir;
		
		if (LockOnTarget_Current != NULL && FVector::DotProduct(-((AAshForestProjectile*)HitProjectile)->GetProjectileMovement()->GetVelocity().GetSafeNormal(), deflectDir) <= .1f)
			deflectDir = CurrentDashDir;

		auto deflectedVel = (DashSpeed * .5f) * deflectDir;
		((AAshForestProjectile*)HitProjectile)->OnDeflected(this, deflectedVel);
	}
}

void AAshForestCharacter::OnKilledEnemy(AActor* KilledEnemy)
{
	if (!KilledEnemy || !KilledEnemy->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass()))
		return;

	USceneComponent* killedTargetComp = NULL;
	TArray<USceneComponent*> comps;
	if (ITargetableInterface::Execute_GetTargetableComponents(KilledEnemy, comps))
	{
		if (comps.Num() == 1)
			killedTargetComp = comps[0];
		else
			return;
	}

	AutoSwitchLockOnTarget(killedTargetComp);
}

void AAshForestCharacter::AutoSwitchLockOnTarget(USceneComponent* OldTarget /*= NULL*/)
{
	if (LockOnTarget_Current == OldTarget)
	{
		SetLockOnTarget(NULL);

		if (OldTarget)
			SetLockOnTarget(FindLockOnTarget(true, (OldTarget->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation()));
		else
			SetLockOnTarget(FindLockOnTarget(false));
	}
	else
		SetLockOnTarget(NULL);
}

void AAshForestCharacter::TargetableDie(const AActor* Murderer)
{
	ITargetableInterface::Execute_OnTargetableDeath(this, Murderer);
	Respawn();
}

void AAshForestCharacter::OnTouchCheckpoint(AActor* Checkpoint)
{
	if (Checkpoint && Checkpoint->IsA(AAshForestCheckpoint::StaticClass()))
	{
		auto index = ((AAshForestCheckpoint*)Checkpoint)->GetCheckpointIndex();

		if (index >= 0 && (LatestCheckpoint == nullptr || index > LatestCheckpointIndex))
		{
			LatestCheckpoint = Checkpoint;
			LatestCheckpointIndex = index;

			OnCheckpointUpdated();
		}
	}
}

void AAshForestCharacter::OnCheckpointUpdated_Implementation()
{

}

void AAshForestCharacter::Respawn()
{
	if (LatestCheckpoint)
	{
		CurrentHealth = MaxHealth;

		auto newTrans = ((AAshForestCheckpoint*)LatestCheckpoint)->GetRespawnTransform();
		newTrans.SetScale3D(FVector(1.f));

		SetActorTransform(newTrans);
		GetController()->SetControlRotation(newTrans.GetRotation().Rotator());
		SetLockOnTarget(NULL);

		FVector zeroVel = FVector::ZeroVector;
		((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(zeroVel);

		OnRespawned();
	}
}

void AAshForestCharacter::OnRespawned_Implementation()
{

}

void AAshForestCharacter::StartFocusing()
{
	bWantsToFocus = true;

	if (CurrentFocusPointTrigger)
		SetIsFocusing(true);
}

void AAshForestCharacter::StopFocusing()
{
	bWantsToFocus = false;

	SetIsFocusing(false);
}

void AAshForestCharacter::SetIsFocusing(bool NewFocusing)
{
	if (bIsFocusing != NewFocusing)
	{
		bIsFocusing = NewFocusing;
		OnFocusStateChanged();
	}
}

void AAshForestCharacter::SetFocusPointTrigger(AFocusPointTrigger* NewTrigger)
{
	if (NewTrigger != CurrentFocusPointTrigger)
	{
		CurrentFocusPointTrigger = NewTrigger;

		if (CurrentFocusPointTrigger == nullptr)
			SetIsFocusing(false);

		OnFocusPointTriggerUpdated();
	}
}

void AAshForestCharacter::OnFocusPointTriggerUpdated_Implementation()
{
	//AS: DO SHIT IN BP
}

void AAshForestCharacter::OnFocusStateChanged_Implementation()
{
	//AS: DO SHIT IN BP
}
