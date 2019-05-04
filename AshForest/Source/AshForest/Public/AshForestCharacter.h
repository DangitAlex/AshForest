// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DamageableCharacter.h"
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

USTRUCT(Blueprintable)
struct FInitialCharMovementVars
{
	GENERATED_USTRUCT_BODY()

	float InitialGravityScale;
	float InitialAcceleration;
	float InitialGroundFriction;
	float InitialMaxWalkSpeed;
	float InitialFallingLateralFriction;
	float InitialAirControl;
};

UCLASS(config=Game)
class AAshForestCharacter : public ADamageableCharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	FInitialCharMovementVars MyInitialMovementVars;
public:
	AAshForestCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	float BaseLookUpRate;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	virtual void Jump() override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	virtual void TargetableDie(const AActor* Murderer) override;

protected:

	void MoveForward(float Value);
	void MoveRight(float Value);

	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;

	void OnMouseWheelScroll(float Rate);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashCooldownTime_Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashCooldownTime_AfterWallJump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		int32 DashCharges_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashChargeReloadInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		int32 AllowedDashesWhileFalling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashDuration_MAX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashDistance_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
		float DashCooldownTime_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		int32 DashCharges_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		float DashChargeReloadDurationCurrent;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		int32 DashesWhileFalling_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		float DashDuration_Current;

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

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		FVector OriginalDashStartLocation;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		TArray<AActor*> DashDamagedActors;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		bool bAllowedToDash;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		bool bWantsToDash;

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void OnDashPressed();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void OnDashReleased();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void TryDash();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		bool CanDash(const bool bIsForStart = false) const;

	UFUNCTION(BlueprintPure, Category = "Climbing") FORCEINLINE
		bool IsDashing() const { return AshMoveState_Current == EAshCustomMoveState::EAshMove_DASHING; };

	UFUNCTION(BlueprintNativeEvent, Category = "Dash")
		void OnDash();

	UFUNCTION(BlueprintNativeEvent, Category = "Dash")
		void OnDashRecharge();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void StartDash(FVector & DashDir);

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void Tick_Dash(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void EndDash();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void EndDashWithHit(const FHitResult & EndHit);

//AS: =========================================================================
//AS: Climbing ================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
		float ClimbingSpeed_Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
		float ClimbingSpeed_DecayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
		float ClimbingDuration_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
		FVector2D ClimbingJumpImpulseAxisSizes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
		float WallRunSpeed_Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
		float WallRunSpeed_DecayRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
		float WallRunDuration_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Running")
		float WallRunJumpVelocityZ;

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
		float LastWallJumpTime;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		bool bDidWallJump;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		float ClimbingSpeed_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		FVector CurrentClimbingDir;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Climbing")
		bool bIsWallRunning;

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		bool CanClimbHitSurface(const bool & bIsForStart, const FHitResult & SurfaceHit) const;

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		void StartClimbing(const FHitResult & ClimbingSurfaceHit);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		void Tick_Climbing(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		void EndClimbing(const bool bDoClimbOver = false, const FVector SurfaceTopLocation = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
		void DoWallJump();

	UFUNCTION(BlueprintNativeEvent, Category = "Climbing")
		void OnWallJump();

//AS: ===========================================================================
//AS: Ledge Grab ================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing")
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
//AS: Lock On =================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
		float LockOnFindTarget_Radius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
		float LockOnFindTarget_WithinLookDirAngleDelta;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
		float AllowSwitchLockOnTargetInterval;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock On")
		TWeakObjectPtr<USceneComponent> LockOnTarget_Current;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock On")
		TWeakObjectPtr<USceneComponent> LockOnTarget_Previous;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock On")
		bool bShouldBeLockedOn;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock On")
		bool bCanSwitchLockOnTarget;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock On")
		bool bWantsToLockOn;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock On")
		int32 WantsToSwitchLockOnTargetDir;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock On")
		float LastSwitchLockOnTargetTime;

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void OnLockOnPressed();

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void OnLockOnReleased();

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void OnLockOnSwitchInput(float Dir);

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void TryLockOn();

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		USceneComponent* FindLockOnTarget(const bool bIgnorePreviousTarget = false, const FRotator OverrideViewRot = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		USceneComponent* GetPotentialLockOnTargets(TArray<USceneComponent*> & PotentialTargets, const bool bIgnorePreviousTarget = false, const FRotator OverrideViewRot = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void SetLockOnTarget(USceneComponent* NewLockOnTarget);

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		bool IsLockedOn(USceneComponent* ToSpecificTarget = NULL) const;

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		bool TrySwitchLockOnTarget(const float & RightInput);

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void SwitchLockOnTarget_Left();

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void SwitchLockOnTarget_Right();

	UFUNCTION(BlueprintNativeEvent, Category = "Lock On")
		void OnLockOnTargetUpdated();

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void AutoSwitchLockOnTarget(USceneComponent* OldTarget = NULL);

	UFUNCTION(BlueprintCallable, Category = "Lock On")
		void Tick_LockedOn(float DeltaTime);

//AS: =========================================================================
//AS: Camera  =================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ash Camera")
		float LockOnInterpViewToTargetSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
		float LockedOnInterpCameraSocketOffsetSpeed_IN;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock On")
		float LockedOnInterpCameraSocketOffsetSpeed_OUT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ash Camera")
		FVector DefaultCameraSocketOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ash Camera")
		FVector CameraSocketVelocityOffset_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ash Camera")
		FVector LockedOnCameraSocketOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ash Camera")
		float CameraArmLength_MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ash Camera")
		FVector2D CameraArmLengthInterpSpeeds;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Ash Camera")
		float CameraArmLength_Default;

	UFUNCTION(BlueprintCallable, Category = "Ash Camera")
		void Tick_UpdateCamera(float DeltaTime);

//AS: =========================================================================
//AS: Mesh Interpolation ======================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Interp")
		float MeshInterpSpeed_Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Interp")
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
//AS: Combat ==================================================================
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
		float HealthRestoreRate;

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void Tick_UpdateHealth(float DeltaTime);

public:
	UFUNCTION(BlueprintCallable, Category = "Combat")
		void DeflectProjectile(AActor* HitProjectile);

protected:
//AS: =========================================================================
//AS: Respawning ==============================================================

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Respawning")
		AActor* LatestCheckpoint;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Respawning")
		int32 LatestCheckpointIndex;

	UFUNCTION(BlueprintCallable, Category = "Respawning")
		void Respawn();

	UFUNCTION(BlueprintNativeEvent, Category = "Respawning")
		void OnRespawned();

	UFUNCTION(BlueprintNativeEvent, Category = "Respawning")
		void OnCheckpointUpdated();

public:
	UFUNCTION(BlueprintCallable, Category = "Respawning")
		void OnTouchCheckpoint(AActor* Checkpoint);

//AS: =========================================================================
//AS: Cash Moniez ==============================================================

	UPROPERTY(BlueprintReadWrite, Transient, Category = "Rewards")
		int32 CurrentSmolMoniez;

	UPROPERTY(BlueprintReadWrite, Transient, Category = "Rewards")
		int32 CurrentBigUnitMoniez;

//AS: =========================================================================
//AS: =========================================================================

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, Category = "Ash Movement") FORCEINLINE
		TEnumAsByte<EAshCustomMoveState::Type> GetCurrentAshMoveState() const { return AshMoveState_Current; };

	UFUNCTION(BlueprintCallable, Category = "Combat")
		void OnKilledEnemy(AActor* KilledEnemy);


};

