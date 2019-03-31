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

	LockOnFindTarget_Radius = 1500.f;
	LockOnFindTarget_WithinLookDirAngleDelta = 44.5f;
	LockOnInterpViewToTargetSpeed = 5.f;
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
	if ((Controller != NULL) && (Value != 0.0f) && !IsDashing())
	{
		// find out which way is forward
		FRotator Rotation = GetControlRotation();

		if (HasLockOnTarget())
			Rotation = (LockOnTarget->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();

		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AAshForestCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f) && !IsDashing())
	{
		// find out which way is right
		FRotator Rotation = GetControlRotation();

		if (HasLockOnTarget())
			Rotation = (LockOnTarget->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();

		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AAshForestCharacter::Tick(float DeltaTime)
{
	if (IsDashing()) Dash_Tick(DeltaTime);
	if (HasLockOnTarget()) Tick_LockedOn(DeltaTime);

	if (GEngine)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("Acceleration: %s"), *((UCharacterMovementComponent*)GetMovementComponent())->GetCurrentAcceleration().ToString()));
		//GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, FString::Printf(TEXT("Velocity: %s"), *GetVelocity().ToString()));
	}
}

void AAshForestCharacter::TryDash()
{
	if (CanDash())
	{
		auto dir = GetLastMovementInputVector();

		//AS: Set the dash dir to the look vector if the player wasn't pressing any movement inputs
		if (dir == FVector::ZeroVector)
		{
			FRotator rotation = GetControlRotation();
			if (HasLockOnTarget())
				rotation = (LockOnTarget->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();
				
			dir = FRotator(0, rotation.Yaw, 0).Vector();
		}

		if (dir != FVector::ZeroVector)
			StartDash(dir);
	}
}

bool AAshForestCharacter::CanDash() const
{
	return (GetWorld()->TimeSince(LastDashEndTime) > DashCooldownTime) && !IsDashing();
}

void AAshForestCharacter::StartDash(FVector & DashDir)
{
	if (!DashDir.IsNormalized())
		DashDir = DashDir.GetSafeNormal();

	bIsDashing = true;
	OriginalDashDir = CurrentDashDir = DashDir;
	PrevDashLoc = GetActorLocation();
	DashDistance_Current = DashDistance_MAX;
	LastDashStartTime = GetWorld()->GetTimeSeconds();

	GetCharacterMovement()->GravityScale = 0.f;
	GetCharacterMovement()->MaxWalkSpeed = DashSpeed;
	GetCharacterMovement()->GroundFriction = 0.f;
	GetCharacterMovement()->FallingLateralFriction = 0.f;

	auto newVel = CurrentDashDir * DashSpeed;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);
}

void AAshForestCharacter::Dash_Tick(float DeltaTime)
{
	if (!IsDashing())
		return;

	if (GetWorld()->TimeSince(LastDashStartTime) > DashDuration_MAX)
	{
		EndDash();  

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (REACHED MAX TIME)")));

		return;
	}

	DashDistance_Current -= (GetActorLocation() - PrevDashLoc).Size2D();

	if (DashDistance_Current <= 0.f)
	{
		EndDash();

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (REACHED MAX DISTANCE)")));

		return;
	}

	float capRadius;
	float capHalfHeight;
	GetCapsuleComponent()->GetScaledCapsuleSize(capRadius, capHalfHeight);

	if (HasLockOnTarget() && (GetActorLocation() - LockOnTarget->GetComponentLocation()).SizeSquared2D() > FMath::Square(DashDistance_MAX * .5f))
	{
		auto dirFromTarget = (GetActorLocation() - LockOnTarget->GetComponentLocation()).GetSafeNormal2D();

		if (FVector::DotProduct(-dirFromTarget, OriginalDashDir) <= 0.f)
		{
			/*auto prevDirFromTarget = (PrevDashLoc - LockOnTarget->GetComponentLocation()).GetSafeNormal2D();
			auto deltaAngle = FMath::Acos(FVector::DotProduct(dirFromTarget, prevDirFromTarget)) * (180.f / PI);

			if (FVector::CrossProduct(dirFromTarget, prevDirFromTarget).Z < 0.f)
				deltaAngle *= -1.f;

			OriginalDashDir = FRotator(0.f, deltaAngle, 0.f).UnrotateVector(OriginalDashDir);*/

			OriginalDashDir = FVector::VectorPlaneProject(OriginalDashDir, dirFromTarget).GetSafeNormal2D();

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
	
	DrawDebugCapsule(GetWorld(), traceStart, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
	DrawDebugLine(GetWorld(), traceStart, traceEnd, bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
	DrawDebugCapsule(GetWorld(), traceEnd, capHalfHeight, capRadius, GetActorRotation().Quaternion(), bFoundHit ? FColor::Orange : FColor::Green, false, -1.f, 0, 3.f);
	
	if (bFoundHit)
	{
		if (dashHit.Actor != NULL)
		{
			if (dashHit.Actor->IsA(APawn::StaticClass()))
			{
				SetActorLocation(dashHit.Location);
				EndDash(dashHit.ImpactNormal);

				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (HIT ENEMY)")));

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
							
							DrawDebugLine(GetWorld(), dashHit.Location, dashHit.Location + (CurrentDashDir * 50.f), FColor::Magenta, false, 5.f, 0, 5.f);
						}
					}
					else
					{
						SetActorLocation(dashHit.Location);
						EndDash(dashHit.ImpactNormal);

						DrawDebugLine(GetWorld(), dashHit.ImpactPoint, dashHit.ImpactPoint + (dashHit.ImpactNormal * 50.f), FColor::Red, false, 5.f, 0, 8.f);

						if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("END DASH (HIT WALL)")));

						return;
					}

					DrawDebugLine(GetWorld(), dashHit.ImpactPoint, dashHit.ImpactPoint + (dashHit.ImpactNormal * 50.f), FColor::Yellow, false, 5.f, 0, 5.f);
				}
			}
		}
	}

	auto newVel = CurrentDashDir * DashSpeed;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);

	PrevDashLoc = GetActorLocation();
}

void AAshForestCharacter::EndDash(const FVector EndHitNormal /*= FVector::ZeroVector*/)
{
	if (!IsDashing())
		return;

	bIsDashing = false;
	LastDashEndTime = GetWorld()->GetTimeSeconds();

	auto movementCompCDO = GetDefault<UCharacterMovementComponent>();
	GetCharacterMovement()->GravityScale = movementCompCDO->GravityScale;
	GetCharacterMovement()->GroundFriction = movementCompCDO->GroundFriction;
	GetCharacterMovement()->MaxWalkSpeed = movementCompCDO->MaxWalkSpeed;
	GetCharacterMovement()->FallingLateralFriction = movementCompCDO->FallingLateralFriction;

	auto newVel = GetVelocity().GetClampedToSize2D(0.f, 1000.f);
	newVel.Z = 0.f;
	//auto newVel = FVector::ZeroVector;
	((UCharacterMovementComponent*)GetMovementComponent())->OverrideVelocity(newVel);
	((UCharacterMovementComponent*)GetMovementComponent())->StopActiveMovement();

	if (EndHitNormal != FVector::ZeroVector)
	{
		auto impulseDir = ((EndHitNormal + FVector::UpVector) * .5f).GetSafeNormal();
		((UCharacterMovementComponent*)GetMovementComponent())->AddImpulse(impulseDir * 1000.f, true);
	}
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
	objectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(this);

	TArray<FOverlapResult> potentialTargets;
	GetWorld()->OverlapMultiByObjectType(potentialTargets, GetActorLocation(), FQuat::Identity, objectParams, FCollisionShape::MakeSphere(LockOnFindTarget_Radius), queryParams);

	DrawDebugSphere(GetWorld(), GetActorLocation(), LockOnFindTarget_Radius, 32, FColor::Purple, false, 5.f, 0, 3.f);

	FRotator rotation = GetControlRotation();
	if (HasLockOnTarget())
		rotation = (LockOnTarget->GetComponentLocation() - GetActorLocation()).GetSafeNormal2D().Rotation();

	auto YawRotationVec = FRotator(0, rotation.Yaw, 0).Vector();
	USceneComponent* retTarget = NULL;
	auto dirToTarget = FVector::ZeroVector;
	auto angleToTarget_curr = 0.f;
	auto angleToTarget_best = 400.f;
	auto bIsValidTarget = false;

	if (potentialTargets.Num() > 0)
	{
		for (FOverlapResult currTarget : potentialTargets)
		{
			if (currTarget.Actor == NULL)
				continue;

			if (HasLockOnTarget() && currTarget.Actor->GetRootComponent() == LockOnTarget)
				continue;

			bIsValidTarget = false;

			FVector viewLoc;
			FRotator viewRot;
			GetController()->GetPlayerViewPoint(viewLoc, viewRot);

			angleToTarget_curr = FMath::Abs(FMath::Acos(FVector::DotProduct((currTarget.Actor->GetRootComponent()->GetComponentLocation() - viewLoc).GetSafeNormal2D(), YawRotationVec)) * (180.f / PI));

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Orange, FString::Printf(TEXT("Maybe target: %s [%3.2f] "), *currTarget.Actor->GetName(), angleToTarget_curr));

			if (angleToTarget_curr <= (LockOnFindTarget_WithinLookDirAngleDelta))
			{
				PotentialTargets.Add(currTarget.Actor->GetRootComponent());

				if (angleToTarget_curr < angleToTarget_best)
				{
					angleToTarget_best = angleToTarget_curr;
					retTarget = currTarget.Actor->GetRootComponent();

					bIsValidTarget = true;
				}

				DrawDebugLine(GetWorld(), GetActorLocation(), currTarget.Actor->GetRootComponent()->GetComponentLocation(), bIsValidTarget ? FColor::Yellow : FColor::Red, false, 5.f, 0, 3.f);
			}
		}

		if (retTarget)
			DrawDebugLine(GetWorld(), GetActorLocation(), retTarget->GetComponentLocation(), FColor::Green, false, 5.f, 0, 6.f);
	}

	return retTarget;
}

void AAshForestCharacter::SetLockOnTarget(USceneComponent* NewLockOnTarget)
{
	if (LockOnTarget != NewLockOnTarget)
	{
		LockOnTarget = NewLockOnTarget;
		OnLockOnTargetUpdated();
	}
}

bool AAshForestCharacter::HasLockOnTarget(USceneComponent* SpecificTarget /*= NULL*/) const
{
	return SpecificTarget ? LockOnTarget == SpecificTarget : LockOnTarget != NULL;
}

bool AAshForestCharacter::TrySwitchLockOnTarget(const float & RightInput)
{
	if (RightInput == 0.f)
		return false;

	/*TArray<USceneComponent*> potentialTargets;
	GetPotentialLockOnTargets(potentialTargets);

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
	DrawDebugLine(GetWorld(), GetActorLocation(), LockOnTarget->GetComponentLocation(), FColor::Magenta, false, -1.f, 0, 3.f);

	auto newControlRot = GetControlRotation();
	auto RotToTarget = (LockOnTarget->GetComponentLocation() - GetPawnViewLocation()).GetSafeNormal().Rotation();
	RotToTarget.Roll = 0.f;

	auto rotDelta = (RotToTarget - newControlRot);

	if (FMath::Abs(rotDelta.Pitch) > .1f || FMath::Abs(rotDelta.Yaw) > .1f)
		newControlRot = FMath::RInterpTo(GetControlRotation(), RotToTarget, DeltaTime, LockOnInterpViewToTargetSpeed);
	else
		newControlRot = RotToTarget;

	GetController()->SetControlRotation(newControlRot);
}