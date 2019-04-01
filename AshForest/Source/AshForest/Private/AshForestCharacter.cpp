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
#include "AshForestCreature.h"

//////////////////////////////////////////////////////////////////////////
// AAshForestCharacter

AAshForestCharacter::AAshForestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	DashCooldownTime = .1f;
	DashSpeed = 9000.f;
	DashDuration_MAX = .3f;
	DashDistance_MAX = 750.f;

	ClimbingSpeed_Start = 1200.f;
	ClimbingSpeed_DecayRate = 800.f;
	ClimbingDuration_MAX = 1.f;

	GrabLedgeCheckInterval = .05;

	LockOnFindTarget_Radius = 1500.f;
	LockOnFindTarget_WithinLookDirAngleDelta = 44.5f;
	LockOnInterpViewToTargetSpeed = 5.f;
	LockedOnCameraSocketOffset = FVector(50.f, 250.f, 80.f);

	MeshInterpSpeed_Location = 8.f;
	MeshInterpSpeed_Rotation = 5.f;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AAshForestCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//AS: Custom Actions
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AAshForestCharacter::TryDash);
	PlayerInputComponent->BindAction("LockOn", IE_Pressed, this, &AAshForestCharacter::TryLockOn);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAshForestCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAshForestCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AAshForestCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AAshForestCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &AAshForestCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AAshForestCharacter::LookUpAtRate);
}

void AAshForestCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	if (!HasLockOnTarget())
		AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	else if (FMath::Abs(Rate) >= .5f && HasLockOnTarget())
		TrySwitchLockOnTarget(Rate);
}

void AAshForestCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	if(!HasLockOnTarget())
		AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AAshForestCharacter::AddControllerPitchInput(float Val)
{
	if (!HasLockOnTarget())
		Super::AddControllerPitchInput(Val);
}

void AAshForestCharacter::AddControllerYawInput(float Val)
{
	if (!HasLockOnTarget())
		Super::AddControllerYawInput(Val);
}

void AAshForestCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && AshMoveState_Current == EAshCustomMoveState::EAshMove_NONE)
	{
		// find out which way is forward
		FRotator Rotation = GetControlRotation();

		if (HasLockOnTarget())
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

		if (HasLockOnTarget())
			Rotation = (LockOnTarget_Current->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();

		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AAshForestCharacter::Tick(float DeltaTime)
{
	switch (AshMoveState_Current)
	{
	case EAshCustomMoveState::EAshMove_DASHING:
		Dash_Tick(DeltaTime);
		break;
	case EAshCustomMoveState::EAshMove_CLIMBING:
		Climbing_Tick(DeltaTime);
		break;
	default:
		if (WantsToGrabLedge())
			TryGrabLedge();
		break;
	}

	Tick_UpdateCamera(DeltaTime);

	if (HasLockOnTarget()) 
		Tick_LockedOn(DeltaTime);

	if (bIsMeshTransformInterpolating)
		Tick_MeshInterp(DeltaTime);
}

void AAshForestCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode /*= 0*/)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (((UCharacterMovementComponent*)GetMovementComponent())->IsWalking())
		bIsDashActive = true;
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

void AAshForestCharacter::TryDash()
{
	if (CanDash(true))
	{
		auto dir = GetLastMovementInputVector();

		//AS: Set the dash dir to the look vector if the player wasn't pressing any movement inputs
		if (dir == FVector::ZeroVector)
		{
			FRotator rotation = GetControlRotation();
			if (HasLockOnTarget())
				rotation = (LockOnTarget_Current->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();
				
			dir = FRotator(0, rotation.Yaw, 0).Vector();
		}

		if (dir != FVector::ZeroVector)
			StartDash(dir);
	}
}

bool AAshForestCharacter::CanDash(const bool bIsForStart /*= false*/) const
{
	return bIsForStart ? ((GetWorld()->TimeSince(LastDashEndTime) > DashCooldownTime) && AshMoveState_Current == EAshCustomMoveState::EAshMove_NONE && bIsDashActive) : (IsDashing());
}

void AAshForestCharacter::StartDash(FVector & DashDir)
{
	if (!DashDir.IsNormalized())
		DashDir = DashDir.GetSafeNormal();

	SetAshCustomMoveState(EAshCustomMoveState::EAshMove_DASHING);

	OriginalDashDir = CurrentDashDir = DashDir;
	PrevDashLoc = GetActorLocation();
	DashDistance_Current = DashDistance_MAX;
	LastDashStartTime = GetWorld()->GetTimeSeconds();

	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->MaxWalkSpeed = DashSpeed;
	GetCharacterMovement()->GroundFriction = 0.f;
	GetCharacterMovement()->FallingLateralFriction = 0.f;

	SetActorRotation(FRotator(0.f, CurrentDashDir.Rotation().Yaw, 0.f));

	auto newVel = CurrentDashDir * DashSpeed;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);
}

void AAshForestCharacter::Dash_Tick(float DeltaTime)
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
	if (DashDistance_Current <= 0.f)
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (REACHED MAX DISTANCE)")));
		
		EndDash();
		return;
	}

	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	if (HasLockOnTarget() && (GetActorLocation() - LockOnTarget_Current->GetComponentLocation()).SizeSquared2D() > FMath::Square(DashDistance_MAX * .5f))
	{
		auto dirFromTarget = (GetActorLocation() - LockOnTarget_Current->GetComponentLocation()).GetSafeNormal2D();

		if (FVector::DotProduct(-dirFromTarget, OriginalDashDir) <= 0.f)
		{
			OriginalDashDir = FVector::VectorPlaneProject(OriginalDashDir, dirFromTarget).GetSafeNormal2D();

			if (bDebugAshMovement)
				DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (OriginalDashDir * 100.f), FColor::Blue, false, 5.f, 0, 5.f);
		}
	}

	CurrentDashDir = OriginalDashDir;

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	auto traceStart = GetActorLocation();
	auto traceEnd = GetActorLocation() + (OriginalDashDir * DashDistance_Current);
	FHitResult dashHit;
	auto bFoundHit = GetWorld()->SweepSingleByChannel(dashHit, traceStart, traceEnd, GetActorRotation().Quaternion(), ECollisionChannel::ECC_Visibility, FCollisionShape::MakeCapsule(capRadius, capHalfHeight), params);
	
	if (bDebugAshMovement)
	{
		DrawDebugCapsule(GetWorld(), traceStart, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
		DrawDebugLine(GetWorld(), traceStart, traceEnd, bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
		DrawDebugCapsule(GetWorld(), traceEnd, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
	}
	
	if (bFoundHit)
	{
		if (dashHit.Actor != NULL)
		{
			if (dashHit.Actor->IsA(APawn::StaticClass()))
			{
				SetActorLocation(dashHit.Location);
				EndDashWithHit(dashHit);

				if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (HIT ENEMY)")));

				return;
			}
			else
			{
				//AS: Try to dash around smaller blockers or up ramps
				auto hitForwardDot = FVector::DotProduct(dashHit.ImpactNormal, OriginalDashDir);
				
				if (hitForwardDot < 0.f)
				{
					auto hitUpDot = FVector::DotProduct(dashHit.ImpactNormal, FVector::UpVector);

					if (hitUpDot >= .5f)
					{
						if ((dashHit.Location - GetActorLocation()).SizeSquared2D() <= 1.f/*FMath::Square(5.f)*/)
						{
							CurrentDashDir = FVector::VectorPlaneProject(OriginalDashDir, dashHit.ImpactNormal);//OriginalDashDir.ProjectOnToNormal(dashHit.ImpactNormal).GetSafeNormal();
							
							if (bDebugAshMovement)
								DrawDebugLine(GetWorld(), dashHit.Location, dashHit.Location + (CurrentDashDir * 50.f), FColor::Magenta, false, 5.f, 0, 5.f);
						}
					}
					else
					{
						SetActorLocation(dashHit.Location);
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

	auto movementCompCDO = GetDefault<UCharacterMovementComponent>();
	GetCharacterMovement()->GravityScale = movementCompCDO->GravityScale;
	GetCharacterMovement()->GroundFriction = movementCompCDO->GroundFriction;
	GetCharacterMovement()->MaxWalkSpeed = movementCompCDO->MaxWalkSpeed;
	GetCharacterMovement()->FallingLateralFriction = movementCompCDO->FallingLateralFriction;

	auto newVel = GetVelocity().GetClampedToSize2D(0.f, 1000.f);
	newVel.Z = 0.f;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

	if (!((UCharacterMovementComponent*)GetMovementComponent())->IsWalking())
		bIsDashActive = false;
}

void AAshForestCharacter::EndDashWithHit(const FHitResult EndHit)
{
	if (!IsDashing())
		return;

	EndDash();

	if (EndHit.ImpactNormal != FVector::ZeroVector)
	{
		if (!CanClimbHitSurface(EndHit))
		{
			auto impulseDir = ((EndHit.ImpactNormal + FVector::UpVector) * .5f).GetSafeNormal();
			((UCharacterMovementComponent*)GetMovementComponent())->AddImpulse(impulseDir * 1000.f, true);
		}
		else
			StartClimbing(EndHit);
	}
}

bool AAshForestCharacter::CanClimbHitSurface(const FHitResult & SurfaceHit) const
{
	return SurfaceHit.Component->CanBeClimbed();
}

void AAshForestCharacter::StartClimbing(const FHitResult & ClimbingSurfaceHit)
{
	SetAshCustomMoveState(EAshCustomMoveState::EAshMove_CLIMBING);
	
	LastStartClimbingTime = GetWorld()->GetTimeSeconds();
	CurrentClimbingNormal = ClimbingSurfaceHit.ImpactNormal;
	CurrentDirToClimbingSurface = (ClimbingSurfaceHit.ImpactPoint - GetActorLocation()).GetSafeNormal();
	ClimbingSpeed_Current = ClimbingSpeed_Start;
	PrevClimbingLocation = FVector::ZeroVector;

	((UCharacterMovementComponent*)GetMovementComponent())->SetMovementMode(MOVE_Falling);

	auto newVel = FVector::VectorPlaneProject(FVector::UpVector, CurrentClimbingNormal).GetSafeNormal() * ClimbingSpeed_Current;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);
}

void AAshForestCharacter::Climbing_Tick(float DeltaTime)
{
	if (GetWorld()->TimeSince(LastStartClimbingTime) > ClimbingDuration_MAX)
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

	//AS: If the player basically hasn't moved since the last frame
	if (PrevClimbingLocation != FVector::ZeroVector && (GetActorLocation() - PrevClimbingLocation).SizeSquared() <= FMath::Square(2.f))
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (STUCK ON GEO)")));

		EndClimbing();
		return;
	}

	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	FHitResult climbingHit;
	auto bFoundSurface = GetWorld()->SweepSingleByChannel(climbingHit, GetActorLocation(), GetActorLocation() + (CurrentDirToClimbingSurface * 100.f), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeCapsule(capRadius, capHalfHeight), params);
	
	if (bFoundSurface && !CanClimbHitSurface(climbingHit))
		bFoundSurface = false;
	
	auto projectedClimbDir = FVector::VectorPlaneProject(FVector::UpVector, CurrentClimbingNormal).GetSafeNormal();

	if (bFoundSurface)
	{
		CurrentClimbingNormal = climbingHit.ImpactNormal;
		CurrentDirToClimbingSurface = (climbingHit.ImpactPoint - GetActorLocation()).GetSafeNormal();

		SoftSetActorLocation(FMath::VInterpTo(climbingHit.Location, GetActorLocation(), DeltaTime, 5.f), true);
		SetActorRotation(FRotator(0.f, (CurrentDirToClimbingSurface).Rotation().Yaw, 0.f));

		//auto newMeshRot = FRotationMatrix::MakeFromZX(climbingHit.ImpactNormal, projectedClimbDir).Rotator();
		//MeshTargetRelRotation = (GetActorRotation() - newMeshRot);
	}
	else
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (NO VALID SURFACE FOUND)")));

		EndClimbing();
		return;
	}

	auto newVel = projectedClimbDir * ClimbingSpeed_Current;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

	ClimbingSpeed_Current -= (DeltaTime * ClimbingSpeed_DecayRate);

	if (ClimbingSpeed_Current <= 0.f)
	{
		if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END CLIMBING (RAN OUT OF SPEED)")));

		EndClimbing();
		return;
	}

	PrevClimbingLocation = GetActorLocation();
}

void AAshForestCharacter::EndClimbing(const bool bDoClimbOver /*= false*/, const FVector SurfaceTopLocation /*= FVector::ZeroVector*/)
{
	SetAshCustomMoveState(EAshCustomMoveState::EAshMove_NONE);

	LastEndClimbingTime = GetWorld()->GetTimeSeconds();
	CurrentClimbingNormal = FVector::ZeroVector;
	CurrentDirToClimbingSurface = FVector::ZeroVector;
	ClimbingSpeed_Current = 0.f;
	
	ResetMeshTransform();

	if (!bDoClimbOver)
	{
		auto newVel = GetVelocity();
		newVel.Z = FMath::Min(newVel.Z, 0.f);
		((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);
	}
	else
		ClimbOverLedge(SurfaceTopLocation);
}

PRAGMA_DISABLE_OPTIMIZATION
bool AAshForestCharacter::WantsToGrabLedge() const
{
	return GetCharacterMovement()->IsFalling() && /*GetVelocity().Z <= 0.f &&*/ AshMoveState_Current == EAshCustomMoveState::EAshMove_NONE && FVector::DotProduct(GetLastMovementInputVector(), GetVelocity().GetSafeNormal2D()) > .25f;
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

	if (GetWorld()->TimeSince(LastGrabLedgeCheckTime) < GrabLedgeCheckInterval)
		return false;

	LastGrabLedgeCheckTime = GetWorld()->GetTimeSeconds();

	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	auto traceOrigin = GetActorLocation() + (FVector::UpVector * (capHalfHeight + 50.f));
	auto traceOrigin_Forward = traceOrigin + (GetActorForwardVector() * (capRadius + 20.f));

	auto traceStart_Left_Origin = traceOrigin + (-GetActorRightVector() * capRadius);
	auto traceStart_Right_Origin = traceOrigin + (GetActorRightVector() * capRadius);
	auto traceStart_Left = traceOrigin_Forward + (-GetActorRightVector() * capRadius);
	auto traceStart_Right = traceOrigin_Forward + (GetActorRightVector() * capRadius);
	auto traceEnd_Left = traceStart_Left + (-FVector::UpVector * (capHalfHeight + 50.f));
	auto traceEnd_Right = traceStart_Right + (-FVector::UpVector * (capHalfHeight + 50.f));

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	//AS: Do left ledge trace =========================================================================================================
	FHitResult ledgeHit_Left;
	auto bFoundLedge_Left = GetWorld()->LineTraceSingleByChannel(ledgeHit_Left, traceStart_Left_Origin, traceStart_Left, ECC_Visibility, params);

	if (bDebugAshMovement)
		DrawDebugLine(GetWorld(), traceStart_Left_Origin, traceStart_Left, !bFoundLedge_Left ? FColor::Green : FColor::Yellow, false, 5.f, 0, 5.f);

	if (bFoundLedge_Left)
	{
		if (bDebugAshMovement)
			DrawDebugSphere(GetWorld(), ledgeHit_Left.ImpactPoint, 20.f, 16, FColor::Orange, false, 5.f, 0, 3.f);

		traceStart_Left = ledgeHit_Left.ImpactPoint + ledgeHit_Left.ImpactNormal * .1f;
	}

	bFoundLedge_Left = GetWorld()->LineTraceSingleByChannel(ledgeHit_Left, traceStart_Left, traceEnd_Left, ECC_Visibility, params);
	
	if (bDebugAshMovement)
		DrawDebugLine(GetWorld(), traceStart_Left, traceEnd_Left, bFoundLedge_Left ? FColor::Green : FColor::Red, false, 5.f, 0, 5.f);

	if (bFoundLedge_Left && !IsValidLedgeHit(ledgeHit_Left))
		bFoundLedge_Left = false;

	if (bDebugAshMovement)
		DrawDebugLine(GetWorld(), traceStart_Left, traceEnd_Left, bFoundLedge_Left ? FColor::Green : FColor::Red, false, 5.f, 0, 5.f);

	if (bFoundLedge_Left)
	{
		if (bDebugAshMovement)
			DrawDebugSphere(GetWorld(), ledgeHit_Left.ImpactPoint, 20.f, 16, FColor::Cyan, false, 5.f, 0, 3.f);
	}
	else
		return false;

	//AS: Do right ledge trace =========================================================================================================
	FHitResult ledgeHit_Right;
	auto bFoundLedge_Right = GetWorld()->LineTraceSingleByChannel(ledgeHit_Right, traceStart_Right_Origin, traceStart_Right, ECC_Visibility, params);

	bFoundLedge_Right = GetWorld()->LineTraceSingleByChannel(ledgeHit_Right, traceStart_Right, traceEnd_Right, ECC_Visibility, params);

	if (bFoundLedge_Right)
	{
		if (bDebugAshMovement)
			DrawDebugSphere(GetWorld(), ledgeHit_Right.ImpactPoint, 20.f, 16, FColor::Orange, false, 5.f, 0, 3.f);

		traceStart_Right = ledgeHit_Right.ImpactPoint + ledgeHit_Right.ImpactNormal * .1f;
	}

	if (bDebugAshMovement)
		DrawDebugLine(GetWorld(), traceStart_Right, traceEnd_Right, bFoundLedge_Right ? FColor::Green : FColor::Red, false, 5.f, 0, 5.f);

	if (bFoundLedge_Right)
	{
		if (bDebugAshMovement)
			DrawDebugSphere(GetWorld(), ledgeHit_Right.ImpactPoint, 20.f, 16, FColor::Cyan, false, 5.f, 0, 3.f);
	}
	else
		return false;

	//AS: Do center ledge trace to the average found ledge points ======================================================================
	auto avgLedgeLoc = (ledgeHit_Left.ImpactPoint + ledgeHit_Right.ImpactPoint) * .5f;
	auto traceStart_center = avgLedgeLoc + FVector(0.f, 0.f, 100.f);
	auto traceEnd_center = traceStart_center + (-FVector::UpVector * 200.f);

	FHitResult ledgeHit_Center;
	auto bFoundLedge_Center = GetWorld()->LineTraceSingleByChannel(ledgeHit_Center, traceStart_center, traceEnd_center, ECC_Visibility, params);

	if (bFoundLedge_Center && !IsValidLedgeHit(ledgeHit_Center))
		bFoundLedge_Center = false;

	if (bDebugAshMovement)
		DrawDebugLine(GetWorld(), traceStart_Right, traceEnd_Right, bFoundLedge_Center ? FColor::Green : FColor::Red, false, 5.f, 0, 5.f);

	if (bFoundLedge_Center)
	{
		if (bDebugAshMovement)
			DrawDebugSphere(GetWorld(), ledgeHit_Center.ImpactPoint, 20.f, 16, FColor::Green, false, 5.f, 0, 3.f);
	}
	else
		return false;

	FoundLedgeLocation = (ledgeHit_Center.ImpactPoint + (ledgeHit_Center.ImpactNormal * .1f));

	return true;
}

bool AAshForestCharacter::IsValidLedgeHit(const FHitResult & LedgeHit)
{
	return LedgeHit.ImpactNormal != FVector::ZeroVector && CanClimbHitSurface(LedgeHit) && (FVector::DotProduct(LedgeHit.ImpactNormal, FVector::UpVector) >= .25f);
}
PRAGMA_ENABLE_OPTIMIZATION

bool AAshForestCharacter::ClimbOverLedge(const FVector & FoundLedgeLocation)
{
	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	auto wantsLocation = FoundLedgeLocation + (FVector::UpVector * capHalfHeight);

	//AS: Make sure there is enough space for the player capsule on top of the ledge
	auto bEnoughSpace = !GetWorld()->OverlapAnyTestByChannel(wantsLocation, GetActorRotation().Quaternion(), ECC_Visibility, FCollisionShape::MakeCapsule(capRadius, capHalfHeight), params);

	if (bDebugAshMovement)
		DrawDebugCapsule(GetWorld(), wantsLocation, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bEnoughSpace ? FColor::Green : FColor::Red, false, 5.f, 0, 3.f);

	if (bEnoughSpace)
	{
		ResetMeshTransform();
		SoftSetActorLocation(wantsLocation);

		auto newVel = FVector::ZeroVector;//GetVelocity();
		//newVel.Z = FMath::Min(newVel.Z, 10.f);
		((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

		return true;
	}

	return false;
}

void AAshForestCharacter::TryLockOn()
{
	if (!HasLockOnTarget())
		SetLockOnTarget(FindLockOnTarget());
	else
		SetLockOnTarget(NULL);
}

USceneComponent* AAshForestCharacter::FindLockOnTarget()
{
	TArray<USceneComponent*> temp;
	return GetPotentialLockOnTargets(temp);
}

USceneComponent* AAshForestCharacter::GetPotentialLockOnTargets(TArray<USceneComponent*> & PotentialTargets)
{
	PotentialTargets.Empty();

	FCollisionObjectQueryParams objectParams;
	//objectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
	objectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> potentialTargets;
	GetWorld()->OverlapMultiByObjectType(potentialTargets, GetActorLocation(), FQuat::Identity, objectParams, FCollisionShape::MakeSphere(LockOnFindTarget_Radius), queryParams);

	if (bDebugAshMovement)
		DrawDebugSphere(GetWorld(), GetActorLocation(), LockOnFindTarget_Radius, 32, FColor::Purple, false, 5.f, 0, 3.f);

	FRotator rotation = GetControlRotation();
	if (HasLockOnTarget())
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
		for (FOverlapResult currTarget : potentialTargets)
		{
			if (currTarget.Actor == NULL || !currTarget.Actor->IsA(AAshForestCreature::StaticClass()))
				continue;

			if (((AAshForestCreature*)currTarget.Actor.Get())->GetTargetableComponents(currPotentialTargetsArray))
			{
				if (currPotentialTargetsArray.Num() == 1)
					currPotentialTarget = currPotentialTargetsArray[0];
				else
					continue;
			}
			else
				continue;

			if (HasLockOnTarget() && currPotentialTarget == LockOnTarget_Current)
				continue;

			bIsValidTarget = false;

			FVector viewLoc;
			FRotator viewRot;
			GetController()->GetPlayerViewPoint(viewLoc, viewRot);

			angleToTarget_curr = FMath::Abs(FMath::Acos(FVector::DotProduct((currPotentialTarget->GetComponentLocation() - viewLoc).GetSafeNormal2D(), YawRotationVec)) * (180.f / PI));

			if (bDebugAshMovement && GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("Maybe target: %s [%3.2f] "), *currTarget.Actor->GetName(), angleToTarget_curr));

			if (angleToTarget_curr <= (LockOnFindTarget_WithinLookDirAngleDelta))
			{
				PotentialTargets.Add(currPotentialTarget);

				if (angleToTarget_curr < angleToTarget_best)
				{
					angleToTarget_best = angleToTarget_curr;
					retTarget = currPotentialTarget;

					bIsValidTarget = true;
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
	if (LockOnTarget_Current != NewLockOnTarget_Current)
	{
		LockOnTarget_Current = NewLockOnTarget_Current;
		OnLockOnTargetUpdated();
	}
}

bool AAshForestCharacter::HasLockOnTarget(USceneComponent* SpecificTarget /*= NULL*/) const
{
	return SpecificTarget ? LockOnTarget_Current == SpecificTarget : LockOnTarget_Current != NULL;
}

bool AAshForestCharacter::TrySwitchLockOnTarget(const float & RightInput)
{
	if (RightInput == 0.f)
		return false;

	/*TArray<USceneComponent*> potentialTargets;
	GetPotentialLockOnTarget_Currents(potentialTargets);

	auto YawRotationVec = FRotator(0, Controller->GetControlRotation().Yaw, 0).Vector();
	USceneComponent* retTarget = NULL;
	auto dirToTarget = FVector::ZeroVector;
	auto angleToTarget_curr = 0.f;
	auto angleToTarget_best = 400.f;
	auto bIsValidTarget = false;

	for (USceneComponent* currTarget : potentialTargets)
	{
		
	}*/

	return false;
}

void AAshForestCharacter::OnLockOnTargetUpdated()
{

}

void AAshForestCharacter::Tick_LockedOn(float DeltaTime)
{
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
	if (HasLockOnTarget())
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, LockedOnCameraSocketOffset, DeltaTime, 5.f);
	else
		CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
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
	if (currRot != MeshTargetRelRotation)
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
