// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AshForestCharacter.generated.h"

UENUM(BlueprintType)
namespace EAshCustomMoveState
{
	enum Type
	{
		EAshMove_NONE			UMETA(DisplayName = "None"),
		EAshMove_DASHING 		UMETA(DisplayName = "Dashing"),
		EAshMove_CLIMBING		UMETA(DisplayName = "Climbing"),
		EAshMove_MAX			UMETA(Hidden)
	};
}

UCLASS(config=Game)
class AAshForestCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AAshForestCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	virtual void Tick(float DeltaTime) override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

protected:

	void MoveForward(float Value);
	void MoveRight(float Value);

	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	UPROPERTY(EditAnywhere, Category = "Debug")
		bool bDebugAshMovement;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Ash Movement")
		TEnumAsByte<EAshCustomMoveState::Type> AshMoveState_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Ash Movement")
		TEnumAsByte<EAshCustomMoveState::Type> AshMoveState_Previous;

	UFUNCTION(BlueprintCallable, Category = "Ash Movement")
		void SetAshCustomMoveState(TEnumAsByte<EAshCustomMoveState::Type> NewMoveState);

	UFUNCTION(BlueprintCallable, Category = "Ash Movement")
		void OnAshCustomMoveStateChanged();

//AS: =========================================================================
//AS: Dashing ================================================================

	UPROPERTY(EditAnywhere, Category = "Dash")
		float DashCooldownTime;

	UPROPERTY(EditAnywhere, Category = "Dash")
		float DashSpeed;

	UPROPERTY(EditAnywhere, Category = "Dash")
		float DashDuration_MAX;

	UPROPERTY(EditAnywhere, Transient, Category = "Dash")
		float DashDuration_Current;

	UPROPERTY(EditAnywhere, Category = "Dash")
		float DashDistance_MAX;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		bool bIsDashActive;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		float DashDistance_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		float LastDashStartTime;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		float LastDashEndTime;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		FVector OriginalDashDir;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		FVector CurrentDashDir;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		FVector PrevDashLoc;

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void TryDash();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		bool CanDash(const bool bIsForStart = false) const;

	UFUNCTION(BlueprintPure, Category = "Climbing") FORCEINLINE
		bool IsDashing() const { return AshMoveState_Current == EAshCustomMoveState::EAshMove_DASHING; };

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void StartDash(FVector & DashDir);

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void Dash_Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void EndDash();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void EndDashWithHit(const FHitResult EndHit);

//AS: =========================================================================
//AS: Climbing ================================================================

	UPROPERTY(EditAnywhere, Category = "Climbing")
		float ClimbingSpeed_Start;

	UPROPERTY(EditAnywhere, Category = "Climbing")
		float ClimbingSpeed_DecayRate;

	UPROPERTY(EditAnywhere, Category = "Climbing")
		float ClimbingDuration_MAX;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		FVector CurrentClimbingNormal;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		FVector CurrentDirToClimbingSurface;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		FVector PrevClimbingLocation;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		float LastStartClimbingTime;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		float LastEndClimbingTime;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		float ClimbingSpeed_Current;

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		bool CanClimbHitSurface(const FHitResult & SurfaceHit) const;

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		void StartClimbing(const FHitResult & ClimbingSurfaceHit);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		void Climbing_Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		void EndClimbing(const bool bDoClimbOver = false, const FVector SurfaceTopLocation = FVector::ZeroVector);

//AS: ===========================================================================
//AS: Ledge Grab ================================================================

	UPROPERTY(EditAnywhere, Category = "Climbing")
		float GrabLedgeCheckInterval;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		float LastGrabLedgeCheckTime;
	
	UFUNCTION(BlueprintCallable, Category = "Ledge Grab")
		bool WantsToGrabLedge() const;
	
	UFUNCTION(BlueprintCallable, Category = "Ledge Grab")
		bool TryGrabLedge();
	
	UFUNCTION(BlueprintCallable, Category = "Ledge Grab")
		bool CheckForLedge(FVector & FoundLedgeLocation);

	UFUNCTION(BlueprintCallable, Category = "Ledge Grab")
		bool IsValidLedgeHit(const FHitResult & LedgeHit);

	UFUNCTION(BlueprintCallable, Category = "Ledge Grab")
		bool ClimbOverLedge(const FVector & FoundLedgeLocation);

//AS: =========================================================================
//AS: Lock-On =================================================================

	UPROPERTY(EditAnywhere, Category = "Lock-On")
		float LockOnFindTarget_Radius;

	UPROPERTY(EditAnywhere, Category = "Lock-On")
		float LockOnFindTarget_WithinLookDirAngleDelta;

	UPROPERTY(EditAnywhere, Category = "Lock-On")
		float LockOnInterpViewToTargetSpeed;

	UPROPERTY(EditAnywhere, Category = "Lock-On")
		FVector LockedOnCameraSocketOffset;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock-On")
		TWeakObjectPtr<USceneComponent> LockOnTarget_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock-On")
		TWeakObjectPtr<USceneComponent> LockOnTarget_Previous;

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		void TryLockOn();

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		USceneComponent* FindLockOnTarget();

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		USceneComponent* GetPotentialLockOnTargets(TArray<USceneComponent*> & PotentialTargets);

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		void SetLockOnTarget(USceneComponent* NewLockOnTarget);

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		bool HasLockOnTarget(USceneComponent* SpecificTarget = NULL) const;

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		bool TrySwitchLockOnTarget(const float & RightInput);

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		void OnLockOnTargetUpdated();

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		void Tick_LockedOn(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Lock-On")
		void Tick_UpdateCamera(float DeltaTime);

//AS: =========================================================================
//AS: Mesh Interpolation ======================================================

	UPROPERTY(EditAnywhere, Category = "Mesh Interp")
		float MeshInterpSpeed_Location;

	UPROPERTY(EditAnywhere, Category = "Mesh Interp")
		float MeshInterpSpeed_Rotation;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Mesh Interp")
		FVector MeshTargetRelLocation;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Mesh Interp")
		FRotator MeshTargetRelRotation;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Mesh Interp")
		bool bIsMeshTransformInterpolating;

	UFUNCTION(BlueprintCallable, Category = "Mesh Interp")
		void SoftSetActorLocation(const FVector & NewLocation, const bool & bSweep = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Interp")
		void SoftSetActorRotation(const FRotator & NewRotation);

	UFUNCTION(BlueprintCallable, Category = "Mesh Interp")
		void SoftSetActorLocationAndRotation(const FVector & NewLocation, const FRotator & NewRotation, const bool & bSweep = false);

	UFUNCTION(BlueprintCallable, Category = "Mesh Interp")
		void Tick_MeshInterp(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Mesh Interp")
		void ResetMeshTransform();

//AS: =========================================================================
//AS: =========================================================================

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

