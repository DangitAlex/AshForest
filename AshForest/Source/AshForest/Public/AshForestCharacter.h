// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AshForestCharacter.generated.h"

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

//AS: =========================================================================
//AS: Dashing ================================================================
	
	//AS: The cooldown time after using the dash where dashing is prevented
	UPROPERTY(BlueprintReadWrite, Category = "Dash")
		float DashCooldownTime;

	//AS: The magnitude that dashing sets to the player velocity multiplied by the dash direction vector
	UPROPERTY(BlueprintReadWrite, Category = "Dash")
		float DashSpeed;

	UPROPERTY(BlueprintReadWrite, Category = "Dash")
		float DashDuration_MAX;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Dash")
		float DashDuration_Current;

	//AS: The maximum distance the dash can ever take you, assuming no collision in the dashing path
	UPROPERTY(BlueprintReadWrite, Category = "Dash")
		float DashDistance_MAX;

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
		bool bIsDashing;

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void TryDash();

	UFUNCTION(BlueprintCallable, Category = "Dash")
		bool CanDash() const;

	//UFUNCTION(BlueprintCallable, Category = "Dash")
	FORCEINLINE bool IsDashing() const { return bIsDashing; };

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void StartDash(FVector & DashDir);

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void Dash_Tick(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Dash")
		void EndDash(const FVector EndHitNormal = FVector::ZeroVector);

//AS: =========================================================================
//AS: Climbing ================================================================



//AS: =========================================================================
//AS: Lock-On =================================================================
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock-On")
		TWeakObjectPtr<USceneComponent> LockOnTarget;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lock-On")
		TWeakObjectPtr<USceneComponent> PrevLockOnTarget;

	UPROPERTY(BlueprintReadWrite, Category = "Lock-On")
		float LockOnFindTarget_Radius;

	UPROPERTY(BlueprintReadWrite, Category = "Lock-On")
		float LockOnFindTarget_WithinLookDirAngleDelta;

	UPROPERTY(BlueprintReadWrite, Category = "Lock-On")
		float LockOnInterpViewToTargetSpeed;

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

//AS: =========================================================================
//AS: =========================================================================

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

