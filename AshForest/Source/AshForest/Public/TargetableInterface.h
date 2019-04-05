// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTargetableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ASHFOREST_API ITargetableInterface
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Lock On")
		bool GetTargetableComponents(TArray<USceneComponent*> & TargetableComps);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
		bool CanBeTargeted(const AActor* ByActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
		bool CanBeDamaged(const AActor* DamageCauser, const FHitResult & DamageHitEvent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
		bool IgnoresCollisionWithDamager(const AActor* DamageCauser, const FHitResult & DamageHitEvent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damage")
		void TakeDamage(const AActor* DamageCauser, const float & DamageAmount, const FHitResult & DamageHitEvent);
};
